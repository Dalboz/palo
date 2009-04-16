////////////////////////////////////////////////////////////////////////////////
/// @brief palo cube
///
/// @file
///
/// Copyright (C) 2006-2008 Jedox AG
///
/// This program is free software; you can redistribute it and/or modify it
/// under the terms of the GNU General Public License (Version 2) as published
/// by the Free Software Foundation at http://www.gnu.org/copyleft/gpl.html.
///
/// This program is distributed in the hope that it will be useful, but WITHOUT
/// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
/// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
/// more details.
///
/// You should have received a copy of the GNU General Public License along with
/// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
/// Place, Suite 330, Boston, MA 02111-1307 USA
/// 
/// You may obtain a copy of the License at
///
/// <a href="http://www.jedox.com/license_palo.txt">
///   http://www.jedox.com/license_palo.txt
/// </a>
///
/// If you are developing and distributing open source applications under the
/// GPL License, then you are free to use Palo under the GPL License.  For OEMs,
/// ISVs, and VARs who distribute Palo with their products, and do not license
/// and distribute their source code under the GPL, Jedox provides a flexible
/// OEM Commercial License.
///
/// Developed by triagens GmbH, Koeln on behalf of Jedox AG. Intellectual
/// property rights has triagens GmbH, Koeln. Exclusive worldwide
/// exploitation right (commercial copyright) has Jedox AG, Freiburg.
///
/// @author Frank Celler, triagens GmbH, Cologne, Germany
/// @author Achim Brandt, triagens GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
#pragma warning( disable : 4267 )
#endif

#include "Olap/Cube.h"

#include <limits>
#include <math.h>
#include <algorithm>
#include <functional>
#include <iostream>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif


#if defined(_MSC_VER)
#include <float.h>
#endif

#include "Exceptions/ParameterException.h"
#include "Exceptions/FileFormatException.h"
#include "Exceptions/FileOpenException.h"
#include "Exceptions/KeyBufferException.h"

#include "InputOutput/Condition.h"
#include "InputOutput/FileReader.h"
#include "InputOutput/FileWriter.h"
#include "InputOutput/FileUtils.h"
#include "InputOutput/JournalFileReader.h"
#include "InputOutput/JournalFileWriter.h"
#include "InputOutput/Logger.h"
#include "InputOutput/Statistics.h"

#include "Parser/RuleParserDriver.h"

#include "Olap/AreaStorage.h"
#include "Olap/AttributesCube.h"
#include "Olap/CacheConsolidationsStorage.h"
#include "Olap/CacheRulesStorage.h"
#include "Olap/CellPath.h"
#include "Olap/CubeStorage.h"
#include "Olap/Database.h"
#include "Olap/Dimension.h"
#include "Olap/Element.h"
#include "Olap/ExportStorage.h"
#include "Olap/HashAreaStorage.h"
#include "Olap/Lock.h"
#include "Olap/MarkerStorage.h"
#include "Olap/NormalCube.h"
#include "Olap/PaloSession.h"
#include "Olap/RightsCube.h"
#include "Olap/RollbackStorage.h"
#include "Olap/Rule.h"
#include "Olap/Server.h"
#include "Olap/SubsetViewCube.h"
#include "Olap/UserInfoCube.h"

#include "Olap/GoalSeekSolver.h"

static const bool DEBUG_FILE = false;

namespace palo {
	double Cube::splashLimit1 = 1000.0; // error
	double Cube::splashLimit2 = 500.0; // warning
	double Cube::splashLimit3 = 100.0; // info

	int Cube::goalseekCellLimit = 1000;
	int Cube::goalseekTimeoutMiliSec = 10000;

	bool Cube::ignoreCellData = false; // for DEBUGGING only


	double   Cube::cacheBarrier = 5000.0;
	uint32_t Cube::cacheClearBarrier = 5 ;
	double   Cube::cacheClearBarrierCells = 1000.0;

	void Cube::setMaximumCacheSize (size_t max) {
		CacheConsolidationsPage::setMaximumCacheSize(max);
	}


	void Cube::setMaximumRuleCacheSize (size_t max) {
		CacheRulesPage::setMaximumCacheSize(max);
	}


	void Cube::setCacheBarrier (double barrier) {
		cacheBarrier = barrier;
	}

	void Cube::setCacheClearBarrier (uint32_t barrier) {
		cacheClearBarrier = barrier;
	}

	void Cube::setCacheClearBarrierCells (double barrier) {
		cacheClearBarrierCells = barrier;
	}

	void Cube::setGoalseekCellLimit(int cells) {
		goalseekCellLimit = cells;
	}

	void Cube::setGoalseekTimeout(int ms) {
		goalseekTimeoutMiliSec = ms;
	}

	////////////////////////////////////////////////////////////////////////////////
	// auxillary functions
	////////////////////////////////////////////////////////////////////////////////

	static void fillBaseElements (Dimension* dimension,
		Element* element,
		set<Element*>* base) {
			Element::ElementType type = element->getElementType();

			if (type == Element::NUMERIC) {
				base->insert(element);
			}
			else if (type == Element::STRING) {
				throw ErrorException(ErrorException::ERROR_INTERNAL, "string element in fillBaseElements");
			}
			else if (type == Element::CONSOLIDATED) {
				const ElementsWeightType * children = dimension->getChildren(element);

				if (children->empty()) {
					return;
				}

				// loop over edges
				for (ElementsWeightType::const_iterator ce = children->begin();  ce != children->end();  ce++) {
					const pair<Element*, double>& child = *ce;
					Element * element = child.first;
					Element::ElementType type = element->getElementType();

					if (type == Element::CONSOLIDATED) {
						fillBaseElements(dimension, element, base);
					}
					else {
						base->insert(element);
					}
				}
			}
	}



	// internal return type
	struct AreaParameterType {
		set<Element*> elements;
		set<Element*> stringElements;
		set<Element*> baseElements;

		double factor;

		Dimension* dimension;
		size_t position;

		AreaParameterType (Dimension* dimension, size_t pos) 
			: factor(0), dimension(dimension), position(pos) {
		}
	};

	struct CompareAreaParameter : public std::binary_function<const AreaParameterType&, const AreaParameterType&, bool> {
		CompareAreaParameter () {
		}

		bool operator () (const AreaParameterType& left, const AreaParameterType& right) {
			return left.factor < right.factor;
		}
	};



	// compute list of numeric elements and corresponding base elements
	static AreaParameterType computeLeafNodes (Dimension* dimension, size_t pos, const IdentifiersType* ids, bool ignoreUnknownElements) {
		AreaParameterType param(dimension, pos);

		for (IdentifiersType::const_iterator iter = ids->begin();  iter != ids->end();  iter++) {
			Element * element = dimension->lookupElement(*iter);

			if (element == 0) {
				if (ignoreUnknownElements) {
					continue;
				}
				throw ErrorException(ErrorException::ERROR_INTERNAL, "unknown element identifier");
			}

			Element::ElementType type = element->getElementType();

			if (type == Element::NUMERIC) {
				param.elements.insert(element);
				param.baseElements.insert(element);
			}
			else if (type == Element::STRING) {
				param.stringElements.insert(element);
			}
			else if (type == Element::CONSOLIDATED) {
				if (dimension->isStringConsolidation(element)) {
					param.stringElements.insert(element);
				}
				else {
					param.elements.insert(element);
					fillBaseElements(dimension, element, &param.baseElements);
				}
			}
		}

		param.factor = double(param.baseElements.size()) / double(param.elements.size());

		return param;
	}



	// allow area to grow this factor
	static const double MAXIMAL_FACTOR = 1.5;

	// ignore grow factor up to this size;
	static const uint64_t MAXIMAL_SIZE = 200000;



	// fill mapping that contains the map from the base element to the request elements
	static void fillMapping (Dimension* dimension,
		map<Element*, ElementsWeightMap>& m,
		Element* request,
		Element* element,
		double factor = 1.0) {
			if (element->getElementType() == Element::NUMERIC) {
				m[element][request] += factor;
			}
			else /* Element::CONSOLIDATED */ {
				const ElementsWeightType * children = dimension->getChildren(element);

				if (children != 0) {
					for (ElementsWeightType::const_iterator childIter = children->begin();
						childIter != children->end();
						childIter++) {
							fillMapping(dimension, m, request, childIter->first, childIter->second * factor);
					}
				}
			}
	}



	// fill mapping that contains the map from the used elements to the requested elements
	static void fillReverseMapping (Dimension* dimension,
		map<Element*, ElementsWeightMap>& reverse,
		Element* request,
		Element* element,
		ElementsType& base,
		double factor = 1.0) {
			if (element->getElementType() == Element::NUMERIC) {
				if (find(base.begin(), base.end(), element) == base.end()) {
					throw ErrorException(ErrorException::ERROR_INTERNAL, "cannot find request elements");
				}

				reverse[element][request] += factor;
			}
			else /* Element::CONSOLIDATED */ {
				if (find(base.begin(), base.end(), element) != base.end()) {
					reverse[element][request] += factor;
				}
				else {
					const ElementsWeightType * children = dimension->getChildren(element);

					if (children != 0) {
						for (ElementsWeightType::const_iterator childIter = children->begin();
							childIter != children->end();
							childIter++) {
								fillReverseMapping(dimension, reverse, request, childIter->first, base, childIter->second * factor);
						}
					}
				}
			}
	}



	// compute area paramaters
	bool Cube::computeAreaParameters (Cube* cube,
		vector<IdentifiersType>* area,
		vector< map<Element*, uint32_t> >& hashMapping,
		vector<ElementsType>& numericArea,
		vector< vector< vector< pair<uint32_t, double> > > >& numericMapping,
		vector< map< uint32_t, vector< pair<IdentifierType, double> > > >& reverseNumericMapping,
		vector<uint32_t>& hashSteps,
		vector<uint32_t>& lengths,
		bool ignoreUnknownElements) {
			Statistics::Timer timer("Cube::computeAreaParameters");

			const vector<Dimension*> * dimensions = cube->getDimensions();

			vector<Dimension*>::const_iterator dimIter = dimensions->begin();

			uint64_t areaSize = 1;
			vector<AreaParameterType> variableDimensions;
			vector<ElementsType> requestElements(dimensions->size());

			size_t pos = 0;

			// compute a new area which possibly uses the base elements
			numericArea.clear();
			numericArea.resize(area->size());

			// check each dimension of the area
			for (vector<IdentifiersType>::iterator iter = area->begin();  iter != area->end();  iter++, dimIter++, pos++) {
				const IdentifiersType& elms = *iter;
				Dimension * dimension = *dimIter;

				// empty dimension, this should not happen - nothing to compute
				if (elms.empty()) {
					return false;
				}

				// fixed dimension
				else {
					AreaParameterType param = computeLeafNodes(dimension, pos, &elms, ignoreUnknownElements);

					areaSize *= (uint64_t) param.elements.size();

					numericArea[pos].clear();
					numericArea[pos].insert(numericArea[pos].begin(), param.elements.begin(), param.elements.end());

					// store request elements as we might decide to change the area for better performance!
					ElementsType& elms = requestElements[pos];
					elms.insert(elms.begin(), param.elements.begin(), param.elements.end());

					if (param.elements.size() > 1) {
						variableDimensions.push_back(param);
					}
				}
			}

			// check how much larger the area is - if we fix the variable dimensions
			double factor = 1.0;

			if (! variableDimensions.empty()) {
				CompareAreaParameter compare;
				sort(variableDimensions.begin(), variableDimensions.end(), compare);

				while (! variableDimensions.empty()) {
					AreaParameterType& param = variableDimensions[0];
					double f = factor * param.factor;

					if (f <= MAXIMAL_FACTOR || areaSize * f <= MAXIMAL_SIZE) {
						Logger::debug << "moving dimension " << param.dimension->getName() << " to fixed list" << endl;

						numericArea[param.position].clear();
						numericArea[param.position].insert(numericArea[param.position].begin(),
							param.baseElements.begin(), param.baseElements.end());

						variableDimensions.erase(variableDimensions.begin());
						factor = f;
					}
					else {
						break;
					}
				}
			}

			// now compute the hash keys for all used elements
			hashMapping.clear();
			hashMapping.resize(dimensions->size());

			hashSteps.clear();
			hashSteps.resize(dimensions->size());

			lengths.clear();
			lengths.resize(dimensions->size());

			uint32_t mult = 1;
			pos = 0;

			for (vector<ElementsType>::iterator iter = numericArea.begin();  iter != numericArea.end();  iter++, pos++) {
				const ElementsType& elms = *iter;
				map<Element*, uint32_t>& hashs = hashMapping[pos];

				// resize vector to fit maximal element identifier
				uint32_t count = 0;

				for (ElementsType::const_iterator i = elms.begin();  i != elms.end();  i++, count++) {
					hashs[*i] = count * mult;
				}

				// store steps and hash
				hashSteps[pos] = mult;
				lengths[pos] = elms.size();

				// update current multiplier
				mult *= elms.size();
			}

			// and now the map to the base elements
			vector< map<Element*, ElementsWeightMap> > mapping(dimensions->size());

			dimIter = dimensions->begin();
			pos = 0;

			for (vector<ElementsType>::iterator iter = numericArea.begin();  iter != numericArea.end();  iter++, dimIter++, pos++) {
				ElementsType& elms = *iter;
				Dimension* dimension = *dimIter;

				map<Element*, ElementsWeightMap>& m = mapping[pos];

				for (ElementsType::const_iterator i = elms.begin();  i != elms.end();  i++) {
					Element* element = *i;
					fillMapping(dimension, m, element, element);
				}
			}

			// plus the reverse (we might have moved to the base elements for better performance)
			vector< map<Element*, ElementsWeightMap> > reverseMapping(dimensions->size());

			vector<ElementsType>::iterator numIter = numericArea.begin();
			vector< map<Element*, ElementsWeightMap> >::iterator revIter = reverseMapping.begin();

			dimIter = dimensions->begin();

			for (vector<ElementsType>::iterator reqIter = requestElements.begin();
				reqIter != requestElements.end();
				reqIter++, numIter++, revIter++, dimIter++) {

					ElementsType& numList = *numIter;
					Dimension* dimension = *dimIter;
					map<Element*, ElementsWeightMap>& reverse = *revIter;

					for (ElementsType::iterator elmIter = reqIter->begin();  elmIter != reqIter->end();  elmIter++) {
						Element* elm = *elmIter;

						if (find(numList.begin(), numList.end(), elm) != numList.end()) {
							reverse[elm][elm] += 1;
						}
						else {
							fillReverseMapping(dimension, reverse, elm, elm, numList);
						}
					}
			}

			// construct a mapping for each dimension base element to request element hash and factor
			numericMapping.clear();
			numericMapping.resize(dimensions->size());

			// rewrite mapping for easier access
			pos = 0;
			dimIter = dimensions->begin();

			for (vector< map<Element*, ElementsWeightMap> >::iterator iter = mapping.begin(); 
				iter != mapping.end();
				iter++, dimIter++, pos++) {
					map<Element*, ElementsWeightMap>& m = *iter;
					Dimension* dimension = *dimIter;

					vector< vector< pair<uint32_t, double> > >& ps = numericMapping.at(pos);
					ps.resize(dimension->getMaximalIdentifier() + 1);

					for (map<Element*, ElementsWeightMap>::iterator j = m.begin();  j != m.end();  j++) {
						Element* element = j->first;
						vector< pair<uint32_t, double> >& pps = ps.at(element->getIdentifier());

						ElementsWeightMap& pairs = j->second;

						for (ElementsWeightMap::iterator k = pairs.begin();  k != pairs.end();  k++) {
							Element* b = k->first;
							uint32_t hash = hashMapping[pos][b];

							pair<uint32_t, double> np(hash, k->second);
							pps.push_back(np);
						}
					}
			}

			// construct a mapping for each numeric area element to the requested element and factor
			reverseNumericMapping.clear();
			reverseNumericMapping.resize(dimensions->size());

			// reqrite mapping for easier access
			pos = 0;
			dimIter = dimensions->begin();

			for (vector< map<Element*, ElementsWeightMap> >::iterator revIter = reverseMapping.begin();
				revIter != reverseMapping.end();
				revIter++, dimIter++, pos++) {
					map< uint32_t, vector< pair<IdentifierType, double> > >& reverseNumeric = reverseNumericMapping[pos];
					map<Element*, ElementsWeightMap>& reverse = *revIter;

					for (map<Element*, ElementsWeightMap>::iterator iter = reverse.begin();  iter != reverse.end();  iter++) {
						Element* element = iter->first;
						ElementsWeightMap& m = iter->second;
						uint32_t hash = hashMapping[pos][element];
						vector< pair<IdentifierType, double> >& reverseList = reverseNumeric[hash];

						for (ElementsWeightMap::iterator j = m.begin();  j != m.end();  j++) {
							pair<IdentifierType, double> x(j->first->getIdentifier(), j->second);
							reverseList.push_back(x);
						}
					}
			}


			// debug output
			if (false) {
				cout << "MAPPING\n";

				for (size_t i = 0;  i < numericMapping.size();  i++) {
					cout << "DIMENSION #" << i << endl;

					vector< vector< pair<uint32_t, double> > >& p = numericMapping[i];

					for (size_t j = 0;  j < p.size();  j++) {
						vector< pair<uint32_t, double> >& pp = p[j];

						if (! pp.empty()) {
							cout << "ELEMENT #" << j << " = ";

							for (size_t k = 0;  k < pp.size();  k++) {
								pair<uint32_t, double>& ppp = pp[k];

								cout << ppp.first << "/" << ppp.second << " ";
							}

							cout << "\n";
						}
					}
				}

				cout << "\n----------------------------------------" << endl;
				cout << "REVERSE MAPPING\n";

				dimIter = dimensions->begin();

				for (size_t i = 0;  i < reverseNumericMapping.size();  i++) {
					cout << "DIMENSION #" << i << "\n";

					map< uint32_t, vector< pair<IdentifierType, double> > >& reverse = reverseNumericMapping[i];

					for (map< uint32_t, vector< pair<IdentifierType, double> > >::iterator j = reverse.begin();  j != reverse.end();  j++) {
						uint32_t hash = j->first;
						vector< pair<IdentifierType, double> >& m = j->second;

						cout << "HASH " << hash << " = ";

						for (vector< pair<IdentifierType, double> >::iterator l = m.begin();  l != m.end();  l++) {
							pair<IdentifierType, double>& p = *l;

							cout << p.first << "/" << p.second << " ";
						}

						cout << endl;
					}
				}
			}

			return true;
	}



	// get cell value of enterprise rule from cache
	static inline bool findInRuleCache (CacheRulesStorage* cacheRulesStorage,
		CellPath* cellPath,
		Cube::CellValueType& cellValue,
		bool* found) {
			// find value in cache
			CubePage::value_t v = cacheRulesStorage->getCellValue(cellPath);

			if (v) {
				double value = * (double*) v;
				IdentifierType r = * (IdentifierType*) (v + sizeof(double));

				if (isnan(value)) {
					*found = false;
					cellValue.doubleValue = 0.0;
				}
				else {
					*found = true;
					cellValue.doubleValue = value;
				}

				cellValue.type = Element::NUMERIC;
				cellValue.rule = r;

				return true;
			}

			return false;
	}



	// get a direct match for an enterprise rule
	static inline bool findDirectRuleMatch (Database* database,
		vector<Rule*>& rules,
		CacheRulesStorage* cacheRulesStorage,
		CellPath* cellPath,
		User* user,
		Cube::CellValueType& cellValue,
		bool* found,
		bool* isCachable,
		set< pair<Rule*, IdentifiersType> >* ruleHistory) {

			for (vector<Rule*>::iterator iter = rules.begin();  iter != rules.end();  iter++) {
				Rule* rule = *iter;

				if (rule == 0 || !rule->isActive()) {
					continue;
				}

				if (rule->within(cellPath)) {
					if (rule->isRestrictedRule() && ! rule->withinRestricted(cellPath)) {
						Logger::trace << "encountered STET restriction" << endl;
						break;
					}

					const IdentifiersType * identifiers = cellPath->getPathIdentifier();
					pair<Rule*, IdentifiersType> exec = make_pair(rule, *identifiers);

					if (!ruleHistory) {
						Logger::trace << "history set is empty" << endl;
						return false;
					}
					else {
						set< pair<Rule*, IdentifiersType> >::iterator find = ruleHistory->find(exec);

						// recursion found
						if (find != ruleHistory->end()) {
							Logger::warning << "recursion in rule found" << endl;
							throw ErrorException(ErrorException::ERROR_RULE_HAS_CIRCULAR_REF,
								"cube=" + StringUtils::convertToString(cellPath->getCube()->getIdentifier())
								+ "&rule=" + StringUtils::convertToString(rule->getIdentifier()));
						}
					}

					ruleHistory->insert(exec);

					bool skipAllRules;
					bool skipRule;

					Cube::CellValueType value = rule->getValue( cellPath, user, &skipRule, &skipAllRules, isCachable, ruleHistory );

					ruleHistory->erase(exec);

					if ( skipAllRules )
						break;
					else if ( skipRule )
						continue; //try next rule       
					else {
						cellValue.type = value.type;
						cellValue.rule = value.rule;

						if (value.type == Element::NUMERIC) {
							cellValue.doubleValue = value.doubleValue;

							if (! rule->hasMarkers()) {
								if (cacheRulesStorage != 0 && *isCachable) {
									uint8_t buffer[sizeof(double) + sizeof(IdentifierType)];

									memcpy(buffer,                  (uint8_t*) &value.doubleValue, sizeof(double));
									memcpy(buffer + sizeof(double), (uint8_t*) &value.rule, sizeof(IdentifierType));

									cacheRulesStorage->setCellValue(cellPath, buffer);

									database->setHasCachedRules();
								}
							}
						}
						else {
							cellValue.charValue.assign(value.charValue);
						}

						*found = true;

						return true;
					}
				}
			}

			return false;
	}



	// get a direct match for an enterprise rule
	static inline bool findIndirectRuleMatch (Database* database,
		Cube* cube,
		vector<Rule*>& rules,
		CacheRulesStorage* cacheRulesStorage,
		CellPath* cellPath,
		Cube::CellValueType& cellValue,
		bool* found,
		bool* isCachable,
		set< pair<Rule*, IdentifiersType> >* ruleHistory) {

			// We split the rules into the following categories:
			// - linear rules applicable to the path
			// - all other rules applicable to the path
			// - all applicable rules in the correct order
			// Keep in mind, that we already dealt with a direct matches. We will not try
			// again here. A direct match must have resulted in a STET because we reached
			// this function.

			int normalRules = 0;
			int linearRules = 0;
			int allRules = 0;
			int restrictedRules = 0;
			int markerRules = 0;

			Rule* linearRule = 0;

			for (vector<Rule*>::iterator iter = rules.begin();  iter != rules.end();  iter++) {
				Rule* rule = *iter;

				if (rule == 0 || !rule->isActive()) {
					continue;
				}


				// no need to check a direct match again
				if (!rule->within(cellPath) && rule->contains(cellPath)) {
					if (rule->getRuleOption() == RuleNode::BASE
						&& rule->isLinearRule()
						&& cellPath->getPathType() == Element::CONSOLIDATED
						&& rule->withinArea(cellPath)) {
							linearRules++;
							linearRule = rule;
					}
					else {
						normalRules++;
					}

					allRules++;

					if (rule->isRestrictedRule() && ! rule->containsRestricted(cellPath)) {
						restrictedRules++;
					}

					if (rule->hasMarkers()) {
						markerRules++;
					}
				}
			}


			if (DEBUG_FILE) {
				Logger::trace << "found " 
					<< linearRules << " linear rules, "
					<< normalRules << " non-linear rules, "
					<< restrictedRules << " restricted rules"
					<< markerRules << " marker rules"
					<< endl;
			}

			// no match at all, then return
			if (allRules == 0 || markerRules > 0) {
				return false;
			}

			// use linear evaluation
			bool useLinear = false;

			// if we have only one match of a linear rule, try to use it
			if (normalRules == 0 && linearRules == 1) {
				useLinear = true;
			}

			// if we have only one match and this is outside of a restriction, stop
			if (allRules == 1 && restrictedRules == 1) {
				Logger::trace << "encountered STET restriction in consolidation" << endl;
				return false;
			}

			// compute value
			Cube::CellValueType value;

			if (useLinear) {
				Logger::debug << "using linear rule to optimize access" << endl;

				bool skipAllRules;
				bool skipRule;
				value = linearRule->getValue( cellPath, 0, &skipRule, &skipAllRules, isCachable, ruleHistory );

				cellValue.type = value.type;
				cellValue.rule = Rule::NO_RULE;

				*found = true;
			}
			else {
				value = cube->getConsolidatedRuleValue(cellPath, found, isCachable, ruleHistory);

				cellValue.type = value.type;
				cellValue.rule = Rule::NO_RULE;
			}

			// cache numeric values
			if (cellValue.type == Element::NUMERIC) {
				cellValue.doubleValue = value.doubleValue;

				if (cacheRulesStorage != 0 && *isCachable) {
					uint8_t buffer[sizeof(double) + sizeof(IdentifierType)];

					memcpy(buffer,                  (uint8_t*) &value.doubleValue, sizeof(double));
					memcpy(buffer + sizeof(double), (uint8_t*) &value.rule, sizeof(IdentifierType));

					cacheRulesStorage->setCellValue(cellPath, buffer);

					database->setHasCachedRules();
				}
			}
			else {
				cellValue.charValue.assign(value.charValue);
			}

			return true;
	}


	////////////////////////////////////////////////////////////////////////////////
	// constructors and destructors
	////////////////////////////////////////////////////////////////////////////////

	Cube::Cube (IdentifierType identifier,
		const string& name,
		Database * database,
		vector<Dimension*> * dimensions)
		: token(rand()),
		clientCacheToken(rand()),
		identifier(identifier),
		name(name),
		dimensions(*dimensions),
		sizeDimensions(dimensions->size(), 0),
		database(database),
		deletable(true),
		renamable(true),
		fileName(0),
		ruleFileName(0),
		journal(0),
		maxRuleIdentifier(0) {

			if (dimensions->empty()) {
				throw ParameterException(ErrorException::ERROR_CUBE_EMPTY,
					"missing dimensions",
					"dimensions", "");
			}

			transform(dimensions->begin(),
				dimensions->end(),
				sizeDimensions.begin(),
				mem_fun(&Dimension::sizeElements));

			storageDouble = new CubeStorage(this, &sizeDimensions, sizeof(double), false);
			storageString = new CubeStorage(this, &sizeDimensions, sizeof(char*),  true);

			// create cache storages in normal cubes only    
			cacheConsolidationsStorage = 0;
			cacheRulesStorage = 0;    

			status = CHANGED;

			hasArea = false;
			cubeWorker = 0;
			invalidateCacheCounter = 0;

			hasLock = false;
			maxLockId = 0;    
	}



	Cube::~Cube () {
		delete storageDouble;
		delete storageString;
		if (cacheConsolidationsStorage) {
			delete cacheConsolidationsStorage;
		}
		if (cacheRulesStorage) {
			delete cacheRulesStorage;
		}

		if (journal != 0) { 
			delete journal;
		}

		if (fileName != 0) {
			delete fileName;
		}

		if (ruleFileName != 0) {
			delete ruleFileName;
		}

		if (cubeWorker) {
			delete cubeWorker;
		}

		for (vector<Rule*>::iterator iter = rules.begin();  iter != rules.end();  iter++) {
			delete *iter;
		}    

		Server::removeChangedMarkerCube(this);
	}  

	////////////////////////////////////////////////////////////////////////////////
	// functions to load and save a cube
	////////////////////////////////////////////////////////////////////////////////

	bool Cube::isLoadable () const {
		return fileName == 0 ? false : (FileUtils::isReadable(*fileName) || FileUtils::isReadable(FileName(*fileName, "tmp")));
	}



	Cube* Cube::loadCubeFromType (FileReader* file,
		Database* database,
		IdentifierType identifier,
		const string& name,
		vector<Dimension*>* dimensions,
		uint32_t type) {
			switch(type) {
	  case RightsCube::CUBE_TYPE:
		  return new RightsCube(identifier, name, database, dimensions);

	  case NormalCube::CUBE_TYPE:
		  return new NormalCube(identifier, name, database, dimensions);

	  case AttributesCube::CUBE_TYPE:
		  return new AttributesCube(identifier, name, database, dimensions);

	  case ConfigurationCube::CUBE_TYPE:
		  return new ConfigurationCube(identifier, name, database, dimensions);

	  case SubsetViewCube::CUBE_TYPE:
		  return new SubsetViewCube(identifier, name, database, dimensions);

	  case UserInfoCube::CUBE_TYPE:
		  return new UserInfoCube(identifier, name, database, dimensions);

	  default:
		  Logger::error << "cube '" << name << "' has unknown type '" <<  type << "'" << endl;
		  throw FileFormatException("unknown dimension type", file);
			}
	}



	void Cube::loadCubeOverview (FileReader* file) {
		vector<int> sizes;

		// last save time
		long seconds;
		long useconds;

		if (file->isSectionLine() && file->getSection() == "CUBE") {
			file->nextLine();

			while (file->isDataLine()) {
				file->getTimeStamp(&seconds, &useconds, 0);

				sizes = file->getDataIntegers(1);

				file->nextLine();
			}
		}
		else {
			throw FileFormatException("Section 'CUBE' not found.", file);
		}

		sizeDimensions.clear();
		sizeDimensions.assign(sizes.begin(), sizes.end());

		if (sizeDimensions.size() != dimensions.size()) {
			Logger::error << "size of dimension list and size of size list is not equal" << endl;
			throw FileFormatException("list of dimension is corrupted", file);
		}  
	}



	//static const int PROGRESS_COUNT = 500 * 1000;
	static const int TIME_BASED_PROGRESS_COUNT = 10;



	void Cube::loadCubeCells (FileReader* file) {

		// remove old storage objects
		delete storageDouble;
		delete storageString;

		// create new storage objects
		storageDouble = new CubeStorage(this, &sizeDimensions, sizeof(double), false);
		storageString = new CubeStorage(this, &sizeDimensions, sizeof(char*), true);

		if (cacheConsolidationsStorage) {      
			delete cacheConsolidationsStorage;
			cacheConsolidationsStorage  = new CacheConsolidationsStorage(&sizeDimensions, sizeof(double));    

			cachedAreas.clear();
			cachedCellValues.clear();
		}
		invalidateCacheCounter = 0;

		if (cacheRulesStorage) {      
			delete cacheRulesStorage;
			cacheRulesStorage  = new CacheRulesStorage(&sizeDimensions, sizeof(double) + sizeof(IdentifierType));
		}

		size_t size = dimensions.size();
		size_t count = 0;

		if (file->isSectionLine() && file->getSection() == "NUMERIC") {
			file->nextLine();

			while (file->isDataLine()) {
				IdentifiersType ids = file->getDataIdentifiers(0);
				double d = file->getDataDouble(1);

				if (size != ids.size()) {
					Logger::error << "error in numeric cell path of cube '" << name << "'" << endl;
					throw FileFormatException("error in numeric cell path", file);
				}

				bool failed = false;

				for (uint32_t i = 0;  i < size;  i++) {
					if (dimensions[i]->lookupElement(ids[i]) == 0) {
						Logger::warning << "error in numeric cell path of cube '" << name << "', skipping entry " 
							<< ids[i] << " in dimension '" << dimensions[i]->getName() << endl;
						failed = true;
						break;
					}
				}

				if (! failed) {
					setBaseCellValue(&ids, d);
				}

				if ((++count % TIME_BASED_PROGRESS_COUNT) == 0) {
					database->getServer()->timeBasedProgress();
				}

				file->nextLine();
			}
		}
		else {
			throw FileFormatException("section 'NUMERIC' not found", file);
		}

		if (file->isSectionLine() && file->getSection() == "STRING") {
			file->nextLine();

			while (file->isDataLine()) {
				IdentifiersType ids = file->getDataIdentifiers(0);
				string s = file->getDataString(1);

				if (size != ids.size()) {
					Logger::error << "error in string cell path of cube '" << name<< "'" << endl;
					throw FileFormatException("error in string cell path", file);
				}

				bool failed = false;

				for (uint32_t i = 0;  i < size;  i++) {
					if (dimensions[i]->lookupElement(ids[i]) == 0) {
						Logger::warning << "error in numeric cell path of cube '" << name << "', skipping entry " 
							<< ids[i] << " in dimension '" << dimensions[i]->getName() << endl;
						failed = true;
						break;
					}
				}

				if (! failed) {
					setBaseCellValue(&ids, s);
				}

				if ((++count % TIME_BASED_PROGRESS_COUNT) == 0) {
					database->getServer()->timeBasedProgress();
				}

				file->nextLine();
			}
		}
		else {
			throw FileFormatException("section 'STRING' not found", file);
		}
	}



	bool Cube::loadCubeJournal () {
		bool changed = false;

		{
			JournalFileReader history(FileName(*fileName, "log"));

			try {
				history.openFile();
			}
			catch (FileOpenException fe) {
				return changed;
			}

			Logger::trace << "scaning log file for cube '" << name << "'" << endl;

			while (history.isDataLine()) {
				string username = history.getDataString(1);
				string event = history.getDataString(2);
				string command = history.getDataString(3);

				if (command == "DELETE_ELEMENT") {
					IdentifierType idDimension = history.getDataInteger(4);
					IdentifierType idElement   = history.getDataInteger(5);
					bool processStorageDouble  = history.getDataBool(6);
					bool processStorageString  = history.getDataBool(7);
					bool deleteRule            = history.getDataBool(8);

					Dimension * dimension = 0;

					for (vector<Dimension*>::const_iterator i = dimensions.begin();  i != dimensions.end();  i++) {
						if ((*i)->getIdentifier() == idDimension) {
							dimension = *i;
							break;
						}
					}

					if (dimension != 0) {
						deleteElement(username,
							event,
							database->findDimension(idDimension, 0),
							idElement,
							processStorageDouble,
							processStorageString,
							deleteRule);
					}
					else {
						Logger::info << "journal file: dimension identifier '" << idDimension 
							<< "' not found. Ignoring command 'DELETE_ELEMENT' on element '"
							<< idElement << "'"
							<< endl;
					}

					changed = true;
				}
				else if (command == "SET_DOUBLE") {
					IdentifiersType ids = history.getDataIdentifiers(4);
					SplashMode splashMode = (SplashMode) history.getDataInteger(5);
					double value = history.getDataDouble(6);

					try {
						CellPath cellPath(this, &ids);

						setCellValue(&cellPath, value, 0, 0, false, false, false, splashMode, 0);
						changed = true;
					}
					catch (ErrorException e) {
						// Logger::warning << e.getMessage() << endl;         
					}
				}
				else if (command == "SET_STRING") {
					IdentifiersType ids = history.getDataIdentifiers(4);
					string value = history.getDataString(5);

					try {
						CellPath cellPath(this, &ids);

						setCellValue(&cellPath, value, 0, 0, false, false, 0);
						changed = true;
					}
					catch (ErrorException e) {
						// Logger::warning << e.getMessage() << endl;         
					}

				}
				else if (command == "CLEAR_CELL") {
					IdentifiersType ids = history.getDataIdentifiers(4);
					try {
						CellPath cellPath(this, &ids);

						clearCellValue(&cellPath, 0, 0, false, false, 0);
						changed = true;
					}
					catch (ErrorException e) {
						// Logger::warning << e.getMessage() << endl;         
					}
				}
				else if (command == "CLEAR_CELLS") {
					clearCells(0);
					changed = true;
				} 
				else if (command == "COPY_VALUES") {
					IdentifiersType from = history.getDataIdentifiers(4);
					IdentifiersType to = history.getDataIdentifiers(5);
					double factor = history.getDataDouble(6);					

					try {
						CellPath fromPath(this, &from);
						CellPath toPath(this, &to);

						copyCellValues(&fromPath, &toPath, 0, 0, factor);
						changed = true;
					}
					catch (ErrorException e) {
						// Logger::warning << e.getMessage() << endl;         
					}
				}

				history.nextLine();
			}
		}

		return changed;
	}



	void Cube::loadCubeRuleInfo (FileReader* file) {
		maxRuleIdentifier = file->getDataInteger(0);
	}



	void Cube::loadCubeRule (FileReader* file, int version) {
		IdentifierType id = file->getDataInteger(0);
		string definition = file->getDataString(1);
		string external = file->getDataString(2);
		string comment = file->getDataString(3);
		time_t timestamp = file->getDataInteger(4);
		bool isActive = file->getDataBool(5, true);

		if (id > maxRuleIdentifier) {
			maxRuleIdentifier = id;
		}

		if (version == 1) {
			replace(definition.begin(), definition.end(), '[', '{');
			replace(definition.begin(), definition.end(), ']', '}');
		}

		try {
			RuleParserDriver driver;

			driver.parse(definition);
			RuleNode* r = driver.getResult();

			if (r) {          
				string errorMsg;
				bool ok = r->validate(database->getServer(), database, this, errorMsg);

				if (!ok) {
					Logger::error << "cannot parse rule " << id << " in cube '" << name << "': " << errorMsg << endl;
					delete r;
					return;
				}

				Rule* rule = new Rule(id, this, r, external, comment, timestamp, isActive);
				rules.push_back(rule);

				// the marker areas will be updated later
				if (isActive && rule->hasMarkers()) {
					newMarkerRules.insert(rule);
				}
			}
			else {
				Logger::error << "cannot parse rule " << id << ": " << driver.getErrorMessage() << endl;
			}
		}
		catch (const ErrorException& ex) {
			Logger::error << "cannot parse rule " << id << ": " << ex.getMessage() << " (" << ex.getDetails() << ")" << endl;
		}
	}



	void Cube::loadCubeRules () {
		if (! FileUtils::isReadable(*ruleFileName)) {
			return;
		}

		FileReader fr(*ruleFileName);
		fr.openFile();
		int version = 1;

		if (fr.isSectionLine()) {

			if (fr.getSection() == "RULES INFO") {
				fr.nextLine();
				while (fr.isDataLine()) {
					loadCubeRuleInfo(&fr);
					fr.nextLine();
				}

			}

			if (fr.getSection() == "RULES2") {
				fr.nextLine();
				version = 2;
			}

		}

		while (fr.isDataLine()) {
			loadCubeRule(&fr, version);

			fr.nextLine();
		}
	}



	void Cube::loadCube (bool processJournal) {
		if (status == LOADED) {
			return;
		}

		if (fileName == 0) {
			throw ErrorException(ErrorException::ERROR_INTERNAL,
				"cube file name not set");
		}

		updateToken();
		updateClientCacheToken();

		if (!FileUtils::isReadable(*fileName) && FileUtils::isReadable(FileName(*fileName, "tmp"))) {
			Logger::warning << "using temp file for cube '" << name << "'" << endl;

			// delete journal
			JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"), false);

			// rename temp file
			FileUtils::rename(FileName(*fileName, "tmp"), *fileName);
		} 

		FileReader fr(*fileName);
		fr.openFile();

		// load overview
		database->getServer()->makeProgress();
		loadCubeOverview(&fr);

		// and cell values
		database->getServer()->makeProgress();

		if (! ignoreCellData && (getType() == NORMAL || getType() == USER_INFO)) {
			loadCubeCells(&fr);
		}

		if (processJournal) {
			database->getServer()->makeProgress();
			processCubeJournal(status);
		}

		// the cube is now loaded
		database->getServer()->makeProgress();
		status = LOADED;

		// we can now load the rules
		loadCubeRules();
	}




	void Cube::processCubeJournal (CubeStatus cubeStatus) {
		// close the journal
		bool journalOpen = false;

		if (journal != 0) {
			closeJournal();
			journalOpen = true;
		}

		try {

			// and read the journal
			if (cubeStatus == UNLOADED) {
				bool changed = loadCubeJournal();

				// force a write back to disk, this will archive the journal
				if (changed) {
					status = CHANGED;
					saveCube();
				}
			}

			// delete the journal as we have replaced the cube with the saved data
			else {
				JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"), false);
			}
		}
		catch (...) {
			if (journalOpen) {
				openJournal();
			}

			throw;
		}

		if (journalOpen) {
			openJournal();
		}

		// the cube is now loaded
		status = LOADED;
	}




	void Cube::saveCubeOverview (FileWriter* file) {
		file->appendComment("PALO CUBE DATA");
		file->appendComment("");

		file->appendComment("Description of data: ");
		file->appendComment("TIME_STAMP;SIZE_DIMENSIONS;");
		file->appendSection("CUBE");
		file->appendTimeStamp();    
	}



	void Cube::saveCubeDimensions (FileWriter* file) {
		vector<int32_t> sizes;
		sizes.assign(sizeDimensions.begin(), sizeDimensions.end());

		file->appendIntegers(&sizes);
		file->nextLine();
	}



	void Cube::saveCubeCells (FileWriter* file) {
		IdentifiersType path(dimensions.size());
		size_t size; // will be set in getIndexTable

		file->appendComment("Description of data: ");
		file->appendComment("PATH;VALUE ");
		file->appendSection("NUMERIC");

		Logger::trace<<"saving numeric cube cells..."<<endl;

		size_t count = 1;

		for (CubePage::element_t const * table = storageDouble->getArray(size);  0 < size;  table++) {
			if (*table != 0) {
				double * d = (double*) *table;

				if (*d != 0) {
					storageDouble->fillPath(*table, &path);

					file->appendIdentifiers(&path);
					file->appendDouble(*d);
					file->nextLine();
				}

				size--;
			}
			if ((++count % TIME_BASED_PROGRESS_COUNT) == 0) {
				database->getServer()->timeBasedProgress();
			}
		}

		Logger::trace<<"saving string cube cells..."<<endl;

		file->appendComment("Description of data: ");
		file->appendComment("PATH;VALUE ");
		file->appendSection("STRING");

		for (CubePage::element_t const * table = storageString->getArray(size);  0 < size;  table++) {
			if (*table != 0) {
				char ** c = (char**) *table;

				storageString->fillPath(*table, &path);

				file->appendIdentifiers(&path);
				file->appendEscapeString(*c);
				file->nextLine();

				size--;
			}
			if ((++count % TIME_BASED_PROGRESS_COUNT) == 0) {
				database->getServer()->timeBasedProgress();
			}
		}

		Logger::trace<<"cube cells saved!"<<endl;
	}


	void Cube::saveCube () {
		if (fileName == 0) {
			throw ErrorException(ErrorException::ERROR_INTERNAL,
				"cube file name not set");
		}

		database->getServer()->makeProgress();
		database->saveDatabase();

		if (status == LOADED) {
			return;
		}

		// open a new temp-cube file
		FileWriter fw(FileName(*fileName, "tmp"), false);
		fw.openFile();

		// save overview
		database->getServer()->makeProgress();
		saveCubeOverview(&fw);

		// and dimensions
		database->getServer()->makeProgress();
		saveCubeDimensions(&fw);

		// and cells
		database->getServer()->makeProgress();

		if (! ignoreCellData && (getType() == NORMAL || getType() == USER_INFO)) {
			saveCubeCells(&fw);
		}

		// that's it
		fw.appendComment("");
		fw.appendComment("PALO CUBE DATA END");

		fw.closeFile();

		// delete journal files
		bool journalOpen = false;

		if (journal != 0) {
			closeJournal();
			journalOpen = true;
		}

		// archive journal
		archiveJournalFiles(FileName(*fileName, "log"));
		// remove old cube file
		FileUtils::remove(*fileName);

		// delete journal
		JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"), false);

		// rename temp-cube file
		FileUtils::rename(FileName(*fileName, "tmp"), *fileName);

		// reopen journal
		if (journalOpen) {
			openJournal();
		}

		// the cube is now loaded
		database->getServer()->makeProgress();
		status = LOADED;
	}



	void Cube::saveCubeRule (FileWriter* file, Rule* rule) {
		file->appendIdentifier(rule->getIdentifier());

		StringBuffer sb;
		sb.initialize();
		rule->appendRepresentation(&sb);
		file->appendEscapeString(sb.c_str());
		sb.free();

		file->appendEscapeString(rule->getExternal());
		file->appendEscapeString(rule->getComment());
		file->appendInteger(rule->getTimeStamp());
		file->appendBool(rule->isActive());    
	}



	void Cube::saveCubeRules () {
		FileWriter fw(FileName(*ruleFileName, "tmp"), false);
		fw.openFile();

		fw.appendComment("PALO CUBE RULES");
		fw.appendSection("RULES INFO");
		fw.appendInteger(maxRuleIdentifier);    
		fw.nextLine();
		fw.appendSection("RULES2");

		for (vector<Rule*>::iterator iter = rules.begin();  iter != rules.end();  iter++) {
			saveCubeRule(&fw, *iter);
			fw.nextLine();
		}

		fw.appendComment("PALO CUBE RULES END");

		fw.closeFile();

		// rename temp-rule file
		FileUtils::remove(*ruleFileName);
		bool ok = FileUtils::rename(FileName(*ruleFileName, "tmp"), *ruleFileName);

		if (! ok) {
			Logger::error << "cannot rename rule file to '" << ruleFileName->fullPath() << "' " << strerror(errno) << endl;
			throw FileOpenException("cannot rename rule file", ruleFileName->fullPath());
		}
	}



	void Cube::setCubeFile (const FileName& fileName) {
		if (this->fileName != 0) {
			delete this->fileName;
			delete ruleFileName;
		}

		this->fileName = new FileName(fileName);
		this->ruleFileName = new FileName(fileName.path, fileName.name + "_rules", fileName.extension);

		if (isLoadable()) {
			status = UNLOADED;
		}
		else {
			saveCube();
			saveCubeRules();
		}

		closeJournal();
		openJournal();
	}



	void Cube::deleteCubeFiles () {

		// delete cube file from disk
		if (FileUtils::isReadable(*fileName)) {
			FileWriter::deleteFile(*fileName);
		}

		if (FileUtils::isReadable(*ruleFileName)) {
			FileWriter::deleteFile(*ruleFileName);
		}

		// delete journals 
		closeJournal();
		JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"));
	}



	void Cube::unloadCube () {
		if (!isLoadable()) {
			throw ParameterException(ErrorException::ERROR_CUBE_UNSAVED,
				"it is not possible to unload an unsaved cube, use delete for unsaved cubes",
				"", "");
		}

		if (status == UNLOADED) {
			return;
		}

		// save any outstanding changes
		saveCube();

		// update token
		updateToken();
		updateClientCacheToken();

		// reset all data:
		delete storageDouble;
		delete storageString;

		// create new storage objects
		storageDouble = new CubeStorage(this, &sizeDimensions, sizeof(double), false);
		storageString = new CubeStorage(this, &sizeDimensions, sizeof(char*),  true);

		if (cacheConsolidationsStorage) {      
			delete cacheConsolidationsStorage;
			cacheConsolidationsStorage  = new CacheConsolidationsStorage(&sizeDimensions, sizeof(double));    

			cachedAreas.clear();
			cachedCellValues.clear();
		}
		invalidateCacheCounter = 0;

		if (cacheRulesStorage) {      
			delete cacheRulesStorage;
			cacheRulesStorage = new CacheRulesStorage(&sizeDimensions, sizeof(double) + sizeof(IdentifierType));
		}

		// cube is now unloaded
		status = UNLOADED;
	}

	////////////////////////////////////////////////////////////////////////////////
	// getter and setter
	////////////////////////////////////////////////////////////////////////////////

	vector<Rule*> Cube::getRules (User* user) {
		checkCubeRuleRight(user, RIGHT_READ);

		vector<Rule*> result;

		for (vector<Rule*>::const_iterator iter = rules.begin();  iter != rules.end();  iter++) {
			if (*iter != 0) {
				result.push_back(*iter);
			}
		}

		return result;
	}



	uint32_t Cube::getToken () const {
		// return database token in order to get changes on a dimension
		// TODO this could be refined
		return database->getToken();
	}



	uint32_t Cube::getClientCacheToken () {
		switch (database->getClientCacheType()) {
	  case ConfigurationCube::CACHE_ALL  : 
		  return clientCacheToken;
	  case ConfigurationCube::NO_CACHE_WITH_RULES :
		  if (rules.empty()) {
			  return clientCacheToken;
		  }
	  default : 
		  return ++clientCacheToken;
		}    
	}



	int32_t Cube::sizeFilledCells () {
		return (int32_t)(storageString->size() + storageDouble->size());
	}

	////////////////////////////////////////////////////////////////////////////////
	// functions setting and clearing cells
	////////////////////////////////////////////////////////////////////////////////

	void Cube::clearCells (User* user) {
		if (getType() == USER_INFO) {
			checkCubeAccessRight(user, RIGHT_WRITE);
		}
		else {
			checkCubeAccessRight(user, RIGHT_DELETE);
		}

		// check for locks
		if (hasLock) {
			throw ErrorException(ErrorException::ERROR_CUBE_BLOCKED_BY_LOCK, "cannot clear cells because of a locked area");                
		}

		// we need to recalculate the markers
		Server::addChangedMarkerCube(this);

		// remove old storage objects
		delete storageDouble;
		delete storageString;

		// create empty storage
		storageDouble = new CubeStorage(this, &sizeDimensions, sizeof(double), false);
		storageString = new CubeStorage(this, &sizeDimensions, sizeof(char*),  true);

		if (cacheConsolidationsStorage) {      
			delete cacheConsolidationsStorage;
			cacheConsolidationsStorage  = new CacheConsolidationsStorage(&sizeDimensions, sizeof(double));    

			cachedAreas.clear();
			cachedCellValues.clear();
		}
		invalidateCacheCounter = 0;

		if (cacheRulesStorage) {      
			delete cacheRulesStorage;
			cacheRulesStorage = new CacheRulesStorage(&sizeDimensions, sizeof(double) + sizeof(IdentifierType));
		}

		if (journal != 0) {
			journal->appendCommand(Server::getUsername(user),
				Server::getEvent(),
				"CLEAR_CELLS");
			journal->nextLine();
		}

		status = CHANGED;
		updateClientCacheToken();
	}



	void Cube::clearCells (vector<IdentifiersType> * baseElements,
		User * user) {
			if (baseElements->size() != dimensions.size()) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"wrong cube used for base elements");
			}

			if (user != 0) {
				RightsType minimumRight;

				if (getType() == USER_INFO) {
					minimumRight = RIGHT_WRITE;
				}
				else {
					minimumRight = RIGHT_DELETE;
				}


				// check role "cell data" right
				if (user->getRoleCellDataRight() < minimumRight) {
					throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
						"insufficient access rights for path",
						"user", (int) user->getIdentifier());
				}

				// check cube data right
				if (user->getCubeDataRight(database, this) < minimumRight) {
					throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
						"insufficient access rights for path",
						"user", (int) user->getIdentifier());
				}

				// check dimension data right for all dimensions
				vector<Dimension*>::iterator dimIter = dimensions.begin();
				vector<IdentifiersType>::iterator idsIter = baseElements->begin();

				for (; dimIter != dimensions.end(); dimIter++, idsIter++) {
					Dimension* dimension = *dimIter;
					const IdentifiersType& ids = *idsIter;

					for (IdentifiersType::const_iterator i = ids.begin();  i != ids.end();  i++) {
						IdentifierType id = *i;
						Element* element = dimension->findElement(id, 0);

						if (user->getDimensionDataRight(database, dimension, element) < minimumRight) {
							throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
								"insufficient access rights for path",
								"user", (int) user->getIdentifier());
						}
					}
				}
			}

			// we need to recalculate the markers
			Server::addChangedMarkerCube(this);

			// get bases elements
			vector< set<IdentifierType> > baseElementsSet;
			baseElementsSet.resize(baseElements->size());

			double count = 1.0;

			vector< set<IdentifierType> >::iterator besIter = baseElementsSet.begin();
			vector< IdentifiersType >::const_iterator beIter = baseElements->begin();
			vector<Dimension*>::iterator dimIter = dimensions.begin();
			for (;  besIter != baseElementsSet.end();  besIter++, beIter++, dimIter++) {
				set<IdentifierType>& bs = *besIter;
				const IdentifiersType& b = *beIter;
				Dimension* dimension = *dimIter;

				for (IdentifiersType::const_iterator ci = b.begin(); ci != b.end(); ci++) {        
					IdentifierType id = *ci;
					Element* element = dimension->findElement(id, 0);

					const map<IdentifierType, double>* baseE = element->getBaseElements(dimension);
					map<IdentifierType, double>::const_iterator bi = baseE->begin();
					for (; bi != baseE->end(); bi++) {
						bs.insert(bi->first);
					}       
				}

				count *= bs.size();
			}

			Logger::trace << "going to clear " << count << " cells" << endl;


			if (hasLock) {
				throw ErrorException(ErrorException::ERROR_CUBE_BLOCKED_BY_LOCK, "cannot clear cells because of a locked area");                
			}

			// save values in storageDouble->removeCellValue()
			storageDouble->removeCellValue(&baseElementsSet, 0);

			//
			storageString->removeCellValue(&baseElementsSet, 0);

			// clear complete cache
			database->clearRuleCaches();

			if (cacheConsolidationsStorage) {
				cacheConsolidationsStorage->clear();

				cachedAreas.clear();
				cachedCellValues.clear();      
			}

			invalidateCacheCounter = 0;

			status = CHANGED;
			updateClientCacheToken();
	}




	semaphore_t Cube::clearCellValue (CellPath* cellPath,
		User* user,
		PaloSession * session,
		bool checkArea,
		bool sepRight,
		Lock* lock, 
		bool ignoreJournal) {
			Statistics::Timer timer("Cube::clearCellValue");

			// check cell path
			if (cellPath->getCube() != this) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"wrong cube used for cell path");
			}

			if (getType() == USER_INFO) {
				checkPathAccessRight(user, cellPath, RIGHT_WRITE);
			}
			else {
				checkPathAccessRight(user, cellPath, RIGHT_DELETE);
			}

			Element::ElementType elementType = cellPath->getPathType();

			string area;    

			// use SEP
			if (checkArea) {
				if (isInArea(cellPath, area)) {
					string sid = (session) ? session->getEncodedIdentifier() : "";

					return cubeWorker->executeSetCell(area, sid, cellPath->getPathIdentifier(), 0.0);
				}
			}

			// check that the user is allowed circumventing the SEP
			else if (sepRight) {
				checkSepRight(user, RIGHT_DELETE);
			}


			// delete numeric element
			if (elementType == Element::NUMERIC) {

				if (lock == Lock::checkLock) {
					lock = lookupLockedArea(cellPath, user);

					if (lock) {

						// check capacity of rollback storage        
						if (!lock->getStorage()->hasCapacity(1.0)) {
							throw ErrorException(ErrorException::ERROR_CUBE_LOCK_NO_CAPACITY,
								"rollback size is exceeded");
						}

						// add new step to rollback storage
						lock->getStorage()->addRollbackStep();
					}        
				}

				if (lock) {

					// get old cell value
					double * old = (double*) storageDouble->getCellValue(cellPath);

					if (old) {
						// save old value
						lock->getStorage()->addCellValue(cellPath->getPathIdentifier(), old);

						storageDouble->deleteCell(cellPath);
						removeCachedCells(cellPath);
					}

					// this might change the markers
					if (! fromMarkers.empty()) {
						Server::addChangedMarkerCube(this);
					}
				}
				else {
					bool found = storageDouble->deleteCell(cellPath);
					removeCachedCells(cellPath);

					// this might change the markers
					if (found) {
						Server::addChangedMarkerCube(this);
					}
				}
			}

			// delete string element
			else if (elementType == Element::STRING) {
				if (lock == Lock::checkLock) {
					lock = lookupLockedArea(cellPath, user);

					if (lock) {
						// check capacity of rollback storage        
						if (!lock->getStorage()->hasCapacity(1.0)) {
							throw ErrorException(ErrorException::ERROR_CUBE_LOCK_NO_CAPACITY,
								"rollback size is exceeded");
						}

						// add new step to rollback storage
						lock->getStorage()->addRollbackStep();
					}        
				}

				if (lock) {
					// get old cell value
					char * * old = (char * *) storageString->getCellValue(cellPath);

					if (old) {
						// save old value
						lock->getStorage()->addCellValue(cellPath->getPathIdentifier(), old);

						storageString->deleteCell(cellPath);
						removeCachedCells(cellPath);
					}
				}
				else {
					storageString->deleteCell(cellPath);
					removeCachedCells(cellPath);
				}
			}

			// delete aggregation
			else if (elementType == Element::CONSOLIDATED) {

				// check splash right
				if (user && user->getRoleCellDataRight() < RIGHT_SPLASH) {
					throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
						"insufficient access rights for path",
						"user", (int) user->getIdentifier());
				}

				// we need to recalculate the markers
				Server::addChangedMarkerCube(this);

				// get bases elements
				vector< set<IdentifierType> > baseElements;
				double count = computeBaseElements(cellPath, &baseElements);

				if (lock == Lock::checkLock) {
					lock = lookupLockedArea(cellPath, user);

					if (lock) {

						// check capacity of rollback storage        
						if (!lock->getStorage()->hasCapacity(count)) {
							throw ErrorException(ErrorException::ERROR_CUBE_LOCK_NO_CAPACITY,
								"rollback size is exceeded");
						}        

						// add new step to rollback storage
						lock->getStorage()->addRollbackStep();        
					}
				}

				// save values in storageDouble->removeCellValue()
				storageDouble->removeCellValue(&baseElements, lock);
				removeCachedCells(cellPath, &baseElements);
			}

			// wrong element type
			else {
				throw ParameterException(ErrorException::ERROR_INVALID_ELEMENT_TYPE,
					"wrong element type", 
					"elementType", elementType);
			}

			if (!ignoreJournal && journal != 0) {
				journal->appendCommand(Server::getUsername(user),
					Server::getEvent(),
					"CLEAR_CELL");
				journal->appendIdentifiers(cellPath->getPathIdentifier());
				journal->nextLine();
			}

			status = CHANGED;
			updateClientCacheToken();

			return 0;
	}



	void Cube::copyCellValues (CellPath* cellPathFrom, 
		CellPath* cellPathTo,
		User* user,
		PaloSession* session,
		double factor) {

			// check cell path
			if (cellPathFrom->getCube() != this || cellPathTo->getCube() != this) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"wrong cube used for cell path");
			}

			if (cellPathFrom->getPathType() == Element::STRING) {
				throw ParameterException(ErrorException::ERROR_INVALID_ELEMENT_TYPE,
					"cannot copy from string path",
					"source element type", cellPathFrom->getPathType());
			}

			if (cellPathTo->getPathType() == Element::STRING) {
				throw ParameterException(ErrorException::ERROR_INVALID_ELEMENT_TYPE,
					"cannot copy to string path",
					"destination element type", cellPathTo->getPathType());
			}

			// check minimal access rights, we might need splashing - check is done later
			checkPathAccessRight(user, cellPathFrom, RIGHT_READ);
			checkPathAccessRight(user, cellPathTo, RIGHT_WRITE);

			Lock* lock = lookupLockedArea(cellPathTo, user);

			// get compatible elements (with type and weight)
			/*vector< vector<Element*> > baseElementsFrom;
			vector< vector<Element*> > baseElementsTo;

			bool splash = computeCompatibleElements_m2(cellPathFrom, &baseElementsFrom,
			cellPathTo, &baseElementsTo);*/


			vector< vector<CopyElementInfo> > baseElementsFrom;
			vector< vector<CopyElementInfo> > baseElementsTo;

			bool splash = computeCompatibleElements_m3(cellPathFrom, baseElementsFrom,
				cellPathTo, baseElementsTo);

			//check splash rights
			if (getType() != USER_INFO && splash && user && (user->getRoleCellDataRight() < RIGHT_SPLASH)) 
				throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
				"insufficient access rights for path",
				"user", (int) user->getIdentifier());
#if 0
			cout<<"copyCellValues...";
#endif
			copyCellValues(&baseElementsFrom, &baseElementsTo, user, session, factor, lock);

			//check if destination cell value matches source cell value and splash value if neccessary.
			bool found;
			set< pair<Rule*, IdentifiersType> > ruleHistory;

			double fromValue = factor * this->getCellValue(cellPathFrom, &found, user, session, &ruleHistory).doubleValue;
			if (found) {
				ruleHistory.clear();
				double toValue = this->getCellValue(cellPathTo, &found, user, session, &ruleHistory).doubleValue;                 

				if (found && toValue!=fromValue) {                                        
					this->setCellValue(cellPathTo, fromValue, user, session, false, false, false, Cube::DEFAULT, lock, true);
				}
			}

#if 0
			cout<<"done"<<endl;
#endif

			if (journal!=0) {
				journal->appendCommand(Server::getUsername(user),
					Server::getEvent(),
					"COPY_VALUES");
				journal->appendIdentifiers(cellPathFrom->getPathIdentifier());
				journal->appendIdentifiers(cellPathTo->getPathIdentifier());
				journal->appendDouble(factor);
				journal->nextLine();
			}

			removeCachedCells(cellPathTo);
			status = CHANGED;
			updateClientCacheToken();

			// we need to recalculate the markers
			Server::addChangedMarkerCube(this);
	}



	bool Cube::computeCompatibleElements (CellPath* cellPathFrom,
		vector< vector<Element*> >* baseElementsFrom,
		CellPath* cellPathTo,
		vector< vector<Element*> >* baseElementsTo) {
			const PathType* from = cellPathFrom->getPathElements();
			const PathType* to = cellPathTo->getPathElements();
			bool splash = false;
			bool disjunct = false;
			vector<Dimension*>::iterator dimIter = dimensions.begin();

			for (PathType::const_iterator f = from->begin(), t = to->begin();
				f != from->end();
				f++, t++, dimIter++) {
					Dimension * dimension = *dimIter;

					vector<Element*> elementsFrom;
					vector<Element*> elementsTo;
					bool multiple = false;

					set<Element*> setTo = dimension->getBaseElements(*t, &multiple);

					bool compatible = computeCompatibleElements(dimension,
						*f, &elementsFrom,
						*t, &elementsTo);
					if (!compatible) {
						splash = true;

						if (multiple) {
							elementsFrom.clear();
							elementsTo.clear();

							elementsFrom.push_back(*f);
							elementsTo.push_back(*t);
						}
					}          

					if (! disjunct) {
						set<Element*> intersection;
						set<Element*> setFrom = dimension->getBaseElements(*f, 0);

						set_intersection(setFrom.begin(), setFrom.end(),
							setTo.begin(), setTo.end(),
							insert_iterator< set<Element*> >(intersection, intersection.begin()));

						if (intersection.empty()) {
							disjunct = true;
						}
					}

					baseElementsFrom->push_back(elementsFrom);
					baseElementsTo->push_back(elementsTo);
			}

			if (! disjunct) {
				throw ParameterException(ErrorException::ERROR_ELEMENT_CIRCULAR_REFERENCE,
					"source and destination paths are not disjunct",
					"source", "...");
			}

			return splash;
	}

	bool Cube::buildTreeWithoutMultipleParents(Dimension* dimension, Element* el, map<Element*, ElementsWeightType>* tree, bool* multiParent) {

		const ElementsWeightType * children = dimension->getChildren(el);

		ElementsWeightType nonMultipleParentChildren;

		bool multiparent = false;

		for (ElementsWeightType::const_iterator f = children->begin(); f!=children->end(); f++)
		{
			multiparent |= tree->find(f->first)!=tree->end();
			if (buildTreeWithoutMultipleParents(dimension, f->first, tree, multiParent))
				nonMultipleParentChildren.push_back(*f); //add child                  
		}

		tree->insert(make_pair(el, nonMultipleParentChildren));

		*multiParent |= multiparent;

		return !multiparent;
	}

	bool Cube::computeCompatibleElements_m2 (
		CellPath* cellPathFrom,
		vector< vector<Element*> >* baseElementsFrom,               
		CellPath* cellPathTo,
		vector< vector<Element*> >* baseElementsTo          
		) {
			const PathType* from = cellPathFrom->getPathElements();
			const PathType* to = cellPathTo->getPathElements();
			bool splash = false;
			bool disjunct = false;
			vector<Dimension*>::iterator dimIter = dimensions.begin();

			for (PathType::const_iterator f = from->begin(), t = to->begin();
				f != from->end();
				f++, t++, dimIter++) {
					Dimension * dimension = *dimIter;

					bool multiParentFrom = false;
					bool multiParentTo = false;

					//build dimension trees without multiple parents
					map<Element*, ElementsWeightType> fromElementsTree; 
					buildTreeWithoutMultipleParents(dimension, *f, &fromElementsTree, &multiParentFrom);
					map<Element*, ElementsWeightType> toElementsTree; 
					buildTreeWithoutMultipleParents(dimension, *t, &toElementsTree, &multiParentTo);

					splash |= multiParentTo;

					vector<Element*> elementsFrom;
					vector<Element*> elementsTo;
					bool multiple = false;

					set<Element*> setTo = dimension->getBaseElements(*t, &multiple);

					bool compatible = computeCompatibleElements_m2(dimension,
						*f, &fromElementsTree, &elementsFrom,
						*t, &toElementsTree, &elementsTo);

					if (!compatible) {
						splash = true;

						if (multiple) {
							elementsFrom.clear();
							elementsTo.clear();

							elementsFrom.push_back(*f);
							elementsTo.push_back(*t);
						}
					}          

					if (! disjunct) {
						set<Element*> intersection;
						set<Element*> setFrom = dimension->getBaseElements(*f, 0);

						set_intersection(setFrom.begin(), setFrom.end(),
							setTo.begin(), setTo.end(),
							insert_iterator< set<Element*> >(intersection, intersection.begin()));

						if (intersection.empty()) {
							disjunct = true;
						}
					}

					baseElementsFrom->push_back(elementsFrom);
					baseElementsTo->push_back(elementsTo);
			}

			if (! disjunct) {
				throw ParameterException(ErrorException::ERROR_ELEMENT_CIRCULAR_REFERENCE,
					"source and destination paths are not disjunct",
					"source", "...");
			}

			return splash;
	}	

	bool Cube::computeCompatibleElements_m3 (CellPath* cellPathFrom,
		vector< vector<CopyElementInfo> >& baseElementsFrom,
		CellPath* cellPathTo,
		vector< vector<CopyElementInfo> >& baseElementsTo) 
	{
		const PathType* from = cellPathFrom->getPathElements();
		const PathType* to = cellPathTo->getPathElements();
		bool splash = false;
		bool disjunct = false;
		vector<Dimension*>::iterator dimIter = dimensions.begin();

		for (PathType::const_iterator f = from->begin(), t = to->begin();
			f != from->end();
			f++, t++, dimIter++) {
				Dimension * dimension = *dimIter;
#if 0
				std::cout<<"Dimension: "<<dimension->getName()<<"... ";
#endif

				vector<CopyElementInfo> elementsFrom;
				vector<CopyElementInfo> elementsTo;
				bool multiple = false;

				set<Element*> setTo = dimension->getBaseElements(*t, &multiple);

				set<Element*> accessedFromElements; accessedFromElements.insert(*f);
				set<Element*> accessedToElements; accessedToElements.insert(*t);

				bool compatible = computeCompatibleElements_m3(dimension,
					*f, accessedFromElements, elementsFrom,
					*t, accessedToElements, elementsTo);

#if 0
				cout<<accessedFromElements.size()<<" elements."<<endl;
#endif
				if (!compatible) {
					splash = true;

					if (multiple) {
						elementsFrom.clear();
						elementsTo.clear();

						elementsFrom.push_back(CopyElementInfo(*f, false));
						elementsTo.push_back(CopyElementInfo(*t, false));
					}
				}          

				if (! disjunct) {
					set<Element*> intersection;
					set<Element*> setFrom = dimension->getBaseElements(*f, 0);

					set_intersection(setFrom.begin(), setFrom.end(),
						setTo.begin(), setTo.end(),
						insert_iterator< set<Element*> >(intersection, intersection.begin()));

					if (intersection.empty()) {
						disjunct = true;
					}
				}

				baseElementsFrom.push_back(elementsFrom);
				baseElementsTo.push_back(elementsTo);
		}

		if (! disjunct) {
			throw ParameterException(ErrorException::ERROR_ELEMENT_CIRCULAR_REFERENCE,
				"source and destination paths are not disjunct",
				"source", "...");
		}

		return splash;

	}

	bool Cube::computeCompatibleElements(Dimension* dimension,
		Element* from, vector<Element*>* fromElements,           
		Element* to, vector<Element*>* toElements) 
	{

		if (from->getElementType() == Element::NUMERIC
			|| to->getElementType() == Element::NUMERIC) {
				fromElements->push_back(from);
				toElements->push_back(to);
				return true;
		}
		else if (from->getElementType() == Element::STRING
			|| to->getElementType() == Element::STRING) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"source paths are not supported");
		}
		else if (from->getElementType() == Element::CONSOLIDATED
			&& to->getElementType() == Element::CONSOLIDATED) {
				const ElementsWeightType * fromChildren = dimension->getChildren(from);
				const ElementsWeightType * toChildren = dimension->getChildren(to);
				bool equal = true;

				if (equal && fromChildren->size() != toChildren->size()) {
					equal = false;
				}

				for (ElementsWeightType::const_iterator f = fromChildren->begin(), t = toChildren->begin();
					f != fromChildren->end() && equal;
					f++, t++) {
						if (f->second != t->second) {
							equal = false;
						}
				}

				if (equal) {
					for (ElementsWeightType::const_iterator f = fromChildren->begin(), t = toChildren->begin();
						f != fromChildren->end();
						f++, t++) {
							bool equalSub = computeCompatibleElements(dimension,
								f->first, fromElements,
								t->first, toElements);
							equal = equal && equalSub; 
					}
				}
				else {
					fromElements->push_back(from);
					toElements->push_back(to);
				}
				return equal;
		}
		else {
			throw ErrorException(ErrorException::ERROR_INTERNAL,
				"unknown element type");
		}

	}


	bool Cube::computeCompatibleElements_m2(Dimension* dimension,
		Element* from, const map<Element*, ElementsWeightType>* fromElementsTree, vector<Element*>* fromElements,
		Element* to, const map<Element*, ElementsWeightType>* toElementsTree, vector<Element*>* toElements) {

			if (from->getElementType() == Element::NUMERIC
				|| to->getElementType() == Element::NUMERIC) {
					fromElements->push_back(from);
					toElements->push_back(to);
					return to->getElementType() == Element::NUMERIC;
			}
			else if (from->getElementType() == Element::STRING
				|| to->getElementType() == Element::STRING) {
					throw ErrorException(ErrorException::ERROR_INTERNAL,
						"source paths are not supported");
			}
			else if (from->getElementType() == Element::CONSOLIDATED
				&& to->getElementType() == Element::CONSOLIDATED) {
					const ElementsWeightType * fromChildren = &fromElementsTree->find(from)->second;
					const ElementsWeightType * toChildren = &toElementsTree->find(to)->second;
					bool equal = true;

					if (equal && fromChildren->size() != toChildren->size()) {
						equal = false;
					}

					for (ElementsWeightType::const_iterator f = fromChildren->begin(), t = toChildren->begin();
						f != fromChildren->end() && equal;
						f++, t++) {
							if (f->second != t->second) {
								equal = false;
							}
					}

					if (equal) {
						for (ElementsWeightType::const_iterator f = fromChildren->begin(), t = toChildren->begin();
							f != fromChildren->end();
							f++, t++) {
								bool equalSub = computeCompatibleElements_m2(dimension,
									f->first, 
									fromElementsTree, 
									fromElements,
									t->first, 
									toElementsTree, 
									toElements);
								equal = equal && equalSub; 
						}
					}
					else {
						fromElements->push_back(from);
						toElements->push_back(to);
					}
					return equal;
			}
			else {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"unknown element type");
			}
	}	

	bool Cube::computeCompatibleElements_m3(Dimension* dimension,
		Element* from, 
		set<Element*>& accessedFromElements,
		vector<CopyElementInfo>& fromElements,
		Element* to,
		set<Element*>& accessedToElements,
		vector<CopyElementInfo>& toElements)
	{

		if (from->getElementType() == Element::NUMERIC
			|| to->getElementType() == Element::NUMERIC) {
				fromElements.push_back(CopyElementInfo(from, false));  
				toElements.push_back(CopyElementInfo(to, false));
				return true;
		}
		else if (from->getElementType() == Element::STRING
			|| to->getElementType() == Element::STRING) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"source paths are not supported");
		}
		else if (from->getElementType() == Element::CONSOLIDATED
			&& to->getElementType() == Element::CONSOLIDATED) {

				const ElementsWeightType * fromChildren = dimension->getChildren(from);
				const ElementsWeightType * toChildren = dimension->getChildren(to);

				/*size_t fromChildrenCount = fromChildren->size();
				size_t toChildrenCount = toChildren->size();*/

				bool equal = true;

				for (ElementsWeightType::const_iterator f = fromChildren->begin(); f!=fromChildren->end(); f++)
					if (accessedFromElements.find(f->first)!=accessedFromElements.end()) 
						equal = false;
					else
						accessedFromElements.insert(f->first);

				for (ElementsWeightType::const_iterator t = toChildren->begin(); t!=toChildren->end(); t++)
					if (accessedToElements.find(t->first)!=accessedToElements.end())
						equal = false;
					else
						accessedToElements.insert(t->first);

				if (equal && fromChildren->size() != toChildren->size()) {
					equal = false;
				} 

				if (equal) {
					for (ElementsWeightType::const_iterator f = fromChildren->begin(), t = toChildren->begin(); f != fromChildren->end();
						f++, t++) {
							if (f->second != t->second) {
								equal = false;
								break;
							}
					}
				}

				if (equal) {
					fromElements.push_back(CopyElementInfo(from, true));  
					toElements.push_back(CopyElementInfo(to, true));
					for (ElementsWeightType::const_iterator f = fromChildren->begin(), t = toChildren->begin();
						f != fromChildren->end();
						f++, t++) {
							bool equalSub = computeCompatibleElements_m3(dimension,
								f->first, accessedFromElements, fromElements.back().children,
								t->first, accessedToElements, toElements.back().children);
							equal = equal && equalSub; 
					}
				}
				else {
					fromElements.push_back(CopyElementInfo(from, false));  
					toElements.push_back(CopyElementInfo(to, false));
				}
				return equal;
		}
		else {
			throw ErrorException(ErrorException::ERROR_INTERNAL,
				"unknown element type");
		}
	}

	void Cube::copyLikeCellValues (CellPath* cellPathFrom,
		CellPath* cellPathTo,
		User* user,
		PaloSession * session,
		double value) {

			// check cell path
			if (cellPathFrom->getCube() != this || cellPathTo->getCube() != this) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"wrong cube used for cell path");
			}

			checkPathAccessRight(user, cellPathFrom, RIGHT_READ);

			if (cellPathFrom->getPathType() == Element::STRING) {
				throw ParameterException(ErrorException::ERROR_INVALID_ELEMENT_TYPE,
					"string path cannot be copied from",
					"source element type", cellPathFrom->getPathType());
			}

			if (value == 0.0) {
				throw ParameterException(ErrorException::ERROR_INVALID_COPY_VALUE,
					"wrong value for copy like",
					"value", 0.0);
			}

			bool found;
			set< pair<Rule*, IdentifiersType> > ruleHistory;
			CellValueType cellValue = getCellValue(cellPathFrom, &found, 0, 0, &ruleHistory);

			if (found) {
				double factor = value / cellValue.doubleValue;
				copyCellValues(cellPathFrom, cellPathTo, user, session, factor);
			}
			else {
				clearCellValue(cellPathTo, user, session, false, false, Lock::checkLock);
			}

			status = CHANGED;
			updateClientCacheToken();

			// we need to recalculate the markers
			Server::addChangedMarkerCube(this);
	}



	void Cube::copyCellValues (vector< vector<Element*> >* elementsWeigthFrom,
		vector< vector<Element*> >* elementsWeigthTo, 
		User* user,
		PaloSession* session,
		double factor,
		Lock* lock) {

			vector< Element* > fromElements;
			fromElements.resize(elementsWeigthFrom->size());

			vector< Element* > toElements;
			toElements.resize(elementsWeigthTo->size());

			if (lock) {
				RollbackStorage* rstorage = lock->getStorage();

				double numCells = 1.0;
				for (vector< vector<Element*> >::iterator i = elementsWeigthTo->begin(); i != elementsWeigthTo->end(); i++) {
					numCells *= i->size();
				}

				// check number of values and capacity of rollback storage        
				if (!rstorage->hasCapacity(numCells)) {
					throw ErrorException(ErrorException::ERROR_CUBE_LOCK_NO_CAPACITY,
						"rollback size is exceeded");
				}

				rstorage->addRollbackStep();
			}


			copyCellValuesRecursive(elementsWeigthFrom->begin(), elementsWeigthFrom->end(),
				elementsWeigthTo->begin(),
				&fromElements, &toElements,
				0, 
				user, session,
				factor,
				lock);
	}



	void Cube::copyCellValuesRecursive (vector< vector<Element*> >::iterator fromIterator,
		vector< vector<Element*> >::iterator endIterator,
		vector< vector<Element*> >::iterator toIterator,
		vector< Element* >* fromElements,
		vector< Element* >* toElements,
		int pos,
		User* user,
		PaloSession* session,
		double factor,
		Lock* lock) {

			if (fromIterator != endIterator) {
				vector<Element*>::iterator f   = fromIterator->begin();
				vector<Element*>::iterator end = fromIterator->end();
				vector<Element*>::iterator t   = toIterator->begin();

				for (;  f != end;  f++, t++) {
					(*fromElements)[pos] = *f;
					(*toElements)[pos]   = *t;

					copyCellValuesRecursive(fromIterator + 1, endIterator,
						toIterator + 1,
						fromElements, toElements,
						pos + 1,
						user, session,
						factor,
						lock);
				}    
			}
			else {

#if 0
				static int index = 0;
				++index;

				if (index % 10000 == 0)
					cout<<index<<" ";
#endif

				CellPath pathFrom(this, fromElements);
				CellPath pathTo(this, toElements);

				bool found;
				set< pair<Rule*, IdentifiersType> > ruleHistory;
				CellValueType cellValue = getCellValue(&pathFrom, &found, 0, 0, &ruleHistory);

				if (found) {
					if (cellValue.type != Element::NUMERIC) {
						throw ErrorException(ErrorException::ERROR_INTERNAL,
							"source path must be numeric in copyCellValuesRecursive");
					}

					setCellValue(&pathTo, cellValue.doubleValue * factor, user, session, false, false, false, DEFAULT, lock);
				}
				else {
					setCellValue(&pathTo, 0.0, user, session, false, false, false, DEFAULT, lock);
				}
			}
	}

	void Cube::copyCellValues (vector< vector<CopyElementInfo> >* elementsWeigthFrom,
		vector< vector<CopyElementInfo> >* elementsWeigthTo, 
		User* user,
		PaloSession* session,
		double factor,
		Lock* lock) {

			if (lock) {
				RollbackStorage* rstorage = lock->getStorage();

				double numCells = 1.0;
				for (vector< vector<CopyElementInfo> >::iterator i = elementsWeigthTo->begin(); i != elementsWeigthTo->end(); i++) {
					numCells *= i->size();
				}

				// check number of values and capacity of rollback storage        
				if (!rstorage->hasCapacity(numCells)) {
					throw ErrorException(ErrorException::ERROR_CUBE_LOCK_NO_CAPACITY,
						"rollback size is exceeded");
				}

				rstorage->addRollbackStep();
			}

			vector<CopyElementInfo*> from; from.resize(elementsWeigthFrom->size());
			vector<CopyElementInfo*> to; to.resize(elementsWeigthTo->size());

			vector< Element* > fromElements; fromElements.resize(elementsWeigthFrom->size());
			vector< Element* > toElements; toElements.resize(elementsWeigthTo->size());

			int zeroCheckCount = 0;

			for (size_t i = 0; i<elementsWeigthFrom->size(); i++) {
				if ((*elementsWeigthFrom)[i].size()!=1)
					throw ErrorException(ErrorException::ERROR_INTERNAL,
					"source path must contain exactly one element in each dimension");
				from[i] = &((*elementsWeigthFrom)[i][0]);
				to[i] = &((*elementsWeigthTo)[i][0]);
				fromElements[i] = from[i]->element;
				toElements[i] = to[i]->element;
				if (from[i]->zero_check)
					zeroCheckCount++;
			}

			vector<IdentifiersWeightType> baseElements;
			CellPath cellPathTo(this, &toElements);
			computeBaseElements(&cellPathTo, &baseElements, false);

			int nsi = -1;
			size_t count = 0;
			bool addCubeToChangedMarkers = false;
			copyCellValuesRecursive_m3(from, to, fromElements, toElements, nsi, zeroCheckCount, user, session, factor, &count, &addCubeToChangedMarkers, lock);

			removeCachedCells(&cellPathTo, &baseElements);
			if (addCubeToChangedMarkers)
				Server::addChangedMarkerCube(this);

	}	

	void Cube::copyCellValuesRecursive_m3(vector< CopyElementInfo* >& from, vector< CopyElementInfo* >& to,
		vector< Element* >& fromElements,
		vector< Element* >& toElements,
		int lastDiggDimIndex,		
		int zeroCheckCount,
		User* user,
		PaloSession* session,
		double factor,
		size_t* count,
		bool* addCubeToChangedMarkers,
		Lock * lock) 
	{
		if ((++(*count) % TIME_BASED_PROGRESS_COUNT) == 0) 
			database->getServer()->timeBasedProgress();		

		bool zero_check = zeroCheckCount>0;

		CellPath pathFrom(this, &fromElements);
		bool fromFound, toFound;
		set< pair<Rule*, IdentifiersType> > ruleHistory;
		CellValueType fromCellValue = getCellValue(&pathFrom, &fromFound, 0, 0, &ruleHistory);

		if (!fromFound/* || fromCellValue.doubleValue == 0.0*/) {
			CellPath pathTo(this, &toElements);			
			CellValueType toCellValue = getCellValue(&pathTo, &toFound, 0, 0, &ruleHistory);
			if (toFound) {
				//clear consolidated cell value
				vector< set<IdentifierType> > baseElements;
				double cnt = computeBaseElements(&pathTo, &baseElements);				
				storageDouble->removeCellValue(&baseElements, lock);		
				*addCubeToChangedMarkers = true;
			}
		} else if (!zero_check) {
			if (fromCellValue.type != Element::NUMERIC) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"source path must be numeric in copyCellValuesRecursive");
			} 
			//set value
			CellPath pathTo(this, &toElements);	
			//setCellValue(&pathTo, fromCellValue.doubleValue * factor, user, session, false, false, false, DEFAULT, lock);	
			double v = fromCellValue.doubleValue * factor;
			if (v==0.0) {
				//clear base cell value
				if (lock) 
					lock->getStorage()->addCellValue(pathFrom.getPathIdentifier(), &fromCellValue.doubleValue);			
				storageDouble->deleteCell(&fromElements);	
				*addCubeToChangedMarkers = true;
			} else {
				if (lock) {
					double* cellValue = (double*) storageDouble->getCellValue(&pathTo);
					lock->getStorage()->addCellValue(pathTo.getPathIdentifier(), cellValue);
				}
				setBaseCellValue(&toElements, v);
			}

		} else {			
			//find way to split
			int diggDimIndex = -1;

			for (int i = 0; i!=from.size(); i++) {
				lastDiggDimIndex = (lastDiggDimIndex+1)%(int)from.size();
				if (from[lastDiggDimIndex]->zero_check) {
					diggDimIndex = lastDiggDimIndex;
					break;				
				}
			}		

			//digg deeper
			if (diggDimIndex!=-1) {		

				CopyElementInfo* fromCEI = from[diggDimIndex];
				CopyElementInfo* toCEI = to[diggDimIndex];

				for (size_t c = 0; c<fromCEI->children.size(); c++) {

					from[diggDimIndex] = &(fromCEI->children[c]);
					to[diggDimIndex] = &(toCEI->children[c]);

					int zc = from[diggDimIndex]->zero_check ? zeroCheckCount : zeroCheckCount-1;

					fromElements[diggDimIndex] = from[diggDimIndex]->element;
					toElements[diggDimIndex] = to[diggDimIndex]->element;

					copyCellValuesRecursive_m3(from, to, fromElements, toElements, diggDimIndex, zc, user, session, factor, count, addCubeToChangedMarkers, lock);
				}

				fromElements[diggDimIndex] = fromCEI->element;
				toElements[diggDimIndex] = toCEI->element;

				from[diggDimIndex] = fromCEI;
				to[diggDimIndex] = toCEI;
			}
		}	
	}

	bool Cube::computeSplashElements_m3 (CellPath* cellPathFrom,
		vector< vector<CopyElementInfo> >& baseElementsFrom) {

			const PathType* from = cellPathFrom->getPathElements();

			bool splash = false;
			bool disjunct = false;
			vector<Dimension*>::iterator dimIter = dimensions.begin();

			for (PathType::const_iterator f = from->begin();
				f != from->end();
				f++, dimIter++) {
					Dimension * dimension = *dimIter;
#if 0
					std::cout<<"Dimension: "<<dimension->getName()<<"... ";
#endif

					vector<CopyElementInfo> elementsFrom;

					bool multiple = false;



					set<Element*> accessedFromElements; accessedFromElements.insert(*f);


					bool compatible = computeSplashElements_m3(dimension,
						*f, accessedFromElements, elementsFrom);

					baseElementsFrom.push_back(elementsFrom);				
			}		

			return splash;

	}

	bool Cube::computeSplashElements_m3(Dimension* dimension,
		Element* from, 
		set<Element*>& accessedFromElements,
		vector<CopyElementInfo>& fromElements) {

			if (from->getElementType() == Element::NUMERIC) {
				fromElements.push_back(CopyElementInfo(from, false));  				
				return true;
			}
			else if (from->getElementType() == Element::STRING) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"source paths are not supported");
			}
			else if (from->getElementType() == Element::CONSOLIDATED) {

				const ElementsWeightType * fromChildren = dimension->getChildren(from);		

				for (ElementsWeightType::const_iterator f = fromChildren->begin(); f!=fromChildren->end(); f++)
					if (accessedFromElements.find(f->first)==accessedFromElements.end()) 
						accessedFromElements.insert(f->first);

				fromElements.push_back(CopyElementInfo(from, true));  

				for (ElementsWeightType::const_iterator f = fromChildren->begin(); f != fromChildren->end(); f++) {
					bool equalSub = computeSplashElements_m3(dimension,
						f->first, accessedFromElements, fromElements.back().children);							
				}

				return true;
			}
			else {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"unknown element type");
			}

	}

	void Cube::scaleCellValuesRecursive_m3(vector< CopyElementInfo* >& from, 
		vector< Element* >& fromElements,		
		int lastDiggDimIndex,		
		int zeroCheckCount,
		User* user,
		PaloSession* session,
		double factor,
		size_t* count,
		bool* addCubeToChangedMarkers, 
		Lock * lock) 
	{
		if ((++(*count) % TIME_BASED_PROGRESS_COUNT) == 0) 
			database->getServer()->timeBasedProgress();		

		bool zero_check = zeroCheckCount>0;

		CellPath pathFrom(this, &fromElements);
		bool fromFound;
		set< pair<Rule*, IdentifiersType> > ruleHistory;
		CellValueType fromCellValue = getCellValue(&pathFrom, &fromFound, 0, 0, &ruleHistory);

		if (!fromFound/* || fromCellValue.doubleValue == 0.0*/) {
			//nothing to scale
		} else if (!zero_check) {
			if (fromCellValue.type != Element::NUMERIC) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"source path must be numeric in copyCellValuesRecursive");
			} 			
			//setCellValue(&pathTo, fromCellValue.doubleValue * factor, user, session, false, false, false, DEFAULT, lock);	
			double v = fromCellValue.doubleValue * factor;
			if (v==0.0) {
				if (lock) 
					lock->getStorage()->addCellValue(pathFrom.getPathIdentifier(), &fromCellValue.doubleValue);			
				storageDouble->deleteCell(&fromElements);
				*addCubeToChangedMarkers = true;
			} else {
				if (lock) 
					lock->getStorage()->addCellValue(pathFrom.getPathIdentifier(), &fromCellValue.doubleValue);			
				setBaseCellValue(&fromElements, v);
			}

		} else {			
			//invalidate consolidated cell cached value			
			cacheConsolidationsStorage->deleteCell(&fromElements);
			invalidateCacheCounter++;

			//find way to split
			int diggDimIndex = -1;

			for (int i = 0; i!=from.size(); i++) {
				lastDiggDimIndex = (lastDiggDimIndex+1)%(int)from.size();
				if (from[lastDiggDimIndex]->zero_check) {
					diggDimIndex = lastDiggDimIndex;
					break;				
				}
			}		

			//digg deeper
			if (diggDimIndex!=-1) {		

				CopyElementInfo* fromCEI = from[diggDimIndex];				

				for (size_t c = 0; c<fromCEI->children.size(); c++) {

					from[diggDimIndex] = &(fromCEI->children[c]);					

					int zc = from[diggDimIndex]->zero_check ? zeroCheckCount : zeroCheckCount-1;

					fromElements[diggDimIndex] = from[diggDimIndex]->element;	
					scaleCellValuesRecursive_m3(from, fromElements, diggDimIndex, zc, user, session, factor, count, addCubeToChangedMarkers, lock);
				}

				fromElements[diggDimIndex] = fromCEI->element;
				from[diggDimIndex] = fromCEI;
			}
		}
	}

	void Cube::setCellValueConsolidated (CellPath* cellPath, double value, SplashMode splashMode, User* user, Lock* lock) {
		if (splashMode == DISABLED) {
			throw ParameterException(ErrorException::ERROR_SPLASH_DISABLED,
				"cell path is consolidated, but splashing is disabled",
				"splashMode", splashMode);
		}


		// get base elements (with type and weight) 
		vector<IdentifiersWeightType> baseElements;
		computeBaseElements(cellPath, &baseElements, false);

		for (vector<IdentifiersWeightType>::iterator i = baseElements.begin(); i != baseElements.end(); i++) {
			if ( (*i).empty() ) {
				throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
					"cell path is consolidated without any children, cannot splash",
					"splashMode", splashMode);
			}
		}

		// default splash mode
		if (splashMode == DEFAULT) {
			CellValueType result;
			bool found = false;

			size_t numFoundElements = 0;
			result.doubleValue = storageDouble->getConsolidatedCellValue(&baseElements, &found, &numFoundElements, user);

			// no path with value found, we have to distribute values equally on every numeric path
			if (!found || result.doubleValue == 0.0) {

				double sumWeights = 1.0;
				for (vector<IdentifiersWeightType>::iterator i = baseElements.begin(); i != baseElements.end(); i++) {
					double dimensionWeight = 0.0;          
					for (IdentifiersWeightType::iterator j = i->begin(); j != i->end(); j++) {
						dimensionWeight += j->second;
					}
					sumWeights *= dimensionWeight;
				}

				if (sumWeights == 0.0) {
					throw ParameterException(ErrorException::ERROR_SPLASH_NOT_POSSIBLE,
						"sum of weights is 0.0, cannot splash",
						"splashMode", splashMode);
				}

				double baseValue = value / sumWeights;

				// now update all numeric cells
				setBaseElementsRecursive(&baseElements, baseValue, SET_BASE, cellPath, user, lock);        
			}
			// change values by factor
			else {
				if (result.doubleValue == 0.0) {
					throw ParameterException(ErrorException::ERROR_SPLASH_NOT_POSSIBLE,
						"aggregated cell value is 0.0, cannot splash",
						"splashMode", splashMode);
				}

				double factor = value / result.doubleValue;

				if (lock == Lock::checkLock) {
					lock = lookupLockedArea(cellPath, user);
					if (lock) {
						RollbackStorage* rstorage = lock->getStorage();

						// check capacity of rollback storage
						if (!rstorage->hasCapacity(numFoundElements * 1.0)) {
							throw ErrorException(ErrorException::ERROR_CUBE_LOCK_NO_CAPACITY,
								"rollback size is exceeded");
						}

						// add new step to rollback storage
						rstorage->addRollbackStep();
					}
				}    

				// save values to rollback storage in storageDouble->setConsolidatedCellValue
				// TODO
				//   cout << "factor == " << factor << endl;
				//   if (factor > 0.999999999999 && factor < 1.0000000001) {
				//     lock = 0;
				//   }				
				//storageDouble->setConsolidatedCellValue(&baseElements, factor, lock);

				vector< vector<CopyElementInfo> > elementsWeigthFrom;

				bool splash = computeSplashElements_m3(cellPath, elementsWeigthFrom);				

				vector<CopyElementInfo*> from; from.resize(elementsWeigthFrom.size());
				vector< Element* > fromElements; fromElements.resize(elementsWeigthFrom.size());
				int zeroCheckCount = 0;

				for (size_t i = 0; i<elementsWeigthFrom.size(); i++) {
					if ((elementsWeigthFrom)[i].size()!=1)
						throw ErrorException(ErrorException::ERROR_INTERNAL,
						"source path must contain exactly one element in each dimension");
					from[i] = &((elementsWeigthFrom)[i][0]);					
					fromElements[i] = from[i]->element;					
					if (from[i]->zero_check)
						zeroCheckCount++;
				}

				size_t count = 0;
				bool addCubeToChangedMarkers = false;
				scaleCellValuesRecursive_m3(from, fromElements, 0, zeroCheckCount, user, 0, factor, &count, &addCubeToChangedMarkers, lock);
				if (addCubeToChangedMarkers)
					Server::addChangedMarkerCube(this);
			}
		}

		// set fixed value to all cells
		else if (splashMode == SET_BASE) {
			setBaseElementsRecursive(&baseElements, value, splashMode, cellPath, user, lock);
		}

		// add fixed value to all existing cells
		else if (splashMode == ADD_BASE) {      
			setBaseElementsRecursive(&baseElements, value, splashMode, cellPath, user, lock);
		}

		removeCachedCells(cellPath, &baseElements);
	}



	void Cube::setBaseElementsRecursive (vector<IdentifiersWeightType> *baseElements,
		double value,
		SplashMode splashMode,
		CellPath* cellPath,
		User* user,
		Lock* lock) {
			double numCells = 1.0;
			for (vector<IdentifiersWeightType>::iterator i = baseElements->begin(); i != baseElements->end(); i++) {
				numCells *= i->size();
			}

			Logger::trace << "splashing: setting '" << numCells << "' cells" << endl;

			double megaBytes = (baseElements->size() * 4 + 16) * numCells / (1024*1024);        

			if (megaBytes > splashLimit1) {
				Logger::error   << "palo will need about '" << megaBytes << "' mega-bytes for splashing" << endl;
				throw ParameterException(ErrorException::ERROR_SPLASH_NOT_POSSIBLE,
					"too many cells",
					"splashMode", splashMode);
			}
			else if (megaBytes > splashLimit2) {
				Logger::warning << "palo will need about '" << megaBytes << "' mega-bytes for splashing" << endl;
			}
			else if (megaBytes > splashLimit3) {
				Logger::info    << "palo will need about '" << megaBytes << "' mega-bytes for splashing" << endl;
			}
			else {
				Logger::trace   << "palo will need about '" << megaBytes << "' mega-bytes for splashing" << endl;
			}

			if (lock == Lock::checkLock) {
				lock = lookupLockedArea(cellPath, user);
				if (lock) {
					RollbackStorage* rstorage = lock->getStorage();

					// check number of values and capacity of rollback storage        
					if (!rstorage->hasCapacity(numCells)) {
						throw ErrorException(ErrorException::ERROR_CUBE_LOCK_NO_CAPACITY,
							"rollback size is exceeded");
					}

					// add new step to rollback storage
					rstorage->addRollbackStep();
				}
			}    

			IdentifiersType path;
			path.resize(baseElements->size());

			size_t count = 0;
			setBaseElementsRecursive(0, baseElements, &path, value, splashMode, &count, lock);
	}



	void Cube::setBaseElementsRecursive (size_t position,
		vector<IdentifiersWeightType> *baseElements,
		IdentifiersType *path,
		double value,
		SplashMode splashMode,
		size_t * count,
		Lock* lock) {

			if ((++(*count) % TIME_BASED_PROGRESS_COUNT) == 0) 
				database->getServer()->timeBasedProgress();


			// last dimension found
			if (position == baseElements->size()) {

				// set fixed value to all cells
				if (splashMode == SET_BASE) {
					if (lock) {
						RollbackStorage* rstorage = lock->getStorage();

						double* cellValue = (double*) storageDouble->getCellValue(path);
						rstorage->addCellValue(path, cellValue);
					}
					setBaseCellValue(path,value);
				}

				// add fixed value to all existing cells
				else if (splashMode == ADD_BASE) {
					double* cellValue = (double*) storageDouble->getCellValue(path);

					if (lock) {
						RollbackStorage* rstorage = lock->getStorage();
						rstorage->addCellValue(path, cellValue);
					}

					if (cellValue != 0) {          
						*cellValue += value;
					}
					else {
						setBaseCellValue(path, value);
					}
				}
			}
			else {
				for (IdentifiersWeightType::iterator i = baseElements->at(position).begin();
					i != baseElements->at(position).end();
					i++) {
						path->at(position) = i->first; 
						setBaseElementsRecursive(position+1, baseElements, path, value, splashMode, count, lock);
				} 
			}
	}	

	semaphore_t Cube::setCellValue (CellPath* cellPath,
		double value, 
		User* user, 
		PaloSession * session,
		bool checkArea,
		bool sepRight,
		bool addValue,
		SplashMode splashMode,
		Lock * lock,
		bool ignoreJournal) {

			// check value
			Element::ElementType type = cellPath->getPathType();

			if (addValue) {
				if (type == Element::NUMERIC) {
					double* cellValue = (double*) storageDouble->getCellValue(cellPath);

					if (cellValue != 0) {
						value += *cellValue;
					}
				}
				else if (type == Element::CONSOLIDATED && splashMode == Cube::DEFAULT) {
					bool found;
					CellValueType cellValue = getCellValue(cellPath, &found, 0, 0, 0, false);

					if (found) {
						value += cellValue.doubleValue;
					}
				}
			}

			if (value == 0.0) {
				if (type == Element::CONSOLIDATED && splashMode == DISABLED) {
					throw ParameterException(ErrorException::ERROR_SPLASH_DISABLED,
						"cell path is consolidated, but splashing is disabled",
						"splashMode", splashMode);
				}

				return clearCellValue(cellPath, user, session, checkArea, sepRight, lock);
			}

			// check cell path
			if (cellPath->getCube() != this) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"wrong cube used for cell path");
			}

			checkPathAccessRight(user, cellPath, RIGHT_WRITE);

			string area;

			// use the supervision event processor (SEP)
			if (checkArea) {
				if (isInArea(cellPath, area)) {
					if (session) {
						return cubeWorker->executeSetCell(area,
							session->getEncodedIdentifier(),
							cellPath->getPathIdentifier(), value);
					}
					else {
						return cubeWorker->executeSetCell(area,
							"",
							cellPath->getPathIdentifier(), 
							value);
					}
				}
			}

			// check that the user is allowed circumventing the SEP
			else if (sepRight) {
				checkSepRight(user, RIGHT_DELETE);
			}

			// now set the element value
			if (type == Element::NUMERIC) {
				if (lock == Lock::checkLock) {
					lock = lookupLockedArea(cellPath, user);

					if (lock) {
						RollbackStorage* rstorage = lock->getStorage();

						// check capacity of rollback storage        
						if (!rstorage->hasCapacity(1.0)) {
							throw ErrorException(ErrorException::ERROR_CUBE_LOCK_NO_CAPACITY,
								"rollback size is exceeded");
						}

						// add new step to rollback storage
						rstorage->addRollbackStep();
					}
				}    

				if (lock) {
					RollbackStorage* rstorage = lock->getStorage();

					// get old cell value
					double * old = (double*) storageDouble->getCellValue(cellPath);

					// save old value
					rstorage->addCellValue(cellPath->getPathIdentifier(), old);
				}

				setBaseCellValue(cellPath->getPathIdentifier(), value);
				removeCachedCells(cellPath);

				if (!ignoreJournal && journal != 0) {
					journal->appendCommand(Server::getUsername(user),
						Server::getEvent(),
						"SET_DOUBLE");
					journal->appendIdentifiers(cellPath->getPathIdentifier());
					journal->appendInteger(splashMode);
					journal->appendDouble(value);
					journal->nextLine();
				}
			}
			else if (type == Element::CONSOLIDATED) {
				if (user && user->getRoleCellDataRight() < RIGHT_SPLASH) {
					throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
						"insufficient access rights for path",
						"user", (int) user->getIdentifier());
				}

				setCellValueConsolidated(cellPath, value, splashMode, user, lock); // remove cached cells in setCellValueConsolidated

				if (!ignoreJournal && journal != 0) {
					journal->appendCommand(Server::getUsername(user),
						Server::getEvent(),
						"SET_DOUBLE");
					journal->appendIdentifiers(cellPath->getPathIdentifier());
					journal->appendInteger(splashMode);
					journal->appendDouble(value);
					journal->nextLine();
				}
			}
			else {
				throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
					"wrong value type for cell",
					"value", "NUMERIC");
			}

			status = CHANGED;
			updateClientCacheToken();
			return 0;
	}



	semaphore_t Cube::setCellValue (CellPath* cellPath,
		const string& value,
		User* user,
		PaloSession * session,
		bool checkArea,
		bool sepRight,
		Lock * lock) {

			// check cell path
			if (cellPath->getCube() != this) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"wrong cube used for cell path");
			}

			// check value
			if (value.empty()) {
				return clearCellValue(cellPath, user, session, checkArea, sepRight, lock);
			}

			checkPathAccessRight(user, cellPath, RIGHT_WRITE);

			Element::ElementType type = cellPath->getPathType();

			if (type == Element::STRING) {
				string area;

				// use the SEP
				if (checkArea) {
					if (isInArea(cellPath, area)) {
						if (session) {
							return cubeWorker->executeSetCell(area, session->getEncodedIdentifier(), cellPath->getPathIdentifier(), value);
						}
						else {
							return cubeWorker->executeSetCell(area, "", cellPath->getPathIdentifier(), value);
						} 
					}
				}

				// check that the user is allowed circumventing the SEP
				else if (sepRight) {
					checkSepRight(user, RIGHT_DELETE);
				}

				if (lock == Lock::checkLock) {
					Lock* lock = lookupLockedArea(cellPath, user);
					if (lock) {
						RollbackStorage* rstorage = lock->getStorage();

						// check capacity of rollback storage        
						if (!rstorage->hasCapacity(1.0)) {
							throw ErrorException(ErrorException::ERROR_CUBE_LOCK_NO_CAPACITY,
								"rollback size is exceeded");
						}

						// get old cell value
						char * * old = (char * *) storageString->getCellValue(cellPath);

						// add new step to rollback storage
						rstorage->addRollbackStep();

						// save old value
						rstorage->addCellValue(cellPath->getPathIdentifier(), old);
					}
				}

				setBaseCellValue(cellPath->getPathElements(), value);
				database->clearRuleCaches();

				if (journal != 0) {
					journal->appendCommand(Server::getUsername(user),
						Server::getEvent(),
						"SET_STRING");
					journal->appendIdentifiers(cellPath->getPathIdentifier());
					journal->appendEscapeString(value);
					journal->nextLine();
				}
			}
			else {
				throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
					"wrong value type for cell",
					"value", "STRING");
			}

			status = CHANGED;
			updateClientCacheToken();
			return 0;
	}



	////////////////////////////////////////////////////////////////////////////////
	// functions getting cell values
	////////////////////////////////////////////////////////////////////////////////

	Cube::CellValueType Cube::getCellValue (CellPath* cellPath,
		bool* found,
		User* user,
		PaloSession * session,
		set< pair<Rule*, IdentifiersType> >* ruleHistory,
		bool useEnterpriseRules,
		bool* isCachable) {

			// check cell path and access rights
			if (cellPath->getCube() != this) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"wrong cube used for cell path");
			}

			checkPathAccessRight(user, cellPath, RIGHT_READ);

			checkNewMarkerRules();

			// reset cache counter because of a read 
			invalidateCacheCounter = 0;

			// initialize to not found
			*found = false;

			// initialize is cachable
			bool dummy;

			if (isCachable == 0) {
				isCachable = &dummy;
			}

			*isCachable = true;

			Element::ElementType elementType = cellPath->getPathType();

			// check if we have an enterprise rule for this cell
			if (useEnterpriseRules) {
				CellValueType cellValue;

				// check if we have cached a rule
				if (cacheRulesStorage) {
					if (findInRuleCache(cacheRulesStorage, cellPath, cellValue, found)) {
						return cellValue;
					}
				}

				// try enterprise rules direct match
				if (findDirectRuleMatch(database, rules, cacheRulesStorage, cellPath, user, cellValue, found, isCachable, ruleHistory)) {
					return cellValue;
				}

				// try enterprise rules in consolidations
				if (elementType == Element::CONSOLIDATED
					&& findIndirectRuleMatch(database, this, rules, cacheRulesStorage, cellPath, cellValue, found, isCachable, ruleHistory)) {
						return cellValue;
				}
			}

			// look up a numeric path in the double storage
			if (elementType == Element::NUMERIC) {
				double * value = (double*) storageDouble->getCellValue(cellPath);

				CellValueType cellValue;
				cellValue.type = Element::NUMERIC;
				cellValue.rule = Rule::NO_RULE;

				if (value == 0) {
					cellValue.doubleValue = 0.0;
				}
				else {
					cellValue.doubleValue = * value;
					*found = true;
				}

				return cellValue;
			}

			// look up a string path in the string storage
			else if (elementType == Element::STRING) {
				char * * value = (char * *) storageString->getCellValue(cellPath);

				CellValueType cellValue;
				cellValue.type = Element::STRING;
				cellValue.rule = Rule::NO_RULE;

				if (value == 0) {
					cellValue.charValue.clear();
				}
				else {
					cellValue.charValue.assign(*value);
					*found = true;
				}

				return cellValue;
			}

			// look up a consolidation in the double storage
			else if (elementType == Element::CONSOLIDATED) {

				// result must be numeric, otherwise path would be of type string
				CellValueType cellValue;
				cellValue.type = Element::NUMERIC;
				cellValue.rule = Rule::NO_RULE;

				// do not compute values for an emtpy cube
				if (storageDouble->size() == 0) {
					cellValue.doubleValue = 0.0;
					*found = false;

					return cellValue;
				}

				double count = countBaseElements(cellPath);

				bool showDebug = false;				

				if (showDebug) {
					// base elements of consolidated element will get computed					
					const IdentifiersType * p = cellPath->getPathIdentifier();
					bool first = true;
					cout << "getCellValue path = <" ; 
					for (IdentifiersType::const_iterator i = p->begin(); i != p->end(); i++) {
						if (first) first = false; else cout << ",";
						cout << *i;
					}
					cout << "> count = " << count << " cacheBarrier = " << cacheBarrier;

					cout << " dimension count <" ;
					first = true; 

					if (count < 1.0) {
						vector<IdentifiersWeightType> baseElements;
						computeBaseElements(cellPath, &baseElements, true);

						for (vector<IdentifiersWeightType>::iterator i = baseElements.begin(); i != baseElements.end(); i++) {
							if (first) first = false; else cout << ",";
							cout << i->size();
						}
					}

					cout << ">";
				}

				bool cacheValue = false;

				if (cacheConsolidationsStorage && count > cacheBarrier) {        

					// save path to the session history
					if (session) {
						session->addPathToHistory(database->getIdentifier(), getIdentifier(), cellPath->getPathIdentifier());
					}

					double * value = (double*) cacheConsolidationsStorage->getCellValue(cellPath);

					if (value) {
						if (isnan(*value)) {
							*found = false;
							cellValue.doubleValue = 0.0;
						}
						else {
							*found = true;
							cellValue.doubleValue = *value;
						}

						if (showDebug) {
							cout << " found value in cache =" << cellValue.doubleValue << endl;
						}

						return cellValue;
					}

					if (showDebug) {
						cout << " did not found value in cache " << endl;
					}

					// check history
					if (session && session->getSizeOfHistory(database->getIdentifier(), getIdentifier()) > 5) {

						// fill cache
						bool cacheFilled = fillCacheByHistory(session);

						// get and return value from cache
						if (cacheFilled) {
							double * value = (double*) cacheConsolidationsStorage->getCellValue(cellPath);

							if (value) {
								if (isnan(*value)) {
									*found = false;
									cellValue.doubleValue = 0.0;
								}
								else {
									*found = true;
									cellValue.doubleValue = *value;
								}

								return cellValue;
							}            
						}
					}

					cacheValue = true;
				}
				if (showDebug) {
					cout << endl;
				}

				// compute real base elements
				vector<IdentifiersWeightType> baseElements;
				count = computeBaseElements(cellPath, &baseElements, true);

				// get value from storage
				if (count > 0.0) {
					size_t numFoundElements = 0;
					cellValue.doubleValue = storageDouble->getConsolidatedCellValue(&baseElements, found, &numFoundElements, user);
				}

				// must be empty
				else {
					cellValue.doubleValue = 0.0;
					*found = false; 
				}

				// save value to cache storage
				if (cacheValue) {
					if (*found) {
						cacheConsolidationsStorage->setCellValue(cellPath, (uint8_t*) &cellValue.doubleValue);
					}
					else {          
						double d = numeric_limits<double>::quiet_NaN();          
						cacheConsolidationsStorage->setCellValue(cellPath, (uint8_t*) &d);
					}        
				}

				return cellValue;
			}

			// unknown element type
			else {
				throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
					"wrong element type",
					"elementType", (int) elementType);
			}
	}  



	bool Cube::getAreaCellValues (AreaStorage* storage, 
		vector<IdentifiersType>* area, 
		vector<IdentifiersType>* resultPathes,    
		User* user) {
			Statistics::Timer timer("Cube::getAreaCellValues");

			if (dimensions.size() < 3) {
				getCellValuesFromCache(storage, resultPathes, true, user);      
				return true;
			}

			// check if we have cached the area
			set< vector<IdentifiersType> >::iterator ic = cachedAreas.find(*area);

			if (ic == cachedAreas.end()) {
				getCellValues(storage, area, resultPathes, true, user);
				cachedAreas.insert(*area);
			}
			else {
				getCellValuesFromCache(storage, resultPathes, true, user);
			}

			return true;
	} 



	void Cube::getListCellValues (AreaStorage* storage, 
		vector<IdentifiersType>* cellPathes,
		User* user) {

			Statistics::Timer timer("Cube::getListCellValues");

			if (dimensions.size() < 3) {
				getCellValuesFromCache(storage, cellPathes, false, user);
				return;
			}

			// build area from the list of cell paths    
			vector< set<IdentifierType> > areaSet(dimensions.size());

			for (vector<IdentifiersType>::iterator i = cellPathes->begin();  i != cellPathes->end();  i++) {
				size_t num = 0;

				// copy identifiers to a set
				for (IdentifiersType::iterator j = i->begin();  j != i->end();  j++, num++) {
					areaSet[num].insert(*j);
				}
			}    

			// convert sets back into vectors
			size_t num = 0;
			vector< IdentifiersType > area(dimensions.size());

			for (vector< set<IdentifierType> >::iterator i = areaSet.begin(); i != areaSet.end(); i++, num++) {
				// copy identifiers of the set to the area
				for (set<IdentifierType>::iterator j = i->begin(); j != i->end(); j++) {
					area[num].push_back(*j);
				}
			}    

			// check if we have already cached this area
			set< vector<IdentifiersType> >::iterator ic = cachedCellValues.find(area);

			if (ic == cachedCellValues.end()) {
				getCellValues(storage, &area, cellPathes, false, user);
				cachedCellValues.insert(area);
			}
			else {
				getCellValuesFromCache(storage, cellPathes, false, user);
			}
	} 



	void Cube::getCellValues (AreaStorage* storage, 
		vector<IdentifiersType>* area,
		vector<IdentifiersType>* cellPathes,
		bool buildCellPathes,
		User* user) {
			Logger::trace << "started getCellValues" << endl;

			// if the user has read for everything we do not need to check
			bool checkUserRights = false;    

			if (user && getMinimumAccessRight(user) < RIGHT_READ) {
				checkUserRights = true;
			}

			checkNewMarkerRules();

			// without rules to do not try to use them
			bool useEnterpriseRules = !rules.empty();

			// optimize empty cubes
			bool empty = (sizeFilledCells() == 0);

			// if we build cell paths, than clear paths now
			if (buildCellPathes) {
				cellPathes->clear();
			}

			// fill numeric values
			if (!empty) {
				vector< map<Element*, uint32_t> > hashMapping;
				vector<ElementsType> numericArea;
				vector< vector< vector< pair<uint32_t, double> > > > numericMapping;
				vector< map< uint32_t, vector< pair<IdentifierType, double> > > > reverseNumericMapping;
				vector<uint32_t> hashSteps;
				vector<uint32_t> lengths;

				if (computeAreaParameters(this, area, hashMapping, numericArea, numericMapping, reverseNumericMapping, hashSteps, lengths, true)) {
					HashAreaStorage s(hashMapping);

					storageDouble->computeHashAreaCellValue(&s, numericMapping, user);

					s.fillAreaStorage(storage, reverseNumericMapping, hashSteps, lengths);
				}
			}

			// fill strings and rules
			IdentifiersType path(dimensions.size());

			size_t last = storage->size();    

			bool showDebug = false;

			if (showDebug) {
				cout << "storage filled: size = " << last << endl;
			}

			for (size_t i = 0; i < last; i++) {
				Cube::CellValueType * value;

				// build cell path list for cell/area request
				if (buildCellPathes) {
					value = storage->getCell(i, &path);
					cellPathes->push_back(path);
				}

				// use cell path list from cell/value request
				else {
					value = storage->getCell(i);
					path = cellPathes->at(i);
				}

				if (!value) {
					throw ErrorException(ErrorException::ERROR_INTERNAL, "value not found in Cube::getCellValues");
					continue;
				}

				try {

					// check cell path (throws exception)
					CellPath cellPath(this, &path); 

					if (showDebug) {
						bool first = true;
						cout << "path <";
						for (IdentifiersType::const_iterator i = path.begin(); i != path.end(); i++) {
							if (first) first = false; else cout << ",";
							cout << *i ; 
						}
						cout << "> ";
					}

					// assume that we got a real value
					value->rule = Rule::NO_RULE;

					// check permission  (throws exception) 
					if (checkUserRights) {
						checkPathAccessRight(user, &cellPath, RIGHT_READ);
					}

					// get string values
					if (cellPath.getPathType() == Element::STRING) {
						value->type = Element::STRING;

						if (!empty) {
							char * * v = (char * *) storageString->getCellValue(&cellPath);

							if (v == 0) {
								value->charValue.clear();
							}
							else {
								value->charValue.assign(*v);
							}
						}
						else {
							value->charValue.clear();
						}
					}

					bool isRule = false;
					bool found = false;
					bool foundInCache = false;

					// check if we have an enterprise rule for this cell
					if (useEnterpriseRules) {

						// find value in cache
						if (cacheRulesStorage) {
							if (findInRuleCache(cacheRulesStorage, &cellPath, *value, &found)) {
								isRule = true;
								foundInCache = true;
							}
						}

						bool isCachable = true;

						set< pair<Rule*, IdentifiersType> > ruleHistory;

						// try enterprise rules direct match
						if (! foundInCache) {
							if (findDirectRuleMatch(database, rules, cacheRulesStorage, &cellPath, user, *value, &found, &isCachable, &ruleHistory)) {
								isRule = true;
							}

							// try enterprise rules in consolidations
							if (! isRule 
								&& cellPath.getPathType() == Element::CONSOLIDATED
								&& findIndirectRuleMatch(database, this, rules, cacheRulesStorage, &cellPath, *value, &found, &isCachable, &ruleHistory)) {
									isRule = true;
							}
						}
					}

					// cache value
					if (cellPath.getPathType() == Element::CONSOLIDATED && ! isRule) {

						double count = countBaseElements(&cellPath);

						if (showDebug) {
							if (value->type == Element::NUMERIC) {
								cout << " (consolidation) = " << value->doubleValue << " count = " << count << " cacheBarrier = " << cacheBarrier;
							}
							else {
								cout << " (consolidation) = <not set> count = " << count << " cacheBarrier = " << cacheBarrier;
							}
						}

						if (cacheConsolidationsStorage && count > cacheBarrier) {   
							if (showDebug) cout << " is cached!";    
							if (value->type == Element::NUMERIC) {
								cacheConsolidationsStorage->setCellValue(&path, (uint8_t*) &value->doubleValue);
							}
							else {
								double d = numeric_limits<double>::quiet_NaN();          
								cacheConsolidationsStorage->setCellValue(&path, (uint8_t*) &d);
							}
						}
					}
					if (showDebug) cout << endl;
				}      
				catch (ErrorException e) {
					value->type = Element::UNDEFINED;
					value->doubleValue = e.getErrorType();
					value->charValue.assign(e.getMessage());
				}
			}
			Logger::trace << "finished getCellValues" << endl;
	} 



	void Cube::getCellValuesFromCache (AreaStorage* storage, 
		vector<IdentifiersType>* cellPathes,
		bool buildCellPathes,
		User* user) {
			Statistics::Timer timer("Cube::getCellValuesFromCache");

			Logger::trace << "started getCellValuesFromCache" << endl;

			// only use enterprise rules, if we have some
			bool useEnterpriseRules = !rules.empty();

			// if the user has read for everything we do not need to check
			if (user && getMinimumAccessRight(user) >= RIGHT_READ) {
				user = 0;
			}

			// build path list
			if (buildCellPathes) {
				cellPathes->clear();
			}

			IdentifiersType path(dimensions.size());

			size_t last = storage->size();    

			for (size_t i = 0; i < last; i++) {
				Cube::CellValueType * value;

				// build cell path list for cell/area request
				if (buildCellPathes) {
					value = storage->getCell(i, &path);
					cellPathes->push_back(path);
				}

				// use cell path list from cell/values request
				else {
					value = storage->getCell(i);
					path = (*cellPathes)[i];
				}

				try {

					// check cell path (throws exception)
					CellPath cellPath(this, &path); 

					if (!value) {
						Logger::error << "Cube::getCellValuesFromCache internal error: value not found" << endl;
						continue;
					}

					bool found = false;
					set< pair<Rule*, IdentifiersType> > ruleHistory;
					*value = getCellValue(&cellPath, &found, user, 0, &ruleHistory, useEnterpriseRules);

					if (!found && value->type == Element::NUMERIC) {
						value->type = Element::UNDEFINED;         
						value->doubleValue = 0.0; 
					}

				}      
				catch (ErrorException e) {
					value->type = Element::UNDEFINED;
					value->doubleValue = e.getErrorType();
					value->charValue.assign(e.getMessage());
				}
			}

			Logger::trace << "getCellValuesFromCache finished" << endl;
	}



	// cellPath spans a cube, which contains a rule but is NOT itself defined by a rule

	Cube::CellValueType Cube::getConsolidatedRuleValue (const CellPath* cellPath,
		bool* found,
		bool* isCachable,
		set< pair<Rule*, IdentifiersType> >* ruleHistory) {
			Logger::trace << "started getConsolidatedRuleValue" << endl;

			PathType path = *(cellPath->getPathElements());
			*found = false;
			*isCachable = true;

			// find first element which is a consolidation
			PathType::iterator iter = path.begin();
			vector<Dimension*>::const_iterator dimIter = dimensions.begin();

			for (;  iter != path.end();  iter++, dimIter++) {
				Element * element = *iter;

				if (element->getElementType() == Element::CONSOLIDATED) {
					break;
				}
			}

			if (iter == path.end()) {
				throw ErrorException(ErrorException::ERROR_INTERNAL,
					"in getConsolidatedRuleValue");
			}

			double value = 0;
			const ElementsWeightType * children = (*dimIter)->getChildren(*iter);

			if (children != 0) {
				for (ElementsWeightType::const_iterator childIter = children->begin();
					childIter != children->end();
					childIter++) {
						Element* child = childIter->first;
						double factor = childIter->second;

						bool found2;
						bool isCachable2;

						*iter = child;
						CellPath cp(this, &path);

						CellValueType r = getCellValue(&cp, &found2, 0, 0, ruleHistory, true, &isCachable2);

						if (found2) {
							value += factor * r.doubleValue;
						}

						*found = *found || found2;
						*isCachable = *isCachable && isCachable2;
				}
			}

			CellValueType result;
			result.type = Element::NUMERIC;
			result.doubleValue = value;
			result.rule = Rule::NO_RULE;

			Logger::trace << "finished getConsolidatedRuleValue" << endl;

			return result;
	}



	void Cube::getExportValues (ExportStorage* storage, 
		vector<IdentifiersType>* area,  
		vector<IdentifiersType>* resultPathes,
		IdentifiersType* startPath,
		bool hasStartPath,
		bool useRules,
		bool baseElementsOnly,
		bool skipEmpty,
		Condition * condition,
		User* user) {

			//Logger::trace << "Cube::getExportValues" << endl;

			resultPathes->clear();

			vector<IdentifiersType> baseElementsArea;
			vector< map<IdentifierType, map<IdentifierType, double> > > baseElements;

			// build up mapping without consolidated elements and rebuild area
			if (baseElementsOnly) {
				bool notEmpty = computeBaseElementsOnly(area, &baseElementsArea, &baseElements, false);

				if (! notEmpty) {
					// area is empty 
					return;
				}      

				// copy new area
				*area = baseElementsArea;

				// reset area in export storage
				storage->setArea(area);
			}

			// get numeric base elements mapping
			else {
				computeAreaBaseElements(area, &baseElements, false);

				for (vector<IdentifiersType>::iterator i = area->begin(); i != area->end(); i++) {
					if (i->size() == 0) {
						// area is empty
						return;
					}
				}
			}  

			if (hasStartPath) {
				bool ok = storage->setStartPath(startPath);

				if (!ok) {
					throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
						"could not set start path for cell export",
						"path", 0);
				}      
			}

			bool checkUserRights = false;    

			if (user && getMinimumAccessRight(user) < RIGHT_READ) {
				checkUserRights = true;
			}

			// conditions and user rights might reduce the number of elements in the storage
			// so use a bigger storage to reduce the number of loops
			if (checkUserRights || condition) {
				storage->increasePage();
			}

			bool useEnterpriseRules = !rules.empty() && useRules;
			bool empty = (sizeFilledCells() == 0);

			bool stop = false;
			while (!stop) {

				if (!skipEmpty) {
					storage->fillWithEmptyElements();
				}

				if (!empty) {

					// fill numeric values (adds elements to storage)
					storageDouble->computeExportCellValue(storage, &baseElements, baseElementsOnly, user);

					// fill string values (adds elements to storage)					
					storageString->computeExportStringCellValue(storage, area);
				}

				// fill rule values (adds elements to storage)
				if (useEnterpriseRules) {
					getExportRuleValues(storage, area, user);
				}


				// check conditions (removes elements in storage!) 
				if (condition) {
					storage->processCondition(condition);
				}

				// check permissions (removes elements in storage!)
				if (checkUserRights) {
					checkExportPathAccessRight(storage, user);
				}

				// stop if storage has blocksize elements or 
				// storage could not be reseted for next loop
				stop = storage->hasBlocksizeElements() || !storage->resetForNextLoop();      
			}
	} 



	void Cube::getExportRuleValues (ExportStorage* storage, vector<IdentifiersType>* area, User * user) {

		checkNewMarkerRules();

		size_t last = storage->size();
		IdentifiersType path(area->size());
		for (size_t i = 0; i < last; i++) {
			Cube::CellValueType* ovalue = storage->getCell(i, &path); 
			CellPath cellPath(this, &path);
			// find value in cache
			CellValueType value;     
			bool found = false;
			bool foundInCache = false;

			if (cacheRulesStorage) {
				if (findInRuleCache(cacheRulesStorage, &cellPath, value, &found)) {
					CellValueType * ptr = new CellValueType();
					ptr->type = value.type;
					ptr->doubleValue = value.doubleValue;
					ptr->charValue = value.charValue;
					ptr->rule = value.rule;
					storage->setCellValue(&path, (CubePage::value_t) &ptr);
					foundInCache = true;
				}
			}

			bool isCachable = true;
			bool isRule = false;

			set< pair<Rule*, IdentifiersType> > ruleHistory;

			if (! foundInCache) {
				if (findDirectRuleMatch(database, rules, cacheRulesStorage, &cellPath, user, value, &found, &isCachable, &ruleHistory)) {
					CellValueType * ptr = new CellValueType();
					ptr->type = value.type;
					ptr->doubleValue = value.doubleValue;
					ptr->charValue = value.charValue;
					ptr->rule = value.rule;
					storage->setCellValue(&path, (CubePage::value_t) &ptr);
					isRule = true;
				}

				if (! isRule
					&& cellPath.getPathType() == Element::CONSOLIDATED
					&& findIndirectRuleMatch(database, this, rules, cacheRulesStorage, &cellPath, value, &found, &isCachable, &ruleHistory)) {
						CellValueType * ptr = new CellValueType();
						ptr->type = value.type;
						ptr->doubleValue = value.doubleValue;
						ptr->charValue = value.charValue;
						ptr->rule = value.rule;
						storage->setCellValue(&path, (CubePage::value_t) &ptr);
				}
			}
		}

			//checkNewMarkerRules();

			//IdentifiersType path(* (storage->getFirstPath()) );

			//size_t max = area->size();
			//vector< IdentifiersType::iterator > iv(max);

			//for (size_t i = 0; i < max; i++) {
			//	iv[i] = area->at(i).begin();
			//	while ( iv[i] != area->at(i).end() &&  *(iv[i]) != path[i]) {
			//		iv[i]++;
			//	}
			//}

			//size_t pos = max-1;

			//do {
			//	try {
			//		CellPath cellPath(this, &path);
			//		bool found = false;

			//		// find value in cache
			//		CellValueType value;                
			//		bool foundInCache = false;

			//		if (cacheRulesStorage) {
			//			if (findInRuleCache(cacheRulesStorage, &cellPath, value, &found)) {
			//				CellValueType * ptr = new CellValueType();
			//				ptr->type = value.type;
			//				ptr->doubleValue = value.doubleValue;
			//				ptr->charValue = value.charValue;
			//				ptr->rule = value.rule;
			//				storage->setCellValue(&path, (CubePage::value_t) &ptr);
			//				foundInCache = true;
			//			}
			//		}

			//		bool isCachable = true;
			//		bool isRule = false;

			//		set< pair<Rule*, IdentifiersType> > ruleHistory;

			//		if (! foundInCache) {
			//			if (findDirectRuleMatch(database, rules, cacheRulesStorage, &cellPath, user, value, &found, &isCachable, &ruleHistory)) {
			//				CellValueType * ptr = new CellValueType();
			//				ptr->type = value.type;
			//				ptr->doubleValue = value.doubleValue;
			//				ptr->charValue = value.charValue;
			//				ptr->rule = value.rule;
			//				storage->setCellValue(&path, (CubePage::value_t) &ptr);
			//				isRule = true;
			//			}

			//			if (! isRule
			//				&& cellPath.getPathType() == Element::CONSOLIDATED
			//				&& findIndirectRuleMatch(database, this, rules, cacheRulesStorage, &cellPath, value, &found, &isCachable, &ruleHistory)) {
			//					CellValueType * ptr = new CellValueType();
			//					ptr->type = value.type;
			//					ptr->doubleValue = value.doubleValue;
			//					ptr->charValue = value.charValue;
			//					ptr->rule = value.rule;
			//					storage->setCellValue(&path, (CubePage::value_t) &ptr);
			//			}
			//		}
			//	}
			//	catch (ParameterException e) {
			//		// ignore cell        
			//	}

			//	if (path == *(storage->getLastPath()) ) {
			//		return;
			//	}

			//	if (false) {
			//		cout << "path < ";
			//		for (size_t i = 0; i < max; i++) {
			//			cout << path[i] << " ";
			//		}
			//		cout << "> " << endl;
			//	}

			//	pos = max-1;
			//	bool found = false;

			//	while (!found && pos >= 0 && pos < max) {
			//		iv[pos]++;
			//		if (iv[pos] != area->at(pos).end()) {
			//			path[pos] = * (iv[pos]);
			//			found = true;
			//		}
			//		else {
			//			iv[pos] = area->at(pos).begin();
			//			path[pos] = * (iv[pos]);
			//			pos--;
			//		}
			//	}
			//}
			//while (pos >= 0 && pos < max);
		}

		void Cube::cellGoalSeek(CellPath* cellPath, User* user, PaloSession * session, const double& value) 
		{
			if (cellPath->getPathType() != Element::NUMERIC && cellPath->getPathType() != Element::CONSOLIDATED) {
				throw ParameterException(ErrorException::ERROR_INVALID_ELEMENT_TYPE,
					"cannot goal seek string path",
					"destination element type", cellPath->getPathType());
			}

			// check minimal access rights, we might need splashing - check is done later
			checkPathAccessRight(user, cellPath, RIGHT_READ);
			checkPathAccessRight(user, cellPath, RIGHT_WRITE);

			const vector<Element*>& pathElement = *cellPath->getPathElements();
			vector<int> gsDim;
			vector<Element*> gsParent;
			vector<ElementsWeightType > gsElements;
			vector<vector<int> > gsElementIndex;
			vector<map<int, int> > gsElementIdentiferIndex;
			vector<IdentifiersType> area(dimensions.size());

			bool splash = false;

			for (size_t i = 0; i<pathElement.size(); i++)
			{                 
				IdentifiersType ids;
				const std::vector<Element*>& p = *(dimensions[i]->getParents(pathElement[i]));
				if (p.size()>1) {
					std::string parentsString;
					for (size_t j = 0; j<p.size(); j++)
					{
						if (j>0)
							parentsString+=", ";
						parentsString+="'"+p[j]->getName()+"'";
					}
					throw ErrorException(ErrorException::ERROR_GOALSEEK, "element '"+pathElement[i]->getName()+"' has multiple parents ("+parentsString+")");
				}
				if (p.size()==1) {
					const ElementsWeightType& c = *(dimensions[i]->getChildren(p[0]));
					ElementsWeightType vc;
					std::vector<int> iv;
					map<int, int> iim;
					for (size_t j = 0; j<c.size(); j++) {

						if (c[j].first->getElementType() != Element::NUMERIC && c[j].first->getElementType() != Element::CONSOLIDATED) 
							continue;

						if (c[j].first->getElementType() == Element::CONSOLIDATED) 
							splash = true;

						//check for multiple parents
						const std::vector<Element*>& cp = *(dimensions[i]->getParents(c[j].first));
						if (cp.size()>1) {
							std::string parentsString;
							for (size_t k = 0; k<cp.size(); k++)
							{
								if (k>0)
									parentsString+=", ";
								parentsString+="'"+cp[k]->getName()+"'";
							}
							throw ErrorException(ErrorException::ERROR_GOALSEEK, "element '"+c[j].first->getName()+"' has multiple parents ("+parentsString+")");
						}                                   

						vc.push_back(c[j]);
						iv.push_back(j);
						iim[c[j].first->getIdentifier()] = j;
						ids.push_back(c[j].first->getIdentifier());
					}
					if (vc.size()==0) 
						throw ErrorException(ErrorException::ERROR_GOALSEEK, "dimension element has no numeric children");
					gsElements.push_back(vc);     
					gsElementIndex.push_back(iv);
					gsElementIdentiferIndex.push_back(iim);
					gsDim.push_back((int)i);                                      
				}                               
				else {
					ids.push_back(pathElement[i]->getIdentifier());
					splash |= (pathElement[i]->getElementType() == Element::CONSOLIDATED);

				}
				area[i] = ids;
			}

			//check splash rights
			if (getType() != USER_INFO && splash && user && (user->getRoleCellDataRight() < RIGHT_SPLASH)) 
				throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
				"insufficient access rights for path",
				"user", (int) user->getIdentifier());            

			if (gsDim.size()>0) {

				uint32_t numResult = 1;
				for (size_t i = 0; i<area.size(); i++)
					numResult*=area[i].size();

				if (numResult>(uint32_t)goalseekCellLimit) 
					throw ErrorException(ErrorException::ERROR_GOALSEEK, "slice to big ("+StringUtils::convertToString(numResult)+", max allowed: "+StringUtils::convertToString(goalseekCellLimit)+")");

				AreaStorage storage(&dimensions, &area, numResult, false);

				vector<IdentifiersType> resultPaths;
				bool storageFilled = this->getAreaCellValues(&storage, &area, &resultPaths, user);

				if (storageFilled) {              

					//define problem
					goalseeksolver::Problem p;
					std::vector<int> d;
					for (size_t i = 0; i<gsElements.size(); i++) {
						vector<double> w;
						for (size_t j = 0; j<gsElements[i].size(); j++)
							w.push_back(gsElements[i][j].second);
						p.dimensionElementWeight.push_back(w);
						d.push_back((int)w.size());
					}

					const IdentifiersType* path = cellPath->getPathIdentifier();
					vector<int> coord; coord.resize(gsDim.size());
					for (size_t gsd = 0; gsd<gsDim.size(); gsd++) {
						int ind = gsElementIdentiferIndex[gsd][(*path)[gsDim[gsd]]];
						coord[gsd] = ind;                                     
					}

					p.fixedValue = value;
					p.fixedCoord = coord;

					p.cellValue = goalseeksolver::MDM<double>(d);


					size_t last = storage.size();
					vector<IdentifiersType>::iterator iter = resultPaths.begin();

					for (size_t i = 0; i < last; i++, iter++) {
						Cube::CellValueType* value = storage.getCell(i); 

						for (size_t gsd = 0; gsd<gsDim.size(); gsd++) {
							int ind = gsElementIdentiferIndex[gsd][(*iter)[gsDim[gsd]]];
							coord[gsd] = ind;                                   
						}
						p.cellValue[coord] = value->doubleValue;
					}    

					goalseeksolver::Result res;
					try {
						res = goalseeksolver::solve(p, goalseekTimeoutMiliSec);
					} catch (goalseeksolver::CalculationTimeoutException&) {
						throw ErrorException(ErrorException::ERROR_GOALSEEK, "calculation takes too long");
					}
					if (res.valid) {
						iter = resultPaths.begin();

						for (size_t i = 0; i < last; i++, iter++) {
							Cube::CellValueType* value = storage.getCell(i); 

							for (size_t gsd = 0; gsd<gsDim.size(); gsd++) {
								int ind = gsElementIdentiferIndex[gsd][(*iter)[gsDim[gsd]]];
								coord[gsd] = ind;                                 
							}
							CellPath cp(this, &(*iter));
							setCellValue(&cp, res.cellValue[coord], user, session, false, false, false, Cube::DEFAULT, Lock::checkLock); 
						}  
					} else
						throw ErrorException(ErrorException::ERROR_GOALSEEK, "could not find valid solution");
				}
			} else {
				setCellValue(cellPath, value, user, session, false, false, false, Cube::DEFAULT, Lock::checkLock);
			}

		}

		bool Cube::fillCacheByHistory (PaloSession* session) {
			const vector< IdentifiersType >* history = session->getHistory();

			// copy history to sets
			vector< set<IdentifierType> > ids(dimensions.size());
			size_t max = dimensions.size();
			for (vector< IdentifiersType >::const_iterator path = history->begin(); path != history->end(); path++) {      
				for (size_t i = 0; i < max; i++) {
					ids[i].insert(path->at(i));
				}
			}      

			// guess an area
			vector< IdentifiersType > area;
			vector< Dimension* >::iterator dimIter = dimensions.begin();
			double num = 1.0; 
			for (vector< set<IdentifierType> >::iterator idSet = ids.begin(); idSet != ids.end(); idSet++, dimIter++) {

				IdentifiersType tmpVector;

				if (idSet->size() > 1) {
					// dimension has more than one element
					// add all siblings and parents
					set<IdentifierType> tmpSet;

					for (set<IdentifierType>::iterator id = idSet->begin(); id != idSet->end(); id++) {
						addSiblingsAndParents(&tmpSet, *dimIter, *id);
					}

					tmpVector.resize(tmpSet.size());
					copy(tmpSet.begin(),tmpSet.end(),tmpVector.begin());
				}
				else {
					// dimension has only one element
					tmpVector.push_back(*(idSet->begin()));
				}
				area.push_back(tmpVector);
				num *= tmpVector.size();
			}  

			// check size of area
			if (num >= AreaStorage::MAXIMUM_AREA_SIZE) {
				// guess a smaller area

				Logger::debug << "too many elements (" << num << ") in guessed cell/value area. Using smaller area!" << endl;

				area.clear();
				dimIter = dimensions.begin();
				num = 1.0; 
				for (vector< set<IdentifierType> >::iterator idSet = ids.begin(); idSet != ids.end(); idSet++, dimIter++) {

					IdentifiersType tmpVector;

					if (idSet->size() > 1) {          
						tmpVector.resize(idSet->size());
						copy(idSet->begin(),idSet->end(),tmpVector.begin());
					}
					else {
						// dimension has only one element
						tmpVector.push_back(*(idSet->begin()));
					}
					area.push_back(tmpVector);
					num *= tmpVector.size();
				}
				session->removeHistory();
			}    

			// check size of area
			if (num >= AreaStorage::MAXIMUM_AREA_SIZE) {
				// stop! too many elements
				Logger::info << "too many elements (" << num << ") in guessed cell/value area. Cannot fill cache for cell/value requests" << endl;
				// session->removeHistory();
				return false; 
			}

			size_t numResult = (size_t) num;

			if (false) {
				cout << "area= ";
				size_t maxDimension = area.size();
				for (size_t i = 0; i < maxDimension; i++) {
					if (i > 0) cout << ",";
					size_t maxElement = area.at(i).size();
					for (size_t j = 0; j < maxElement; j++) {
						if (j > 0) cout << ":";
						cout << area.at(i).at(j);
					}
				}
				cout << endl;
			}

			// get base elements of area and create an area storage
			AreaStorage storage(&dimensions, &area, numResult, false);

			// get values of the area and update the cache (in function getCellValues) 
			vector<IdentifiersType> cellPathes;
			getCellValues(&storage, &area, &cellPathes, true, 0);

			return true;
		}

		////////////////////////////////////////////////////////////////////////////////
		// other stuff
		////////////////////////////////////////////////////////////////////////////////

		double Cube::computeRule (CubePage::key_t key, double dflt, User* user) {
			try {
				CellPath cellPath(this, (uint32_t*) key);
				CellValueType cellValue;
				bool found;

				cellValue.doubleValue = 0;

				// check if we have cached a rule
				if (cacheRulesStorage) {
					if (findInRuleCache(cacheRulesStorage, &cellPath, cellValue, &found)) {
						return cellValue.doubleValue;
					}
				}

				// try enterprise rules direct match
				bool isCachable;
				set< pair<Rule*, IdentifiersType> > ruleHistory;

				if (findDirectRuleMatch(database, rules, cacheRulesStorage, &cellPath, user, cellValue, &found, &isCachable, &ruleHistory)) {
					return cellValue.doubleValue;
				}
			}
			catch (const ErrorException& ex) {
				if (DEBUG_FILE) {
					Logger::trace << "cannot evaluate rule: " << ex.getMessage() << " (" << ex.getDetails() << ")" << endl;
				}
			}

			return dflt;
		}



		void Cube::updateToken () {
			token++;
			database->updateToken();
		}



		void Cube::updateClientCacheToken () {
			clientCacheToken++;
		}



		Rule* Cube::createRule (RuleNode* node, const string& external, const string& comment, bool activate, User* user) {
			if (status == UNLOADED) {
				throw ParameterException(ErrorException::ERROR_CUBE_NOT_LOADED,
					"cube not loaded",
					"cube", (int) identifier);
			}

			checkCubeRuleRight(user, RIGHT_WRITE);    

			updateToken();

			// find next identifier
			maxRuleIdentifier++;
			IdentifierType id = maxRuleIdentifier;

			Rule* rule = new Rule(id, this, node, external, comment, activate);
			rules.push_back(rule);

			saveCubeRules();

			clearRulesCache();

			updateClientCacheToken();

			// the marker areas will be updated later
			if (rule->hasMarkers()) {
				newMarkerRules.insert(rule);
			}

			return rule;
		}



		void Cube::modifyRule (Rule* rule, RuleNode* node, const string& external, const string& comment, User* user) {
			if (status == UNLOADED) {
				throw ParameterException(ErrorException::ERROR_CUBE_NOT_LOADED,
					"cube not loaded",
					"cube", (int) identifier);
			}

			checkCubeRuleRight(user, RIGHT_WRITE);

			updateToken();

			// update rule definition
			rule->setDefinition(node);
			rule->setExternal(external);
			rule->setComment(comment);

			saveCubeRules();

			clearRulesCache();

			// the marker areas will be updated later
			if (rule->hasMarkers()) {
				newMarkerRules.insert(rule);
			}
		}



		void Cube::activateRule (Rule* rule, bool activate, User* user) {
			if (status == UNLOADED) {
				throw ParameterException(ErrorException::ERROR_CUBE_NOT_LOADED,
					"cube not loaded",
					"cube", (int) identifier);
			}

			checkCubeRuleRight(user, RIGHT_WRITE);

			if (rule->isActive() == activate) {
				// nothing to do
				return;
			}

			updateToken();

			rule->setActive(activate);

			saveCubeRules();

			clearRulesCache();

			// the marker areas will be updated later
			if (activate) {
				if (rule->hasMarkers()) {
					newMarkerRules.insert(rule);
				}
			}

			// remove marker now
			else {
				if (rule->hasMarkers()) {
					rule->removeMarkers();
				}
			}
		}



		void Cube::deleteRule (IdentifierType id, User* user) {
			checkCubeRuleRight(user, RIGHT_DELETE);

			for (vector<Rule*>::iterator iter = rules.begin();  iter != rules.end();  iter++) {
				Rule* rule = *iter;

				if (rule->getIdentifier() == id) {
					rules.erase(iter);

					saveCubeRules();

					clearRulesCache();

					updateClientCacheToken();

					if (rule->hasMarkers()) {
						rule->removeMarkers();
					}

					delete rule;

					return;
				}
			}

			throw ParameterException(ErrorException::ERROR_RULE_NOT_FOUND,
				"rule not found in cube",
				"rule", (int) id);
		}



		Rule* Cube::findRule (IdentifierType id, User*) {
			for (vector<Rule*>::iterator iter = rules.begin();  iter != rules.end();  iter++) {
				Rule* rule = *iter;

				if (rule->getIdentifier() == id) {
					return rule;
				}
			}

			throw ParameterException(ErrorException::ERROR_RULE_NOT_FOUND,
				"rule not found in cube",
				"rule", (int) id);
		}

		////////////////////////////////////////////////////////////////////////////////
		// helper functions
		////////////////////////////////////////////////////////////////////////////////

		double Cube::countBaseElements (const CellPath * path) {

			// loop over dimensions
			const PathType * elements = path->getPathElements();

			vector<Dimension*>::const_iterator dimensionIter = dimensions.begin();
			vector<Element*>::const_iterator   elementIter   = elements->begin();

			double numBaseCells = 1.0;

			for (;  dimensionIter != dimensions.end();  elementIter++, dimensionIter++) {
				numBaseCells *= (*elementIter)->getNumBaseElements(*dimensionIter);          
			}

			return  numBaseCells;
		}



		double Cube::computeBaseElements (const CellPath * path, vector<IdentifiersWeightType> * baseElements, bool ignoreNullWeight) {

			// setup the size of result vector
			baseElements->resize(dimensions.size());

			// loop over dimensions
			const PathType * elements = path->getPathElements();

			vector<Dimension*>::const_iterator dimensionIter = dimensions.begin();
			vector<Element*>::const_iterator   elementIter   = elements->begin();

			vector<IdentifiersWeightType>::iterator baseIter = baseElements->begin();

			double numBaseCells = 1.0;

			if (ignoreNullWeight) {
				for (;  dimensionIter != dimensions.end();  elementIter++, baseIter++, dimensionIter++) {
					const map<IdentifierType, double>* baseElements = (*elementIter)->getBaseElements(*dimensionIter);      

					for (map<IdentifierType, double>::const_iterator c = baseElements->begin(); c != baseElements->end(); c++) {
						if (c->second != 0.0) {
							baseIter->push_back( make_pair(c->first, c->second) );
						}
					}

					numBaseCells *=  baseIter->size();          
				}
			}
			else {
				for (;  dimensionIter != dimensions.end();  elementIter++, baseIter++, dimensionIter++) {
					const map<IdentifierType, double>* baseElements = (*elementIter)->getBaseElements(*dimensionIter);      

					for (map<IdentifierType, double>::const_iterator c = baseElements->begin(); c != baseElements->end(); c++) {
						baseIter->push_back( make_pair(c->first, c->second) );
					}            

					numBaseCells *=  baseIter->size();          
				}
			}

			return  numBaseCells;
		}



		double Cube::computeBaseElements (const CellPath * path, vector< set<IdentifierType> > * baseElements) {

			// setup the size of result vector
			baseElements->resize(dimensions.size());

			// loop over dimensions
			const PathType * elements = path->getPathElements();

			vector<Dimension*>::const_iterator dimensionIter = dimensions.begin();
			vector<Element*>::const_iterator   elementIter   = elements->begin();

			vector< set<IdentifierType> >::iterator baseIter = baseElements->begin();

			double result = 1.0; 
			for (;  dimensionIter != dimensions.end();  elementIter++, baseIter++, dimensionIter++) {
				const map<IdentifierType, double>* baseElements = (*elementIter)->getBaseElements(*dimensionIter);      
				for (map<IdentifierType, double>::const_iterator c = baseElements->begin(); c != baseElements->end(); c++) {
					baseIter->insert(c->first);
				}
				result *= baseIter->size();
			}

			return result;
		}



		void Cube::deleteNumericBasePath (const vector<ElementsWeightType> * baseElements) {
			// temporary vector for one path
			PathType newPath(baseElements->size());

			// delete base paths recursivly
			deleteNumericBasePathRecursive(0, baseElements->begin(), &newPath);
		}



		void Cube::deleteNumericBasePathRecursive (size_t position,
			vector<ElementsWeightType>::const_iterator baseElementsIter,
			PathType * newPath) {

				// last dimension reached
				if (position == newPath->size()) {      
					storageDouble->deleteCell(newPath);      
				}

				// fill next dimension
				else {      
					ElementsWeightType::const_iterator elementIterator = baseElementsIter->begin();

					// loop over all elements of next dimension 
					for (; elementIterator !=  baseElementsIter->end();  elementIterator++) {
						Element * e = elementIterator->first;

						if (e->getElementType() != Element::NUMERIC) {
							return;
						}

						(*newPath)[position] = e;

						deleteNumericBasePathRecursive(position + 1, baseElementsIter + 1, newPath);
					}
				}
		}




		void Cube::setBaseCellValue (const PathType * path, double value) {
			storageDouble->setCellValue(path, (uint8_t*) &value, false);
		}



		void Cube::setBaseCellValue (const PathWeightType * path, double value) {
			storageDouble->setCellValue(path, (uint8_t*) &value, false);
		}


		void Cube::setBaseCellValue (const IdentifiersType * path, double value) {
			storageDouble->setCellValue(path, (uint8_t*) &value, false);
		}


		void Cube::setBaseCellValue (const PathType * path, const string& value) {
			const char * v = value.c_str();
			size_t n = strlen(v) + 1;

			// the storage will take ownership of this allocated memory block
			char * c = new char [n];

			*c = 0;
			strncat(c, v, n - 1);

			storageString->setCellValue(path, (uint8_t*) &c, false);
		}



		void Cube::setBaseCellValue (const IdentifiersType * path, const string& value) {
			const char * v = value.c_str();
			size_t n = strlen(v) + 1;

			// the storage will take ownership of this allocated memory block
			char * c = new char [n];

			*c = 0;
			strncat(c, v, n - 1);

			storageString->setCellValue(path, (uint8_t*) &c, false);
		}

		void Cube::deleteElement (const string& username,
			const string& event,
			Dimension* dimension,
			IdentifierType element,
			bool processStorageDouble,
			bool processStorageString,
			bool deleteRules) {
				IdentifiersType elements;
				elements.push_back(element);
				deleteElements(username, event, dimension, elements, processStorageDouble, processStorageString, deleteRules);
		}


		void Cube::deleteElements (const string& username,
			const string& event,
			Dimension* dimension,
			IdentifiersType elements,
			bool processStorageDouble,
			bool processStorageString,
			bool deleteRules) {

				IdentifiersType path;
				IdentifiersType mask;		

				size_t ind = -1;

				for (size_t i = 0;  i != dimensions.size();  i++) {
					if (dimensions[i] == dimension) {	
						mask.push_back(1);
						ind = i;
					}
					else {

						mask.push_back(0);
					}
					path.push_back(0);				
				}

				if (ind!=-1) {
					updateToken();
					updateClientCacheToken();

					for (size_t i = 0; i<elements.size(); i++) 
					{
						IdentifierType element = elements[i];
						path[ind] = element;

						if (journal != 0) {
							journal->appendCommand(username, event, "DELETE_ELEMENT");
							journal->appendIdentifier(dimension->getIdentifier());
							journal->appendIdentifier(element);
							journal->appendBool(processStorageDouble);
							journal->appendBool(processStorageString);
							journal->appendBool(deleteRules);
							journal->nextLine();
						}

						if (status != UNLOADED) {

							// delete elements in storage
							if (processStorageDouble) {
								storageDouble->deleteByMask(&path, &mask);
							}

							if (processStorageString) {
								storageString->deleteByMask(&path, &mask);
							}

							if (deleteRules) {
								// delete rules containing the element
								vector<Rule*> del;

								for (vector<Rule*>::iterator iter = rules.begin();  iter != rules.end();  iter++) {
									Rule* rule = *iter;

									if (rule->hasElement(dimension, element)) {
										del.push_back(rule);
									}
								}

								for (vector<Rule*>::iterator iter = del.begin();  iter != del.end();  iter++) {
									deleteRule((*iter)->getIdentifier(), 0);
								}
							}

							removeCachedCells(dimension, element);
						}

						// cube has been changed
						status = CHANGED;
					}
				}
		}



		void Cube::checkPathAccessRight (User* user, const CellPath* cellPath, RightsType minimumRight) {
			if (user == 0) {
				return;
			}

			// check role "cell data" right
			if (user->getRoleCellDataRight() < minimumRight) {
				throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
					"insufficient access rights for path",
					"user", (int) user->getIdentifier());
			}

			// check cube data right
			if (user->getCubeDataRight(database, this) < minimumRight) {
				throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
					"insufficient access rights for path",
					"user", (int) user->getIdentifier());
			}

			// check dimension data right for all dimensions
			vector<Dimension*>::iterator dim = dimensions.begin();
			vector<Element*>::const_iterator element = (cellPath->getPathElements())->begin();

			for (; dim != dimensions.end(); dim++, element++) {
				if (user->getDimensionDataRight(database, *dim, *element) < minimumRight) {
					throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
						"insufficient access rights for path",
						"user", (int) user->getIdentifier());
				}
			}
		}



		RightsType Cube::getMinimumAccessRight (User* user) {
			if (user == 0) {
				return RIGHT_SPLASH;
			}

			RightsType result    = user->getRoleCellDataRight();
			RightsType dataRight = user->getCubeDataRight(database, this);

			if (dataRight < result) {
				result = dataRight;
			}

			RightsType minimumDimRight;
			vector<Dimension*>::iterator dim = dimensions.begin();
			for (; dim != dimensions.end(); dim++) {
				minimumDimRight = user->getMinimumDimensionDataRight(database, *dim);      
				if (minimumDimRight < result) {
					result = minimumDimRight;
				}      
			}

			return result;
		}



		void Cube::checkSepRight (User* user, RightsType minimumRight) {
			if (user == 0) {
				return;
			}

			// check role "event processor" right
			if (user->getRoleEventProcessorRight() < minimumRight) {
				throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
					"insufficient access rights for disabling event processor",
					"user", (int) user->getIdentifier());
			}
		}



		void Cube::makeOrderedElements (ElementsWeightType* elementsWeight) {
			ElementsWeightType::iterator i = elementsWeight->begin();
			ElementsWeightType::iterator end = elementsWeight->end();

			map<IdentifierType, int> elementToPos;
			map<IdentifierType, int>::iterator posIter;

			ElementsWeightType::iterator j = i;
			int pos = 0;
			while (i != end) {
				elementToPos[j->first->getIdentifier()] = pos;
				if (i != j) {
					*j = *i;
				}

				i++;


				bool found = true;
				while (i != end && found) {
					posIter = elementToPos.find(i->first->getIdentifier());      
					if (posIter != elementToPos.end()) {
						elementsWeight->at(posIter->second).second += i->second; 
						i++;
					}
					else {
						found = false;
					}
				}

				j++;
				pos++;
			}

			if (j != end) {
				elementsWeight->resize(j - elementsWeight->begin());
			}
		}



		bool Cube::isEqualBaseElements (vector<ElementsWeightType>* e1, vector<ElementsWeightType>* e2) {
			vector<ElementsWeightType>::iterator from = e1->begin();
			vector<ElementsWeightType>::iterator to   = e2->begin();

			for (; from != e1->end(); from++, to++) {
				// check size of vector
				if (from->size() != to->size()) {
					return false;
				}

				// check every element type and weight
				ElementsWeightType::iterator fromElement = from->begin();
				ElementsWeightType::iterator toElement   = to->begin();      
				for (;fromElement != from->end(); fromElement++, toElement++) {
					// check element type
					if (fromElement->first->getElementType() != toElement->first->getElementType()) {
						return false;
					}

					// check weight
					if (fromElement->second != toElement->second) {
						return false;
					}

				}      

			}
			return true;
		}



		void Cube::setWorkerAreas (vector<string>* areaIdentifiers, vector< vector< IdentifiersType > >* areas) {
			if (areas->size()==0) {
				return;
			}

			if (areas->size()!=areaIdentifiers->size()) {
				Logger::error << "Size of identifiers not equal to size of areas." << endl;
				return;
			}

			hasArea = true;
			workerAreas.clear();
			workerAreaIdentifiers = *areaIdentifiers;

			for (vector< vector< IdentifiersType > >::iterator is = areas->begin(); is != areas->end(); is++) {
				vector< set<IdentifierType> > area;

				for (vector< IdentifiersType >::iterator i = is->begin(); i != is->end(); i++) {
					// copy each identifier to a set
					// (empty set means all elements are in the area)
					set<IdentifierType> newSet;      
					for (IdentifiersType::iterator j = i->begin(); j != i->end(); j++) {
						newSet.insert(*j);
					}
					area.push_back(newSet);      
				}

				workerAreas.push_back(area);
			}
		}



		bool Cube::isInArea (CellPath* cellPath, vector< set<IdentifierType> >* area) {
			const IdentifiersType * path = cellPath->getPathIdentifier();
			IdentifiersType::const_iterator elementsPathIter = path->begin();
			vector< set<IdentifierType> >::iterator areaIter =  area->begin();

			for (; areaIter != area->end(); elementsPathIter++, areaIter++) {      
				if (areaIter->size() > 0) {

					// set is not empty (empty set means all elements are in the area)
					set<IdentifierType>::iterator element = areaIter->find(*elementsPathIter);

					// element not found in set so the cellPath is not in area
					if (element == areaIter->end()) {
						return false;
					}        
				}      
			}

			return true;    
		}      



		bool Cube::isInArea (CellPath* cellPath, string& areaIdentifier) {
			if (!hasArea) {
				return false;
			}


			vector< string >::iterator areaId =  workerAreaIdentifiers.begin();
			vector< vector< set<IdentifierType> > >::iterator areaIter =  workerAreas.begin();

			for (; areaIter != workerAreas.end(); areaIter++, areaId++) {
				bool in = isInArea(cellPath, &(*areaIter));
				if (in) {
					Logger::trace << "request is in area '" << *areaId << "'" << endl;
					areaIdentifier = *areaId;
					return true;
				}
			}

			return false;    
		}      



		bool Cube::isInArea (const IdentifierType* path, const vector< set<IdentifierType> >* area) {
			vector< set<IdentifierType> >::const_iterator areaIter =  area->begin();

			for (; areaIter != area->end(); path++, areaIter++) {      
				if (areaIter->size() > 0) {

					// set is not empty (empty set means all elements are in the area)
					set<IdentifierType>::const_iterator element = areaIter->find(*path);

					// element not found in set so the cellPath is not in area
					if (element == areaIter->end()) {
						return false;
					}        
				}      
			}

			return true;    
		}      



		void Cube::sendExecuteShutdown () {
			if (cubeWorker) {
				cubeWorker->executeShutdown();
			}
		}



		void Cube::removeWorker () {
			if (cubeWorker) {
				Logger::trace << "removing worker of cube '" << name << "'" << endl;      
				delete cubeWorker;      
				cubeWorker = 0;      
			}
		}


		void Cube::getParentElements(Dimension* dimension, Element* child, set<IdentifierType>* parents) {
			const vector<Element*>* p  = dimension->getParents(child);
			for (vector<Element*>::const_iterator i = p->begin(); i != p->end(); i++) {
				parents->insert((*i)->getIdentifier());
				getParentElements(dimension, *i, parents);
			}    
		}


		bool Cube::isInvalid () {
			if (cacheConsolidationsStorage && cacheConsolidationsStorage->size() > 0) {
				if (invalidateCacheCounter > cacheClearBarrier) {
					cacheConsolidationsStorage->clear();    
					invalidateCacheCounter = 0;

					cachedAreas.clear();
					cachedCellValues.clear();

					return false;        
				}      

				return true;
			}    
			return false;
		}



		void Cube::removeCachedCells (CellPath* cellPath) {
			Statistics::Timer timer("Cube::removeCachedCells");

			database->clearRuleCaches();

			if (isInvalid()) {      
				vector< set<IdentifierType> > deleteArea(dimensions.size());
				const vector<Element*>* elements = cellPath->getPathElements();

				vector< set<IdentifierType> >::iterator ai = deleteArea.begin();
				vector<Element*>::const_iterator ei = elements->begin();
				vector<Dimension*>::iterator di = dimensions.begin();

				for (;ai != deleteArea.end(); ai++, ei++, di++) {
					ai->insert((*ei)->getIdentifier());
					getParentElements((*di), (*ei), &(*ai));
				}

				invalidateCache(cellPath, &deleteArea);
			}
		}



		void Cube::removeCachedCells (CellPath* cellPath, vector< set<IdentifierType> >* baseElements) {
			database->clearRuleCaches();

			if (isInvalid()) {      
				vector< set<IdentifierType> > deleteArea(dimensions.size());

				vector< set<IdentifierType> >::iterator ai = deleteArea.begin();
				vector< set<IdentifierType> >::iterator ei = baseElements->begin();
				vector<Dimension*>::iterator di = dimensions.begin();

				for (;ai != deleteArea.end(); ai++, ei++, di++) {

					for (set<IdentifierType>::iterator i = ei->begin(); i != ei->end(); i++) {
						IdentifierType id = *i;
						Element* e = (*di)->lookupElement(id);
						if (e) {
							ai->insert(id);
							getParentElements((*di), e, &(*ai));
						}
					}
				}

				invalidateCache(cellPath, &deleteArea);
			}
		}


		void Cube::removeCachedCells (CellPath* cellPath, vector<IdentifiersWeightType>* baseElements) {
			database->clearRuleCaches();

			if (isInvalid()) {      
				vector< set<IdentifierType> > deleteArea(dimensions.size());

				vector< set<IdentifierType> >::iterator ai = deleteArea.begin();
				vector<IdentifiersWeightType>::iterator ei = baseElements->begin();
				vector<Dimension*>::iterator di = dimensions.begin();

				for (;ai != deleteArea.end(); ai++, ei++, di++) {

					for (IdentifiersWeightType::iterator i = ei->begin(); i != ei->end(); i++) {
						pair<IdentifierType, double>* p = &(*i);
						Element* e = (*di)->lookupElement(p->first);
						if (e) {
							ai->insert(p->first);
							getParentElements((*di), e, &(*ai));
						}
					}
				}

				invalidateCache(cellPath, &deleteArea);
			}
		}


		void Cube::removeCachedCells (CellPath* cellPath, vector<ElementsWeightType>* baseElements) {
			database->clearRuleCaches();

			if (isInvalid()) {      
				vector< set<IdentifierType> > deleteArea(dimensions.size());

				vector< set<IdentifierType> >::iterator ai = deleteArea.begin();
				vector<ElementsWeightType>::iterator ei = baseElements->begin();
				vector<Dimension*>::iterator di = dimensions.begin();

				for (;ai != deleteArea.end(); ai++, ei++, di++) {

					for (ElementsWeightType::iterator i = ei->begin(); i != ei->end(); i++) {
						Element* e = i->first;
						ai->insert(e->getIdentifier());
						getParentElements((*di), e, &(*ai));
					}
				}

				invalidateCache(cellPath, &deleteArea);
			}
		}


		void Cube::removeCachedCells (Dimension* dimension, IdentifierType element) {
			database->clearRuleCaches();

			if (isInvalid()) {      
				Element* e = dimension->lookupElement(element);
				if (e) {
					// put element and parent elements into set
					set<IdentifierType> elementIds;
					elementIds.insert(element);                  
					getParentElements(dimension, e, &elementIds);

					vector<Dimension*>::iterator di = dimensions.begin();
					size_t numDimension = 0;         
					for (;di != dimensions.end(); di++, numDimension++) {
						if (*di == dimension) {
							// remove all cells containing this elements from cache
							cacheConsolidationsStorage->deleteCells(numDimension, &elementIds);
						}
					}
				}
			}
		}

		void Cube::invalidateCache (CellPath* cellPath, vector< set<IdentifierType> >* deleteArea) {
			double count = 1.0;

			// count number of changed cells
			for (vector< set<IdentifierType> >::iterator i = deleteArea->begin(); i != deleteArea->end(); i++) {
				count *= i->size();
			}

			// delete single element
			if (count == 1.0) {
				cacheConsolidationsStorage->deleteCell(cellPath);
				invalidateCacheCounter++;
			}

			// check number of cells
			else if (count < cacheClearBarrierCells) {
				cacheConsolidationsStorage->deleteCells(deleteArea);
				invalidateCacheCounter++;
			}

			// clear cache
			else {
				cacheConsolidationsStorage->clear();
				invalidateCacheCounter = 0;

				cachedAreas.clear();
				cachedCellValues.clear();      
			}
		}

		void Cube::clearRulesCache () {
			if (cacheRulesStorage && cacheRulesStorage->size() > 0) {
				Logger::debug << "clearing rules cache of cube '" << name << "'" << endl;

				cacheRulesStorage->clear();    
			}          
		}


		void Cube::clearAllCaches () {
			clearRulesCache();

			if (cacheConsolidationsStorage) {
				cacheConsolidationsStorage->clear();    
				invalidateCacheCounter = 0;

				cachedAreas.clear();
				cachedCellValues.clear();
			}    
		}


		double Cube::getNumAreaBaseCells (vector<IdentifiersType>* area) {
			// get numeric base elements
			vector< map<IdentifierType, map<IdentifierType, double> > > baseElements;
			return computeAreaBaseElements(area, &baseElements, false);    
		}



		void Cube::checkExportPathAccessRight (ExportStorage* storage, User * user) {
			IdentifiersType path(dimensions.size());

			size_t last =  storage->getLastCheckedElement();   
			for (size_t i = storage->getNumberElements(); i > last; i--) {
				Cube::CellValueType* value = storage->getCell(i-1, &path);        
				try {
					// check cell path (throws exception)
					CellPath cellPath(this, &path);

					// checkPathAccessRight (throws exception)
					checkPathAccessRight(user, &cellPath, RIGHT_READ);
				}
				catch (ParameterException e) {
					storage->removeCell(i-1);
				}
			}

		}



		double Cube::computeAreaBaseElements (vector<IdentifiersType>* paths, 
			vector< map<IdentifierType, map<IdentifierType, double> > > *baseElements,
			bool ignoreWrongIdentifiers) {
				double result = 1.0;

				// setup the size of result vector
				baseElements->resize(dimensions.size());

				vector<Dimension*>::const_iterator dimensionIter = dimensions.begin();
				vector<IdentifiersType>::iterator  elementIter   = paths->begin();
				vector< map<IdentifierType, map<IdentifierType, double> > >::iterator baseIter = baseElements->begin();

				// loop over all dimensions
				for (; dimensionIter != dimensions.end(); elementIter++, baseIter++, dimensionIter++) {
					map<IdentifierType, map<IdentifierType, double> >* mapping = &(*baseIter);

					// loop over all given dimension elements
					for (IdentifiersType::iterator id = elementIter->begin(); id != elementIter->end(); id++) { 
						Element * e;

						if (ignoreWrongIdentifiers) {
							e = (*dimensionIter)->lookupElement(*id);
							if (!e) {
								continue;
							}
						}
						else {
							e = (*dimensionIter)->findElement(*id, 0);
						}

						Element::ElementType type = e->getElementType();

						if (type == Element::CONSOLIDATED) {
							if ((*dimensionIter)->isStringConsolidation(e)) {
								// ignore strings
							}
							else {
								computeAreaBaseElementsRecursive(*dimensionIter, *id, e, 1.0, mapping);
							}
						}
						else if (type == Element::NUMERIC) {
							map<IdentifierType, map<IdentifierType, double> >:: iterator i = mapping->find(*id);

							// mapping found
							if (i != mapping->end()) {
								map<IdentifierType, double>::iterator i2 = i->second.find(*id);

								// mapping id -> id not found
								if (i2 == i->second.end()) {
									i->second.insert( make_pair(*id, 1.0) );
								}
							}

							// add mapping
							else {
								map<IdentifierType, double> m;
								m.insert( make_pair(*id, 1.0) );
								mapping->insert( make_pair(*id, m) );
							}        
						}

						// ignore strings
						else if (type == Element::STRING) {
						}
					}

					result *= mapping->size();
				}

				return result;
		}



		void Cube::computeAreaBaseElementsRecursive (
			Dimension * dimension,
			IdentifierType elementId,
			Element * element,
			double weight,
			map<IdentifierType, map<IdentifierType, double> > * mapping) {
				if (weight == 0.0) {
					// ignore element
					return;
				}

				// get children and weights
				const ElementsWeightType * children = dimension->getChildren(element);

				if (children->empty()) {
					// ignore element      
					return;
				}

				ElementsWeightType::const_iterator ce = children->begin();

				// loop over edges
				for (;  ce != children->end();  ce++) {
					const pair<Element*, double>& child = *ce;
					Element::ElementType type = child.first->getElementType();

					if (type == Element::CONSOLIDATED) {
						computeAreaBaseElementsRecursive(dimension, elementId, child.first, weight * child.second, mapping);
					}
					else if (type == Element::NUMERIC) {

						if (child.second == 0.0) {
							// ignore element
							break;
						}

						IdentifierType id = child.first->getIdentifier();
						map<IdentifierType, map<IdentifierType, double> >:: iterator i = mapping->find(id);
						if (i != mapping->end()) {
							// mapping found

							map<IdentifierType, double>::iterator i2 = i->second.find(elementId);
							if (i2 == i->second.end()) {
								// mapping id -> id not found              
								//Logger::debug << "Cube::computeAreaBaseElementsRecursive adding mapping: " << id << " -> <" << elementId << " (" << weight * child.second << ")> "<< endl;
								i->second.insert( make_pair(elementId, weight * child.second) );
							}
							else {
								// Logger::debug << "Cube::computeAreaBaseElementsRecursive found mapping: " << id << " -> " << elementId << " adding weight " << weight * child.second << endl;
								i2->second += weight * child.second;            
							}

						}
						else {
							// add mapping         
							map<IdentifierType, double> m;
							m.insert( make_pair(elementId, weight * child.second) );
							mapping->insert( make_pair(id, m) );
							//Logger::debug << "Cube::computeAreaBaseElementsRecursive added mapping: " << id << " -> <" << elementId << " (" << weight * child.second << ")> " << endl;  
						}        
					}
				}     
		}




		bool Cube::computeBaseElementsOnly (vector<IdentifiersType>* area, 
			vector<IdentifiersType>* resultArea,
			vector< map<IdentifierType, map<IdentifierType, double> > > *baseElements,
			bool ignoreWrongIdentifiers) {
				bool result = true;

				// setup the size of result vector
				baseElements->resize(dimensions.size());
				resultArea->resize(dimensions.size());

				vector<Dimension*>::const_iterator dimensionIter   = dimensions.begin();
				vector<IdentifiersType>::iterator  elementIter     = area->begin();
				vector<IdentifiersType>::iterator  baseElementIter = resultArea->begin();
				vector< map<IdentifierType, map<IdentifierType, double> > >::iterator baseIter = baseElements->begin();

				// loop over all dimensions
				for (; dimensionIter != dimensions.end(); elementIter++, baseElementIter++, baseIter++, dimensionIter++) {
					map<IdentifierType, map<IdentifierType, double> >* mapping = &(*baseIter);

					// loop over all given dimension elements
					for (IdentifiersType::iterator id = elementIter->begin(); id != elementIter->end(); id++) { 
						Element * e;

						if (ignoreWrongIdentifiers) {
							e = (*dimensionIter)->lookupElement(*id);

							if (!e) {
								continue;
							}
						}
						else {
							e = (*dimensionIter)->findElement(*id, 0);
						}

						Element::ElementType type = e->getElementType();

						// add mapping
						if (type == Element::NUMERIC) {
							map<IdentifierType, double> m;
							m.insert( make_pair(*id, 1.0) );
							mapping->insert( make_pair(*id, m) );
							baseElementIter->push_back(*id);
						}

						// ignore strings
						else if (type == Element::STRING) {
							baseElementIter->push_back(*id);
						}

						// ignore consolidated elements
						else if (type == Element::CONSOLIDATED) {
						}
					}

					// empty dimension (Area is empty)
					if (baseElementIter->size() == 0) {
						result = false;
					}
				}

				return result;
		}



		void Cube::addSiblingsAndParents (set<IdentifierType>* identifiers, Dimension* dimension, IdentifierType id) {
			set<IdentifierType>::iterator i = identifiers->find(id);
			if (i == identifiers->end()) {
				// identifier not found in set

				Element* e = dimension->lookupElement(id);
				if (!e) {
					return;
				}

				IndentType indent = e->getIndent(dimension);

				vector<Element*> elements = dimension->getElements(0);
				for (vector<Element*>::iterator element = elements.begin(); element != elements.end(); element++) {
					if ( (*element)->getIndent(dimension) == indent ) {
						// found element with same indent
						addElementAndParents(identifiers, dimension, *element);
					}
				} 
			}
		}



		void Cube::addElementAndParents (set<IdentifierType>* identifiers, Dimension* dimension, Element* child) {
			set<IdentifierType>::iterator i = identifiers->find(child->getIdentifier());
			if (i == identifiers->end()) {
				identifiers->insert(child->getIdentifier());

				// identifier not found      
				const vector<Element*>* parents = dimension->getParents(child);
				for (vector<Element*>::const_iterator parentIter = parents->begin(); parentIter != parents->end(); parentIter++) {
					addElementAndParents(identifiers, dimension, *parentIter);
				}      
			}
		}



		Lock* Cube::lockCube (vector<IdentifiersType>* area, const string& areaString, User* user) {

			// TODO check user right for locking

			IdentifierType idUser = 0;    
			if (user) {
				idUser = user->getIdentifier();
			}

			FileName lockFileName = FileName(fileName->path, fileName->name + "_lock_" + StringUtils::convertToString(maxLockId), fileName->extension);

			Lock* lock = new Lock(maxLockId++, this, area, areaString, idUser, fileName);

			for (vector<Lock*>::iterator l = locks.begin(); l != locks.end(); l++) {
				if ( (*l)->overlaps(lock->getContainsArea())) {
					delete lock;
					throw ErrorException(ErrorException::ERROR_CUBE_WRONG_LOCK, "overlapping lock area");
				}
			}    

			locks.push_back(lock);

			hasLock = true;

			Scheduler::getScheduler()->addLock(lock);

			updateClientCacheToken();

			return lock;
		}

		void Cube::commitCube (long int id, User* user) {
			IdentifierType idUser = 0;    

			if (user) {
				idUser = user->getIdentifier();
			}

			for (vector<Lock*>::iterator l = locks.begin(); l != locks.end(); l++) {
				if ((*l)->getIdentifier() == id) {        
					Lock* lock = *l;

					if (user != 0 && lock->getUserIdentifier() != idUser) {
						if (user->getRoleSysOpRight() < RIGHT_DELETE) {
							throw ParameterException(ErrorException::ERROR_CUBE_WRONG_USER, "wrong user to unlock cube", "user", user->getName());
						}
					}

					locks.erase(l);
					Scheduler::getScheduler()->removeLock(lock);

					delete lock;

					if (locks.empty()) {
						hasLock = false;
					}

					updateClientCacheToken();

					return;
				}
			}

			throw ParameterException(ErrorException::ERROR_CUBE_LOCK_NOT_FOUND, "lock not found", "id", (int) id);          
		}


		void Cube::rollbackCube (long int id, User* user, size_t numSteps) {
			IdentifierType idUser = 0;    

			if (user) {
				idUser = user->getIdentifier();
			}

			for (vector<Lock*>::iterator l = locks.begin(); l != locks.end(); l++) {
				if ((*l)->getIdentifier() == id) {        
					Lock* lock = *l;

					if (user != 0 && lock->getUserIdentifier() != idUser) {
						if (user->getRoleSysOpRight() < RIGHT_DELETE) {
							throw ParameterException(ErrorException::ERROR_CUBE_WRONG_USER, "wrong user to unlock cube", "user", user->getName());
						}
					}

					RollbackStorage* rstorage = lock->getStorage();

					// rollback
					if (numSteps == 0) {
						rstorage->rollback(this, rstorage->getNumberSteps(), user);

						locks.erase(l);
						Scheduler::getScheduler()->removeLock(lock);

						delete lock;

						if (locks.empty()) {
							hasLock = false;
						}
					}
					else {
						rstorage->rollback(this, numSteps, user);
					}

					updateClientCacheToken();

					return;
				}
			}

			throw ParameterException(ErrorException::ERROR_CUBE_LOCK_NOT_FOUND, "lock not found", "id", (int) id);          
		}


		const vector<Lock*>& Cube::getCubeLocks (User* user) {

			// TODO check user right for locking

			return locks;
		}


		Lock* Cube::lookupLockedArea (CellPath* cellPath, User* user) {
			if (!hasLock) {
				return 0;
			}

			IdentifierType idUser = 0;    
			if (user) {
				idUser = user->getIdentifier();
			}

			for (vector<Lock*>::iterator l = locks.begin(); l != locks.end(); l++) {
				if ((*l)->contains(cellPath)) {        
					Lock* lock = *l;

					if (lock->getUserIdentifier() != idUser) {
						// wrong user for lock
						if (user) {
							throw ParameterException(ErrorException::ERROR_CUBE_WRONG_USER, "wrong user to write to locked area", "user", user->getName());          
						}
						else {
							throw ParameterException(ErrorException::ERROR_CUBE_WRONG_USER, "wrong user to write to locked area", "user", "");          
						}
					}

					return lock;
				}
				else if ((*l)->blocks(cellPath)) {
					Lock* lock = *l;
					throw ErrorException(ErrorException::ERROR_CUBE_BLOCKED_BY_LOCK, "cannot splash because of a locked area");          
				}
			}

			return 0;
		}

		Lock* Cube::findCellLock(CellPath* cellPath) {
			if (!hasLock) {
				return 0;
			}
			for (vector<Lock*>::iterator l = locks.begin(); l != locks.end(); l++) 
				if ((*l)->contains(cellPath))
					return (*l);
			return 0;
		}

		Cube::CellLockInfo Cube::getCellLockInfo(CellPath* cellPath, IdentifierType userId) {
			Lock* l = findCellLock(cellPath);
			if (l!=0)
				if (l->getUserIdentifier()==userId)
					return 1;
				else 
					return 2;
			return 0;
		}

		////////////////////////////////////////////////////////////////////////////////
		// markers
		////////////////////////////////////////////////////////////////////////////////

		void Cube::checkNewMarkerRules () {
			if (Server::triggerMarkerCalculation(this)) {
				newMarkerRules.clear();
			}

			if (newMarkerRules.empty()) {
				return;
			}

			for (set<Rule*>::iterator i = newMarkerRules.begin();  i != newMarkerRules.end();  i++) {
				Rule* rule = *i;

				try {
					rule->computeMarkers();
				}
				catch (const ErrorException& ex) {
					Logger::error << "cannot compute markers for rule, got " << ex.getMessage()
						<< "(" << ex.getDetails() << ")" << endl;
				}
			}

			newMarkerRules.clear();
		}

		void Cube::removeFromMarker (RuleMarker* marker) {
			Logger::trace << "removing from " << *marker << " from cube '" << name << "'" << endl;

			fromMarkers.erase(marker);
		}



		void Cube::removeToMarker (RuleMarker* marker) {
			Logger::trace << "removing to " << *marker << " from cube '" << name << "'" << endl;

			toMarkers.erase(marker);

			// we have to clear the markers and rebuild them
			Server::addChangedMarkerCube(this);
		}



		void Cube::addFromMarker (RuleMarker* marker) {
			Logger::trace << "adding from " << *marker << " to cube '" << name << "'" << endl;

			// construct a new marker storage
			size_t nd = marker->getToCube()->getDimensions()->size();
			MarkerStorage markers(nd,
				marker->getFixed(),
				marker->getPermutations(),
				marker->getMapping());

			// loop through all elements of occuring in the from area
			storageDouble->setMarkers(marker->getFromBase(), &markers);
			Logger::debug << "found " << markers.size() << " marked cells" << endl;

			// and set these values as new cells
			const uint8_t* b = markers.begin();
			const uint8_t* e = markers.end();
			size_t r = markers.getRowSize();
			Cube* toCube = marker->getToCube();

			for (;  b < e;  b += r) {
				const uint32_t* buffer = (const uint32_t*) b;

				toCube->setCellMarker(buffer);
			}

			// keep a list of active "from" markers
			fromMarkers.insert(marker);
		}



		void Cube::addToMarker (RuleMarker* marker) {
			Logger::trace << "adding to " << *marker << " to cube '" << name << "'" << endl;

			// keep a list of active "to" markers
			toMarkers.insert(marker);
		}



		void Cube::clearAllMarkers () {
			storageDouble->clearAllMarkers();
		}



		void Cube::rebuildAllMarkers () {
			Logger::trace << "rebuild all markers for cube '" << name << "'" << endl;

			set<RuleMarker*> markers;
			markers.swap(fromMarkers);

			for (set<RuleMarker*>::iterator i = markers.begin();  i != markers.end();  i++) {
				RuleMarker* marker = *i;

				addFromMarker(marker);
			}

			storageDouble->sort();    
		}



		void Cube::setCellMarker (const uint32_t* path) {
			static double empty = 0;

			// setCellValue will trigger checkFromMarkers if the cell is new
			storageDouble->setCellValue((CubePage::key_t) path, (CubePage::value_t) &empty, true);
		}



		void Cube::checkFromMarkers (CubePage::key_t key) {
			if (fromMarkers.empty()) {
				return;
			}

			for (set<RuleMarker*>::iterator i = fromMarkers.begin();  i != fromMarkers.end();  i++) {
				RuleMarker* marker = *i;

				if (isInArea((IdentifierType*) key, marker->getFromBase())) {
					size_t nd = marker->getToCube()->getDimensions()->size();

					uint32_t * b = new uint32_t[nd];
					const uint32_t* perm = marker->getPermutations();
					const uint32_t* fix = marker->getFixed();
					const vector< vector<uint32_t> > * maps = marker->getMapping();
					uint32_t* ptr = b;
					uint32_t* end = ptr + nd;
					uint32_t* path = (uint32_t*) key;

					if (maps == 0) {
						for (; ptr < end;  ptr++, perm++, fix++) {
							if (*perm != MarkerStorage::NO_PERMUTATION) {
								*ptr = path[*perm];
							}
							else {
								*ptr = *fix;
							}
						}
					}
					else {
						vector< vector<uint32_t> >::const_iterator m = maps->begin();

						for (; ptr < end;  ptr++, perm++, fix++, m++) {
							uint32_t id;
							const vector<uint32_t>& mapping = *m;

							if (*perm != MarkerStorage::NO_PERMUTATION) {
								id = path[*perm];

								if (id >= mapping.size() || mapping[id] == MarkerStorage::NO_MAPPING) {
									continue;
								}

								id = mapping[id];
							}
							else {
								id = *fix;
							}

							*ptr = id;
						}
					}

					marker->getToCube()->setCellMarker(b);
					delete[] b;
				}
			}
		}

		void Cube::archiveJournalFiles (const FileName& fileName) {

			// find last journal file 
			bool lastJournalFound = false;
			int num = 0;

			FileName af(fileName.path, fileName.name, "archived");
			ofstream outputFile(af.fullPath().c_str(), ios::app);

			while (! lastJournalFound) {
				stringstream se;
				se << fileName.name << "_" << num;      

				FileName jf(fileName.path, se.str(), fileName.extension);

				if (! FileUtils::isReadable(jf)) {
					lastJournalFound = true;
				}
				else {
					{
						ifstream inputFile(jf.fullPath().c_str());

						size_t count = 1;

						while (inputFile) {
							string line;
							getline(inputFile, line);

							if (inputFile) {
								outputFile << line << "\n";
								if ((++count % TIME_BASED_PROGRESS_COUNT) == 0) {
									database->getServer()->timeBasedProgress();
								}

							}
						}
					}

					FileWriter::deleteFile(jf);
					num++;
				}
			}
		}
	}
