////////////////////////////////////////////////////////////////////////////////
/// @brief palo base dimension
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

#include "Olap/BasicDimension.h"

#include <iostream>

#include "Exceptions/FileFormatException.h"
#include "Exceptions/ParameterException.h"

#include "Collections/DeleteObject.h"
#include "Collections/StringBuffer.h"

#include "InputOutput/FileReader.h"
#include "InputOutput/FileWriter.h"
#include "InputOutput/Logger.h"
#include "InputOutput/Statistics.h"

#include "Olap/Cube.h"
#include "Olap/Database.h"
#include "Olap/Server.h"

namespace palo {
	ElementsWeightType BasicDimension::emptyChildren;
	BasicDimension::ParentsType BasicDimension::emptyParents;

	////////////////////////////////////////////////////////////////////////////////
	// constructors and destructors
	////////////////////////////////////////////////////////////////////////////////

	BasicDimension::BasicDimension (IdentifierType identifier, const string& name, Database * database)
		: Dimension(identifier, name, database),
		numElements(0),
		parentToChildren(1000, Parent2ChildrenDesc()),
		childToParents(1000, Child2ParentsDesc()),
		nameToElement(1000, Name2ElementDesc()),
		positionToElement(1000, Position2ElementDesc()),
		stringConsolidations(1000, ElementSetDesc()),
		isValidLevel(false),
		maxLevel(0),
		maxIndent(0),
		maxDepth(0),
		isValidBaseElements(false),
		isValidSortedElements(false) {
	}



	BasicDimension::~BasicDimension () {
		for_each(elements.begin(), elements.end(), DeleteObject());
		parentToChildren.clearAndDelete();
		childToParents.clearAndDelete();
	}



	////////////////////////////////////////////////////////////////////////////////
	// functions to load and save a dimension
	////////////////////////////////////////////////////////////////////////////////

	uint32_t BasicDimension::loadDimensionOverview (FileReader* file) {
		const string section = "DIMENSION " + StringUtils::convertToString(identifier);
		uint32_t sizeElements = 0;

		if (file->isSectionLine() && file->getSection() == section) {
			file->nextLine();

			if (file->isDataLine()) {
				uint32_t level  = file->getDataInteger(2);
				uint32_t indent = file->getDataInteger(3);
				uint32_t depth  = file->getDataInteger(4);

				sizeElements = file->getDataInteger(5);
				elements.resize(sizeElements, 0);

				maxLevel  = level;
				maxIndent = indent;
				maxDepth  = depth;

				file->nextLine();
			}
		}
		else {
			throw FileFormatException("Section '" + section + "' not found.", file);
		}

		return sizeElements;
	}



	void BasicDimension::loadDimensionElementParents (FileReader* file,
		Element* element,
		IdentifiersType* parents,
		uint32_t sizeElements) {
			ChildParentsPair * cpp = childToParents.findKey(element);

			if (cpp == 0) {
				cpp = new ChildParentsPair(element);
				childToParents.addElement(cpp);
			}

			ParentsType& pt = cpp->parents;
			pt.resize(parents->size());

			IdentifiersType::const_iterator parentsIter = parents->begin();

			ParentsType::iterator j = pt.begin();

			for (;  parentsIter != parents->end();  parentsIter++, j++) {
				IdentifierType id = *parentsIter;

				if (0 <= id && id < sizeElements) {
					*j = elements[id];
				}
				else {
					Logger::error << "parent element identifier '" << identifier << "' of element '"
						<< element->getName() << "' is greater or equal than maximum ("
						<<  sizeElements << ")" << endl;
					throw FileFormatException("illegal identifier in parents list", file);
				}
			}
	}



	void BasicDimension::loadDimensionElementChildren (FileReader* file,
		Element* element,
		IdentifiersType* children,
		vector<double>* weights,
		uint32_t sizeElements) {
			ParentChildrenPair * pcp = parentToChildren.findKey(element);

			if (pcp == 0) {
				pcp = new ParentChildrenPair(element);
				parentToChildren.addElement(pcp);
			}

			ElementsWeightType& ew = pcp->children;
			ew.resize(children->size());

			IdentifiersType::const_iterator childrenIter = children->begin();
			vector<double>::const_iterator  weightsIter  = weights->begin();

			ElementsWeightType::iterator j = ew.begin();

			for (;  childrenIter != children->end();  childrenIter++, weightsIter++, j++) {
				IdentifierType id = *childrenIter;

				if (0 <= id && id < sizeElements) {
					(*j).first  = elements[id];
					(*j).second = *weightsIter;
				}
				else {
					Logger::error << "child element identifier '" << identifier << "' of element '" 
						<< element->getName() << "' is greater or equal than maximum ("
						<<  sizeElements << ")" << endl;
					throw FileFormatException("illegal identifier in children list", file);
				}
			}
	}



	void BasicDimension::loadDimensionElement (FileReader* file, uint32_t sizeElements) {
		IdentifierType id = file->getDataInteger(0);
		string name = file->getDataString(1);
		PositionType pos = file->getDataInteger(2);
		long i = file->getDataInteger(3);
		Element::ElementType type = Element::UNDEFINED;

		switch (i) {
	  case 1: type = Element::NUMERIC;      break;
	  case 2: type = Element::STRING;       break;
	  case 4: type = Element::CONSOLIDATED; break;

	  default:
		  Logger::error << "element '" << name << "' has unknown type '" <<  i << "'" << endl;
		  throw FileFormatException("element has wrong type", file);
		}

		string isStrCons         = file->getDataString(4);
		long level               = file->getDataInteger(5);
		long indent              = file->getDataInteger(6);
		long depth               = file->getDataInteger(7);          
		IdentifiersType parents  = file->getDataIdentifiers(8);
		IdentifiersType children = file->getDataIdentifiers(9);
		vector<double> weights   = file->getDataDoubles(10);

		if (id < 0 || id >= sizeElements) {
			Logger::error << "element identifier '" << identifier << "' of element '" 
				<< name << "' is greater or equal than maximum (" 
				<<  sizeElements << ")" << endl;
			throw FileFormatException("wrong element identifier found", file);
		}

		Element * element = elements[id];

		element->setName(name);
		element->setPosition(pos);
		element->setLevel(level);
		element->setIndent(indent);
		element->setDepth(depth);

		// update name and position
		nameToElement.addElement(name, element);
		positionToElement.addElement(pos, element);

		// children to parent
		if (! parents.empty()) {
			loadDimensionElementParents(file, element, &parents, sizeElements);
		}

		// parent to children
		if (children.size() != weights.size()) {
			Logger::error << "size of children list and size of children weight list is not equal" << endl;
			throw FileFormatException("children size != children weight size", file);
		}

		if (! children.empty()) {
			loadDimensionElementChildren(file, element, &children, &weights, sizeElements);
		}

		// check element type for consolidated elements 
		if (!children.empty() && type != Element::CONSOLIDATED) {
			type = Element::CONSOLIDATED;
		}
		else if (children.empty() && type == Element::CONSOLIDATED) {
			type = Element::NUMERIC;
		}  

		element->setElementType(type);

		// string consolidations
		if (isStrCons == "1" && type == Element::CONSOLIDATED) {
			stringConsolidations.addElement(element);
		}
	}



	void BasicDimension::loadDimensionElements (FileReader* file, uint32_t sizeElements) {
		const string section = "ELEMENTS DIMENSION " + StringUtils::convertToString(identifier);

		// construct all elements, unsused elements are cleared later
		for (IdentifierType i = 0;  i < sizeElements;  i++) {
			elements[i] = new Element(i);
		}

		// load elements
		uint32_t count = 0;

		if (file->isSectionLine() && file->getSection() == section) {
			file->nextLine();

			while (file->isDataLine()) {
				loadDimensionElement(file, sizeElements);

				count++;
				file->nextLine();
			}
		}
		else {
			throw FileFormatException("Section '" + section + "' not found.", file);
		}

		for (IdentifierType i = 0;  i < sizeElements;  i++) {
			if (elements[i]->getElementType() == Element::UNDEFINED) {
				freeElements.insert(i);
				delete elements[i];
				elements[i] = 0;
			}
		}

		numElements = count;
	}



	void BasicDimension::loadDimension (FileReader* file) {
		Logger::trace << "loading dimension '" << name << "'. " << endl;

		// clear old dimenstion elements
		clearElements(0);

		// load dimension and elements from file
		uint32_t sizeElements = loadDimensionOverview(file);
		loadDimensionElements(file, sizeElements);

		// database is now loaded (memory and file image are in sync)
		setStatus(LOADED);

		isValidLevel = false;
		isValidBaseElements = false;
		isValidSortedElements = false;
	}



	void BasicDimension::saveDimensionOverview (FileWriter* file) {
		const string section = "DIMENSION " + StringUtils::convertToString(identifier);

		file->appendComment("Description of data: ");
		file->appendComment("ID;NAME,LEVEL,INDENT,DEPTH,SIZE_ELEMENTS; ");
		file->appendSection(section);
		file->appendIdentifier(identifier);
		file->appendEscapeString(name);
		file->appendInteger(getLevel());
		file->appendInteger(getIndent());
		file->appendInteger(getDepth());
		file->appendInteger((int32_t) elements.size());
	}



	void BasicDimension::saveDimensionElement (FileWriter* file, Element* element) {
		file->appendIdentifier(element->getIdentifier());
		file->appendEscapeString(element->getName());
		file->appendInteger(element->getPosition());

		switch (element->getElementType()) {
	  case Element::NUMERIC:
		  file->appendInteger(1);
		  file->appendInteger(0);
		  break;

	  case Element::STRING:
		  file->appendInteger(2);
		  file->appendInteger(0);
		  break;

	  case Element::CONSOLIDATED:
		  file->appendInteger(4);

		  if (isStringConsolidation(element)) {
			  file->appendInteger(1);
		  }
		  else {
			  file->appendInteger(0);
		  }

		  break;

	  default:
		  Logger::info << "saveDimension: Element '" << element->getIdentifier() 
			  << "' has wrong type." << endl;
		  file->appendInteger(0);
		  file->appendInteger(0);          
		  break;
		}

		file->appendInteger(element->getLevel(this));
		file->appendInteger(element->getIndent(this));
		file->appendInteger(element->getDepth(this));

		// parents
		const ParentsType * parents = getParents(element);

		IdentifiersType identifier(parents->size());

		ParentsType::const_iterator m = parents->begin();
		IdentifiersType::iterator   j = identifier.begin();

		for (; m != parents->end();  m++, j++) {
			*j = (*m)->getIdentifier();
		}

		file->appendIdentifiers(&identifier);

		// children
		const ElementsWeightType * children = getChildren(element);

		IdentifiersType identifiers(children->size());
		vector<double> weights(children->size());

		ElementsWeightType::const_iterator p = children->begin();
		IdentifiersType::iterator          q = identifiers.begin();
		vector<double>::iterator           k = weights.begin();

		for (;  p != children->end();  p++, q++, k++) {
			*q = (*p).first->getIdentifier();
			*k = (*p).second;
		}

		file->appendIdentifiers(&identifiers);
		file->appendDoubles(&weights);
	}



	void BasicDimension::saveDimensionElements (FileWriter* file) {
		const string section = "ELEMENTS DIMENSION " + StringUtils::convertToString(identifier);

		// write data for dimension
		file->appendComment("Description of data: ");
		file->appendComment("ID;NAME,POSITION,TYPE,STRING_CONSOLIDATION,LEVEL,INDENT,DEPTH,PARENTS,CHILDREN,CHILDREN_WEIGHT; ");
		file->appendSection(section);

		for (vector<Element*>::iterator i = elements.begin(); i != elements.end(); i++) {
			Element * element = *i;

			if (element == 0) {
				continue;
			}

			saveDimensionElement(file, element);
			file->nextLine();
		}
	}



	void BasicDimension::saveDimension (FileWriter* file) {

		// save dimension and elements to file
		saveDimensionOverview(file);
		saveDimensionElements(file);

		// database is now saved (memory and file image are in sync)
		setStatus(LOADED);
	}

	////////////////////////////////////////////////////////////////////////////////
	// getter and setter
	////////////////////////////////////////////////////////////////////////////////

	LevelType BasicDimension::getLevel () { 
		if (! isValidLevel) {
			updateLevelIndentDepth();
		}

		return maxLevel;
	}



	IndentType BasicDimension::getIndent () { 
		if (! isValidLevel) {
			updateLevelIndentDepth();
		}

		return maxIndent;
	}



	DepthType BasicDimension::getDepth () { 
		if (! isValidLevel) {
			updateLevelIndentDepth();
		}

		return maxDepth; 
	};

	////////////////////////////////////////////////////////////////////////////////
	// element functions
	////////////////////////////////////////////////////////////////////////////////

	set<Element*> BasicDimension::ancestors (Element* element) {
		set<Element*> ac;

		// add myself as trivial ancestor
		ac.insert(element);

		// find direct parents
		ChildParentsPair * cpp = childToParents.findKey(element);

		if (cpp == 0) {
			return ac;
		}

		// add direct parents and their ancestors
		ParentsType * parents = &cpp->parents;

		for (ParentsType::iterator iter = parents->begin();  iter != parents->end();  iter++) {
			Element * parent = *iter;

			if (ac.find(parent) == ac.end()) {
				set<Element*> aac = ancestors(parent);
				ac.insert(aac.begin(), aac.end());
			}
		}

		return ac;
	}

	void BasicDimension::clearElements (User* user) {
		checkElementAccessRight(user, RIGHT_WRITE);    

		if (user != 0 && ! changable) {
			throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
				"dimension cannot be changed",
				"dimension", name);
		}

		updateToken();

		for_each(elements.begin(), elements.end(), DeleteObject());

		elements.clear();
		sortedElements.clear();
		numElements = 0;
		freeElements.clear();

		parentToChildren.clearAndDelete();
		childToParents.clearAndDelete();

		nameToElement.clear();
		positionToElement.clear();

		maxLevel  = 0;
		maxIndent = 0;
		maxDepth  = 0;

		isValidLevel = true;
		isValidBaseElements = true;
		isValidSortedElements = true;

		vector<Cube*> cubes = getCubes(0);

		for (vector<Cube*>::iterator c = cubes.begin();  c != cubes.end();  c++) {
			(*c)->clearCells(0);
		}

		JournalFileWriter * journal = database->getJournal();

		if (journal != 0) {
			journal->appendCommand(Server::getUsername(user),
				Server::getEvent(),
				"CLEAR_ELEMENTS");
			journal->appendInteger(identifier);
			journal->nextLine();
		}

		database->clearRuleCaches();
	}



	vector<Element*> BasicDimension::getElements (User* user) {
		checkElementAccessRight(user, RIGHT_READ);

		if (! isValidLevel) {
			updateLevelIndentDepth();
		}

		vector<Element*> result(numElements, 0);

		for (vector<Element*>::iterator i = elements.begin();  i != elements.end();  i++) {
			Element * element = *i;

			if (element == 0) {
				continue;
			}

			result[element->getPosition()] = element;
		}

		return result;
	}      



	IdentifierType BasicDimension::getMaximalIdentifier () {
		IdentifierType max = 0;

		for (vector<Element*>::iterator i = elements.begin();  i != elements.end();  i++) {
			Element * element = *i;

			if (element == 0) {
				continue;
			}

			if (element->getIdentifier() > max) {
				max = element->getIdentifier();
			}
		}

		return max;
	}      



	void BasicDimension::moveElement (Element* element, PositionType newPosition, User* user) {
		checkElementAccessRight(user, RIGHT_WRITE);    

		if (0 <= newPosition && newPosition < numElements) {
			PositionType oldPosition = element->getPosition();
			int start = 0;
			int end   = 0;
			int add   = 0;

			// find direction
			if (oldPosition < newPosition) {
				start = oldPosition + 1;
				end   = newPosition;
				add   = -1;
			}
			else if (oldPosition > newPosition) {
				start = newPosition;
				end   = oldPosition - 1;
				add   = 1;
			}

			if (add != 0) {
				updateToken();

				for (int i = start;  i <= end;  i++) {
					positionToElement.removeKey(i);
				}

				positionToElement.removeKey(oldPosition);

				for (vector<Element*>::iterator i = elements.begin();  i != elements.end();  i++) {
					Element * current = *i;

					if (current == 0) {
						continue;
					}

					int pos = current->getPosition();

					if (start <= pos && pos <= end) {              
						current->setPosition(pos + add);
						positionToElement.addElement(current);
					}
				}

				element->setPosition(newPosition);
				positionToElement.addElement(element);

				setStatus(CHANGED);
			}
		}
		else {
			throw ParameterException(ErrorException::ERROR_INVALID_POSITION,
				"the position of an element has to be less than the number of elements",
				"position", (int) newPosition);
		}

		JournalFileWriter * journal = database->getJournal();

		if (journal != 0) {
			journal->appendCommand(Server::getUsername(user),
				Server::getEvent(),
				"MOVE_ELEMENT");
			journal->appendInteger(identifier);
			journal->appendInteger(element->getIdentifier());
			journal->appendInteger(element->getPosition());
			journal->nextLine();
		}
	}



	Element* BasicDimension::addElement (const string& name, Element::ElementType elementType, User* user, bool useJournal) {
		checkElementAccessRight(user, RIGHT_WRITE);    

		if (user != 0 && ! changable) {
			throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
				"dimension cannot be changed",
				"dimension", name);
		}

		checkElementName(name);

		Element* eByName = lookupElementByName(name);

		if (eByName != 0) {
			throw ParameterException(ErrorException::ERROR_ELEMENT_NAME_IN_USE,
				"element name is already in use",
				"name", name);
		}

		if (isLockedByCube()) {
			throw ErrorException(ErrorException::ERROR_DIMENSION_LOCKED, "dimension is used in a locked cube");
		}

		updateToken();

		IdentifierType elementId;
		Element * element = 0;

		// use new identifier
		// if (freeElements.empty()) {
		if (true) { // do not use freeElement list (version >= 2.1) 
			elementId = (IdentifierType) elements.size();
			elements.push_back(0);
		}

		// use unused element
		else {
			set<IdentifierType>::iterator i = freeElements.begin();
			elementId = (IdentifierType) (*i);
			freeElements.erase(i);
		}

		// create new element and add to the list of elements
		element = new Element(elementId);

		element->setName(name);

		if (elementType != Element::CONSOLIDATED) {
			element->setElementType(elementType);
		}
		else {
			// type Element::CONSOLIDATED is set in addChildren
			element->setElementType(Element::NUMERIC);
		}

		element->setPosition((PositionType) numElements);
		element->setLevel(0);
		element->setIndent(1);
		element->setDepth(0);

		// do not change: maxLevel, maxDepth
		if (maxIndent == 0) {
			maxIndent = 1;
		}
		// do not change isValidLevel, isValidBaseElements, isValidSortedElements
		sortedElements.push_front(element);

		map<IdentifierType, double> tmp;
		tmp.insert(make_pair(elementId, 1.0));
		element->setBaseElements(tmp);

		elements[elementId] = element;

		// update mapping
		positionToElement.addElement(numElements, element);
		nameToElement.addElement(name, element);

		// one more element
		numElements++;

		// dimension has been changed
		setStatus(CHANGED);

		JournalFileWriter * journal = database->getJournal();

		if (useJournal && journal != 0) {
			journal->appendCommand(Server::getUsername(user),
				Server::getEvent(),
				"ADD_ELEMENT");
			journal->appendInteger(identifier);
			journal->appendInteger(elementId);
			journal->appendEscapeString(name);
			journal->appendInteger(elementType);
			journal->nextLine();
		}

		database->clearRuleCaches();

		return element;
	}



	void BasicDimension::changeElementName (Element* element, const string& name, User* user) {
		checkElementAccessRight(user, RIGHT_WRITE);    

		checkElementName(name);

		if (user != 0 && ! changable) {
			throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
				"dimension cannot be changed",
				"dimension", name);
		}

		if (name.empty()) {
			throw ParameterException(ErrorException::ERROR_INVALID_ELEMENT_NAME,
				"element name is empty",
				"name", name);
		}

		if (isLockedByCube()) {
			throw ErrorException(ErrorException::ERROR_DIMENSION_LOCKED, "dimension is used in a locked cube");
		}

		// check for double used name
		Element * eByName = lookupElementByName(name);      

		if (eByName != 0) {
			if (eByName != element) {
				throw ParameterException(ErrorException::ERROR_ELEMENT_NAME_IN_USE,
					"element name is already in use",
					"name", name);
			}        

			if (element->getName() == name) {
				// new name = old name
				return;
			}
		}

		updateToken();

		// delete old name in nameToElement
		nameToElement.removeElement(element);

		// change name 
		element->setName(name);

		// add new name to nameToElement
		nameToElement.addElement(name, element);

		// dimension has been changed
		setStatus(CHANGED);

		JournalFileWriter * journal = database->getJournal();

		if (journal != 0) {
			journal->appendCommand(Server::getUsername(user),
				Server::getEvent(),
				"RENAME_ELEMENT");
			journal->appendInteger(identifier);
			journal->appendInteger(element->getIdentifier());
			journal->appendEscapeString(name);
			journal->nextLine();
		}

		database->clearRuleCaches();
	}



	bool BasicDimension::isCycle (const ParentsType* checkParents, const ElementsWeightType* children) {
		for (ElementsWeightType::const_iterator i = children->begin();  i != children->end();  i++) {
			Element * child = (*i).first;
			ParentsType::const_iterator found = find(checkParents->begin(), checkParents->end(), child);

			// child is parent -> cycle found
			if (found != checkParents->end()) {
				return true;
			}

			ParentChildrenPair * pcp = parentToChildren.findKey(child);

			if (pcp != 0) {
				ElementsWeightType * grandchildren = &pcp->children;

				if (isCycle(checkParents, grandchildren)) {
					return true;
				}
			}
		}

		return false;
	}



	void BasicDimension::addChildren (Element* parent, const ElementsWeightType* children, User* user) {
		if (children->empty()) {
			return;
		}

		checkElementAccessRight(user, RIGHT_WRITE);    

		if (user != 0 && ! changable) {
			throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
				"dimension cannot be changed",
				"dimension", name);
		}

		if (isLockedByCube()) {
			throw ErrorException(ErrorException::ERROR_DIMENSION_LOCKED, "dimension is used in a locked cube");
		}

		// check for cycle in graph
		const ParentsType * parents = getParents(parent);

		if (parents->size() > 0) {
			if (isCycle(parents, children)) {
				throw ParameterException(ErrorException::ERROR_ELEMENT_CIRCULAR_REFERENCE,
					"cycle detected in element hierarchy detected",
					"parent", parent->getName());
			}
		}

		// check for double elements
		set<Element*> eSet;

		for (ElementsWeightType::const_iterator i = children->begin(); i != children->end(); i++) {
			Element * element = (*i).first;

			eSet.insert(element);

			if (element == parent) {
				throw ParameterException(ErrorException::ERROR_ELEMENT_CIRCULAR_REFERENCE,
					"found parent in list of children ",
					"parent", parent->getName());
			}
		}

		if (children->size() != eSet.size()) {
			throw ParameterException(ErrorException::ERROR_ELEMENT_EXISTS,
				"element was found at least twice in list of children",
				"set of children", (int) eSet.size());
		}

		// set type of parent
		if (parent->getElementType() != Element::CONSOLIDATED) {
			changeElementType(parent, Element::CONSOLIDATED, 0,  true);
		}    

		// add children (no more checks)
		if (! children->empty()) {
			addChildrenNC(user, parent, children);
		}

		// delete cell path from cube cache
		removeElementFromCubes(user, parent, false, false, false);

		JournalFileWriter * journal = database->getJournal();

		if (journal != 0) {
			journal->appendCommand(Server::getUsername(user),
				Server::getEvent(),
				"ADD_CHILDREN");
			journal->appendInteger(identifier);
			journal->appendInteger(parent->getIdentifier());

			IdentifiersType identifiers(children->size());
			vector<double> weights(children->size());

			ElementsWeightType::const_iterator p = children->begin();
			IdentifiersType::iterator          q = identifiers.begin();
			vector<double>::iterator           k = weights.begin();

			for (;  p != children->end();  p++, q++, k++) {
				*q = (*p).first->getIdentifier();
				*k = (*p).second;
			}

			journal->appendIdentifiers(&identifiers);
			journal->appendDoubles(&weights);

			journal->nextLine();
		}

		updateToken();

		database->clearRuleCaches();
	}



	void BasicDimension::addChildrenNC (User* user,
		Element* parent,
		const ElementsWeightType* children) {

			// the level structure will change 
			if (isValidSortedElements && sortedElements[0] == parent) {
				isValidLevel = false;
				isValidBaseElements = false;
			}
			else {
				isValidLevel = false;
				isValidBaseElements = false;    
				isValidSortedElements = false;
			}

			// get our children
			ParentChildrenPair * pcp = parentToChildren.findKey(parent);

			if (pcp == 0) {
				pcp = new ParentChildrenPair(parent);
				parentToChildren.addElement(pcp);
			}

			// get list of old children, new children will be added
			ElementsWeightType& oew = pcp->children;
			size_t oldSize = oew.size();

			// check if we have a new string child
			bool hasStringChild = false;

			// add parent to child to parent mapping and lookup string children
			ElementsWeightType::const_iterator childrenIter = children->begin();

			for (;  childrenIter != children->end();  childrenIter++) {
				const pair<Element*,double>& ew = *childrenIter;
				Element* child = ew.first;

				// check if we already know this child
				ElementsWeightType::iterator f      = oew.begin();
				ElementsWeightType::iterator oewEnd = f + oldSize;

				for (;  f != oewEnd && ((*f).first != child);  f++) {
				}

				// if we know this child, just change the weights
				if (f != oewEnd) {
					oew.erase(f);
					oew.push_back(ew);
					continue;
				}

				// get type for string consolidations
				Element::ElementType type = child->getElementType();

				hasStringChild = hasStringChild || (type == Element::STRING) || isStringConsolidation(child);

				// add child and weight to parent to children mapping
				oew.push_back(ew);

				// add parent to children to parent mapping
				ChildParentsPair * cpp = childToParents.findKey(child);

				if (cpp == 0) {
					cpp = new ChildParentsPair(child);
					childToParents.addElement(child, cpp);
				}

				ParentsType& parents = cpp->parents;
				ParentsType::iterator found = find(parents.begin(), parents.end(), parent);

				if (found == parents.end()) {
					parents.push_back(parent);
				}
			}

			// check and update consolidation type
			bool isString = isStringConsolidation(parent);

			if (! isString && hasStringChild) {

				// add parent to string consolidations
				stringConsolidations.addElement(parent);

				// check and update parents
				const ParentsType * p = getParents(parent);

				for (ParentsType::const_iterator i = p->begin(); i != p->end(); i++) {
					checkUpdateConsolidationType(user, *i);
				}
			}

			// dimension has been changed
			setStatus(CHANGED);
	}



	void BasicDimension::removeParentInChildren (Element* parent, ElementsWeightType* ew) {
		for (ElementsWeightType::iterator childIter = ew->begin();  childIter != ew->end();  childIter++) {
			Element * child = (*childIter).first;

			// erase child to parent mapping
			ChildParentsPair * cpp = childToParents.findKey(child);

			if (cpp != 0) {
				ParentsType& parents = cpp->parents;

				// find parent in vector
				ParentsType::iterator pi = find(parents.begin(), parents.end(), parent);

				if (pi != parents.end()) {
					parents.erase(pi);

					// last parent deleted
					if (parents.empty()) {
						childToParents.removeElement(cpp);
						delete cpp;
					}
				}
			}
		}
	}



	void BasicDimension::removeParentInChildrenNotIn (Element* parent, ElementsWeightType* ew, set<Element*> * keep) {
		for (ElementsWeightType::iterator childIter = ew->begin();  childIter != ew->end();  childIter++) {
			Element * child = (*childIter).first;

			if (keep->find(child) != keep->end()) {
				continue;
			}

			// erase child to parent mapping
			ChildParentsPair * cpp = childToParents.findKey(child);

			if (cpp != 0) {
				ParentsType& parents = cpp->parents;

				// find parent in vector
				ParentsType::iterator pi = find(parents.begin(), parents.end(), parent);

				if (pi != parents.end()) {
					parents.erase(pi);

					// last parent deleted
					if (parents.empty()) {
						childToParents.removeElement(cpp);
						delete cpp;
					}
				}
			}
		}
	}



	void BasicDimension::removeChildInParents (User* user, Element* element, bool isString) {
		ChildParentsPair * cpp = childToParents.findKey(element);

		// no parents found, return
		if (cpp == 0) {
			return;
		}

		// loop over all parents
		ParentsType * parents = &cpp->parents;

		for (ParentsType::const_iterator parentsIter = parents->begin();
			parentsIter != parents->end();
			parentsIter++) {
				Element* parent = *parentsIter;

				// get parent to children map
				ParentChildrenPair * pcp = parentToChildren.findKey(parent);

				if (pcp != 0) {
					ElementsWeightType& ew = pcp->children;

					for (ElementsWeightType::iterator children = ew.begin();
						children != ew.end(); 
						children++) {
							if ((*children).first == element) {
								ew.erase(children);
								break;
							}
					}

					// last child of parent deleted
					if (ew.empty()) {
						parentToChildren.removeElement(pcp);

						// change element type to numeric
						changeElementType(parent, Element::NUMERIC, 0, false);

						delete pcp;
					}
				}

				// check and update StringConsolidation
				if (isString) {
					checkUpdateConsolidationType(user, parent);
				}
		}

		// delete element from childToParents
		childToParents.removeKey(element);

		// delete entry
		delete cpp;
	}


	void BasicDimension::removeChildren (User* user, Element* element) {

		if (isLockedByCube()) {
			throw ErrorException(ErrorException::ERROR_DIMENSION_LOCKED, "dimension is used in a locked cube");
		}

		// find childran
		ParentChildrenPair * pcp = parentToChildren.findKey(element);

		// if element has no children, return
		if (pcp == 0) {
			return;
		}

		// the level structure will change
		isValidLevel = false;
		isValidBaseElements = false;
		isValidSortedElements = false;

		updateToken();

		// get list of children
		ElementsWeightType& ew = pcp->children;

		// element found in mapping "parent to children", remove element in parents
		removeParentInChildren(element, &ew);

		// remove element from maps
		parentToChildren.removeKey(element);

		// delete mapping
		delete pcp;

		// check and update consolidation type
		bool isString = isStringConsolidation(element);

		// element was a "string" consolidation and is a normal consolidation now
		if (isString) {

			// remove element from string consolidations
			stringConsolidations.removeKey(element);

			// remove element from cubes (string storage)
			removeElementFromCubes(user, element, false, true, false, hasStringAttributes());

			// check and update parents
			const ParentsType * p = getParents(element);

			for (ParentsType::const_iterator i = p->begin();  i != p->end();  i++) {
				checkUpdateConsolidationType(user, *i);
			}
		}
		else {
			// remove element from cubes (cache storage)
			removeElementFromCubes(user, element, false, false, false);
		}

		JournalFileWriter * journal = database->getJournal();

		if (journal != 0) {
			journal->appendCommand(Server::getUsername(user),
				Server::getEvent(),
				"REMOVE_CHILDREN");
			journal->appendInteger(identifier);
			journal->appendInteger(element->getIdentifier());
			journal->nextLine();
		}

		database->clearRuleCaches();
	}



	template <class T> class notInSet: public unary_function<bool,T> {
	public:
		notInSet(set<T> * keep) : keep(keep) {
		}

		bool operator ()(pair<T,double> t) const {
			return keep->find(t.first) == keep->end();
		}

	private:
		set<T> * keep;
	};



	void BasicDimension::removeChildrenNotIn (User* user, Element* element, set<Element*> * keep) {

		if (isLockedByCube()) {
			throw ErrorException(ErrorException::ERROR_DIMENSION_LOCKED, "dimension is used in a locked cube");
		}

		if (keep->empty()) {
			removeChildren(user, element);
			return;
		}

		// find children
		ParentChildrenPair * pcp = parentToChildren.findKey(element);

		// if element has no children, return
		if (pcp == 0) {
			return;
		}

		// the level structure will change
		isValidLevel = false;
		isValidBaseElements = false;
		isValidSortedElements = false;

		updateToken();

		// get list of children
		ElementsWeightType& ew = pcp->children;

		// element found in mapping "parent to children", remove element in parents
		removeParentInChildrenNotIn(element, &ew, keep);

		// remove deleted children from pcp
		ElementsWeightType::iterator i = remove_if(ew.begin(), ew.end(), notInSet<Element*>(keep));
		ew.erase(i, ew.end());

		// remove element from maps if no children are left
		if (ew.empty()) {
			parentToChildren.removeKey(element);

			// delete mapping
			delete pcp;
		}

		// check and update consolidation type
		bool isString = isStringConsolidation(element);

		// element was a "string" consolidation and is a normal consolidation now
		if (isString) {

			// remove element from string consolidations
			stringConsolidations.removeKey(element);

			// remove element from cubes (string storage)
			removeElementFromCubes(user, element, false, true, false, hasStringAttributes());

			// check and update parents
			const ParentsType * p = getParents(element);

			for (ParentsType::const_iterator i = p->begin();  i != p->end();  i++) {
				checkUpdateConsolidationType(user, *i);
			}
		}
		else {
			// remove element from cubes (cache storage)
			removeElementFromCubes(user, element, false, false, false);
		}

		JournalFileWriter * journal = database->getJournal();

		if (journal != 0) {
			journal->appendCommand(Server::getUsername(user),
				Server::getEvent(),
				"REMOVE_CHILDREN_NOT_IN");
			journal->appendInteger(identifier);
			journal->appendInteger(element->getIdentifier());

			IdentifiersType identifiers(keep->size());

			set<Element*>::const_iterator p = keep->begin();
			IdentifiersType::iterator q = identifiers.begin();

			for (;  p != keep->end();  p++, q++) {
				*q = (*p)->getIdentifier();
			}

			journal->appendIdentifiers(&identifiers);

			journal->nextLine();
		}

		database->clearRuleCaches();
	}



	void BasicDimension::changeElementType (Element* element, 
		Element::ElementType elementType,
		User* user,
		bool setConsolidated) {
			checkElementAccessRight(user, RIGHT_WRITE);    

			if (user != 0 && ! changable) {
				throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
					"dimension cannot be changed",
					"dimension", name);
			}

			if (isLockedByCube()) {
				throw ErrorException(ErrorException::ERROR_DIMENSION_LOCKED, "dimension is used in a locked cube");
			}

			Element::ElementType oldType = element->getElementType();

			// ignore change to consolidation
			if (!setConsolidated && elementType == Element::CONSOLIDATED) {
				return;
			}

			// same type, nothing to do
			if (oldType == elementType) {
				return;
			}

			updateToken();

			// remove children
			if (oldType == Element::CONSOLIDATED) {
				bool isString = isStringConsolidation(element);

				if (isString) {
					stringConsolidations.removeKey(element);
				}      

				// delete cell path containing element (type: string consolidation) in removeChildren
				removeChildren(user, element);       
			}

			// delete cell path containing element 
			else {
				removeElementFromCubes(user, element, true, true, false, hasStringAttributes());
			}

			// change element type
			element->setElementType(elementType);

			// check and update parents
			const ParentsType* p = getParents(element);

			for (ParentsType::const_iterator i = p->begin(); i != p->end(); i++) {
				checkUpdateConsolidationType(user, *i);
			}

			// database has been changed
			setStatus(CHANGED);

			JournalFileWriter * journal = database->getJournal();

			if (journal != 0) {
				journal->appendCommand(Server::getUsername(user),
					Server::getEvent(),
					"CHANGE_ELEMENT");
				journal->appendInteger(identifier);
				journal->appendInteger(element->getIdentifier());
				journal->appendInteger(elementType);
				journal->nextLine();
			}

			database->clearRuleCaches();
	}



	void BasicDimension::deleteElement (Element* element, User* user, bool useJournal) {
		checkElementAccessRight(user, RIGHT_DELETE);    

		if (user != 0 && ! changable) {
			throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
				"dimension cannot be changed",
				"dimension", name);
		}

		if (isLockedByCube()) {
			throw ErrorException(ErrorException::ERROR_DIMENSION_LOCKED, "dimension is used in a locked cube");
		}

		updateToken();

		// the level structure might change
		isValidLevel = false;
		isValidBaseElements = false;
		isValidSortedElements = false;

		// delete cell path containing element 
		removeElementFromCubes(user, element, true, true, false); //deleteRules=false : see mantis (issue 2805)

		// we might have to update the string consolidation info
		bool isString = false;

		// delete all children
		if (element->getElementType() == Element::CONSOLIDATED) {
			ParentChildrenPair * pcp = parentToChildren.findKey(element);

			if (pcp != 0) {
				ElementsWeightType& ew = pcp->children;
				removeParentInChildren(element, &ew);

				// remove element from parent to child hashes 
				parentToChildren.removeKey(element);

				// delete mapping
				delete pcp;
			}

			isString = isStringConsolidation(element);

			if (isString) {
				stringConsolidations.removeKey(element);
			}
		}

		// simple string element
		else if (element->getElementType() == Element::STRING) {
			isString = true;
		}

		// remove element in parents
		removeChildInParents(user, element, isString);

		// now update positions of elements
		positionToElement.removeElement(element);

		// update elements with positions > position of deleted element
		PositionType positionDeletedElement = element->getPosition();

		for (PositionType i = positionDeletedElement;  i < numElements;  i++) {
			positionToElement.removeKey(i);
		}

		for (vector<Element*>::iterator i = elements.begin(); i != elements.end(); i++) {
			Element * current = *i;

			if (current == 0) {
				continue;
			}

			if (current == element) {
				*i = 0;
				continue;
			}

			PositionType pos = current->getPosition();

			if (pos > positionDeletedElement) {
				current->setPosition(pos - 1);
				positionToElement.addElement(current);
			}
		}

		// delete element from hash_map nameToElement
		nameToElement.removeElement(element);

		numElements--;

		// add identifier of the element to freeElements
		freeElements.insert(element->getIdentifier());

		// and finaly delete the element
		IdentifierType elementIdentifier = element->getIdentifier();
		delete element;
		element = 0;

		// the dimension has been changed
		setStatus(CHANGED);

		JournalFileWriter * journal = database->getJournal();

		if (journal != 0 && useJournal) {
			journal->appendCommand(Server::getUsername(user),
				Server::getEvent(),
				"DELETE_ELEMENT");
			journal->appendInteger(identifier);
			journal->appendInteger(elementIdentifier);
			journal->nextLine();
		}

		database->clearRuleCaches();
	}

	void BasicDimension::deleteElements (std::vector<Element *> elementList, User* user, bool useJournal)
	{
		//deleteElements2(elementList, user, useJournal);
		//return;

		Logger::trace<<"deleting "<<elementList.size()<<" elements"<<endl;
		checkElementAccessRight(user, RIGHT_DELETE);    

		if (user != 0 && ! changable) {
			throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
				"dimension cannot be changed",
				"dimension", name);
		}

		if (isLockedByCube()) {
			throw ErrorException(ErrorException::ERROR_DIMENSION_LOCKED, "dimension is used in a locked cube");
		}

		updateToken();

		// the level structure might change
		isValidLevel = false;
		isValidBaseElements = false;
		isValidSortedElements = false;

		std::vector<pair<PositionType, Element*> > deletedPositions; deletedPositions.reserve(elementList.size());
		IdentifiersType elementIdentifiers; elementIdentifiers.reserve(elementList.size());

		// delete cell path containing element 
		for (size_t i = 0; i<elementList.size(); i++) {
			Element* element = elementList[i];
			elementIdentifiers.push_back(element->getIdentifier());

			PositionType positionDeletedElement = element->getPosition();
			deletedPositions.push_back(make_pair(positionDeletedElement, element));
			positionToElement.removeElement(element);
		}

		// update elements with positions > position of deleted element
		sort(deletedPositions.begin(), deletedPositions.end());
		if (!deletedPositions.empty()) {
			PositionType minPosition = deletedPositions.front().first;
			PositionType maxPosition = deletedPositions.back().first;

			for (PositionType i = minPosition;  i < numElements;  i++) {
				positionToElement.removeKey(i);
			}

			for (vector<Element*>::iterator i = elements.begin(); i != elements.end(); i++) {
				Element * current = *i;

				if (current == 0) {
					continue;
				}                 

				PositionType pos = current->getPosition();

				if (pos >= minPosition) {
					//binary search for first position of deleted element >= current position
					//decrement position of current by index
					int s = 0, e = (int)deletedPositions.size();
					int m = e;
					if (pos <= maxPosition) {
						while (s<e) {
							m = (s+e)/2;
							if (deletedPositions[m].first < pos)
								s = m+1;
							else if (deletedPositions[m].first > pos)
								e = m;
							else 
								break;
						}
						m = (s+e)/2;
					}

					if (m<e && deletedPositions[m].second == current) {

						//element is in the list of elements to delete so do so

						Element* element = current;

						removeElementFromCubes(user, element, true, true, false); //deleteRules=false : see mantis (issue 2805)

						// we might have to update the string consolidation info
						bool isString = false;

						// delete all children
						if (element->getElementType() == Element::CONSOLIDATED) {
							ParentChildrenPair * pcp = parentToChildren.findKey(element);

							if (pcp != 0) {
								ElementsWeightType& ew = pcp->children;
								removeParentInChildren(element, &ew);

								// remove element from parent to child hashes 
								parentToChildren.removeKey(element);

								// delete mapping
								delete pcp;
							}

							isString = isStringConsolidation(element);

							if (isString) {
								stringConsolidations.removeKey(element);
							}
						}

						// simple string element
						else if (element->getElementType() == Element::STRING) {
							isString = true;
						}

						// remove element in parents
						removeChildInParents(user, element, isString);

						// delete element from hash_map nameToElement
						nameToElement.removeElement(element);

						numElements--;

						// add identifier of the element to freeElements
						freeElements.insert(element->getIdentifier());

						// and finaly delete the element
						delete element;

						*i = 0; //remove pointer from elements
						continue;
					}
					else {
						//element is not in the list so update element's position
						//first deleted element is at pos m
						current->setPosition(pos - m);
						positionToElement.addElement(current);
					}
				}
			}
		}

		Logger::trace<<"done!"<<endl;

		// the dimension has been changed
		setStatus(CHANGED);

		JournalFileWriter * journal = database->getJournal();
		if (journal != 0 && useJournal) {

			journal->appendCommand(Server::getUsername(user),
				Server::getEvent(),
				"DELETE_ELEMENTS");
			journal->appendInteger(identifier);
			journal->appendIdentifiers(&elementIdentifiers);
			journal->nextLine();
		}

		database->clearRuleCaches();
	}

	void BasicDimension::_destroyElementLeavePosition(User* user, Element* element) {

		// we might have to update the string consolidation info
		bool isString = false;

		// delete all children
		if (element->getElementType() == Element::CONSOLIDATED) {
			ParentChildrenPair * pcp = parentToChildren.findKey(element);

			if (pcp != 0) {
				ElementsWeightType& ew = pcp->children;
				removeParentInChildren(element, &ew);

				// remove element from parent to child hashes 
				parentToChildren.removeKey(element);

				// delete mapping
				delete pcp;
			}

			isString = isStringConsolidation(element);

			if (isString) {
				stringConsolidations.removeKey(element);
			}
		}

		// simple string element
		else if (element->getElementType() == Element::STRING) {
			isString = true;
		}

		// remove element in parents
		removeChildInParents(user, element, isString);

		// delete element from hash_map nameToElement
		nameToElement.removeElement(element);

		numElements--;

		// add identifier of the element to freeElements
		freeElements.insert(element->getIdentifier());

		// and finaly delete the element
		delete element;
	}

	void BasicDimension::deleteElements2(std::vector<Element *> elementList, User* user, bool useJournal)
	{
		checkElementAccessRight(user, RIGHT_DELETE);    

		if (user != 0 && ! changable) {
			throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
				"dimension cannot be changed",
				"dimension", name);
		}

		if (isLockedByCube()) {
			throw ErrorException(ErrorException::ERROR_DIMENSION_LOCKED, "dimension is used in a locked cube");
		}

		updateToken();

		// the level structure might change
		isValidLevel = false;
		isValidBaseElements = false;
		isValidSortedElements = false;

		std::vector<pair<PositionType, Element*> > deletedPositions; deletedPositions.reserve(elementList.size());    
		std::vector<IdentifierType> elementIdentifiers; elementIdentifiers.reserve(elementList.size());
		set<Element*> deletedElements;

		Logger::trace<<"deleting "<<elementList.size()<<" elements"<<endl;
		// delete cell path containing element 
		for (size_t i = 0; i<elementList.size(); i++) {
			Element* element = elementList[i];      
			PositionType positionDeletedElement = element->getPosition();
			deletedPositions.push_back(make_pair(positionDeletedElement, element));     
			elementIdentifiers.push_back(element->getIdentifier());
			deletedElements.insert(element);
		}    
		sort(deletedPositions.begin(), deletedPositions.end());
		Logger::trace<<"element positions sorted"<<endl;


		removeElementFromCubes(user, elementList, true, true, false); //deleteRules=false : see mantis (issue 2805)

		Logger::trace<<"elements deleted from cubes"<<endl;

		int elementCount = (int)positionToElement.size();

		// update elements with positions > position of deleted element
		for (size_t i = 0; i<deletedPositions.size(); i++) {
			int pos = deletedPositions[i].first;
			int nextPos = i+1<deletedPositions.size() ? deletedPositions[i+1].first : elementCount;
			Element* element = deletedPositions[i].second;
			//update positions

			positionToElement.removeKey(pos);
			for (int p = pos+1; p<nextPos; p++) {			
				Element* pEl = positionToElement.findKey(p);			
				positionToElement.removeKey(p);
				pEl->setPosition((PositionType)(p-i-1));
				positionToElement.addElement(pEl);
			}
			//delete element
			_destroyElementLeavePosition(user, element);
		}

		Logger::trace<<"elements deleted"<<endl;

		for (size_t e = 0; e<elements.size(); e++)
			if (elements[e]!=0 && deletedElements.find(elements[e])!=deletedElements.end())
				elements[e] = 0;

		Logger::trace<<"elements pointers updated"<<endl;
		Logger::trace<<elementList.size()<<" elements deleted"<<endl;

		// the dimension has been changed
		setStatus(CHANGED);

		JournalFileWriter * journal = database->getJournal();
		if (journal != 0 && useJournal) {

			journal->appendCommand(Server::getUsername(user),
				Server::getEvent(),
				"DELETE_ELEMENTS");
			journal->appendInteger(identifier);
			journal->appendIdentifiers(&elementIdentifiers);
			journal->nextLine();
		}

		database->clearRuleCaches();
	}


	set<Element*> BasicDimension::getBaseElements (Element* parent, bool* multiple) {
		set<Element*> baseElements;

		if (multiple != 0) { *multiple = false; }

		if (parent->getElementType() == Element::CONSOLIDATED) {
			const ElementsWeightType * children = getChildren(parent);

			for (ElementsWeightType::const_iterator iter = children->begin();
				iter != children->end();
				iter++) {
					bool subMultiple;
					set<Element*> subBase = getBaseElements(iter->first, &subMultiple);

					if (multiple != 0) { *multiple |= subMultiple; }
					baseElements.insert(subBase.begin(), subBase.end());
			}
		}
		else {
			const ParentsType * grandParent = getParents(parent);

			if (grandParent != 0 && grandParent->size() > 1 && multiple != 0) {
				*multiple = true;
			}

			baseElements.insert(parent);
		}

		return baseElements;
	}



	void BasicDimension::checkUpdateConsolidationType (User* user, Element* element) {
		bool hasStringChild = false;

		const ElementsWeightType * children = getChildren(element);

		for (ElementsWeightType::const_iterator i = children->begin();  i != children->end();  i++) {
			Element* child = (*i).first;

			switch (child->getElementType()) {
		case Element::STRING: 
			hasStringChild = true;
			break;

		case Element::CONSOLIDATED: 
			if (isStringConsolidation(child)) {
				hasStringChild = true;
			}
			break;

		default:
			// ignore type
			break;
			}
		}

		bool isString = isStringConsolidation(element);

		// type has changed!
		if (isString != hasStringChild) {
			if (hasStringChild) {
				stringConsolidations.addElement(element);
			}
			else {
				stringConsolidations.removeKey(element);

				// delete cell path containing element in string storage
				removeElementFromCubes(user, element, false, true, false, hasStringAttributes());
			}

			// check and update parents
			const ParentsType * p = getParents(element);

			for (ParentsType::const_iterator i = p->begin();  i != p->end();  i++) {
				checkUpdateConsolidationType(user, *i);
			}
		}      
	}


	void BasicDimension::updateTopologicalSortedElements () {
		// add parents first!

		if (! isValidSortedElements) {

			isValidSortedElements = true;

			set<Element*> knownElements; // list of known elements
			sortedElements.clear();
			for (vector<Element*>::iterator i = elements.begin(); i != elements.end(); i++) {
				Element * element = *i;

				set<Element*>::iterator find = knownElements.find(element);
				if (element != 0 && find == knownElements.end()) {
					// add parent
					addParrentsToSortedList(element, &knownElements);

					// add element
					sortedElements.push_back(element);
					knownElements.insert(element);
				}        
			}
		}
	}


	void BasicDimension::addParrentsToSortedList (Element* child, set<Element*>* knownElements) {
		const ParentsType* pt = getParents(child);

		if (pt == 0) {
			return;
		}

		for (ParentsType::const_iterator pti = pt->begin(); pti != pt->end(); pti++) {
			Element* parent = *pti;

			set<Element*>::iterator find = knownElements->find(parent);
			if (parent != 0 && find == knownElements->end()) {
				// add parents
				addParrentsToSortedList(parent, knownElements);

				// add elements
				sortedElements.push_back(parent);
				knownElements->insert(parent);
			}      
		}    
	}


	void BasicDimension::updateElementBaseElements () {
		if (! isValidBaseElements) {

			isValidBaseElements = true;

			// update the topological sorted list of elements
			updateTopologicalSortedElements();

			for (deque<Element*>::reverse_iterator i = sortedElements.rbegin(); i != sortedElements.rend(); i++) {
				Element * element = *i;
				element->clearBaseElements();

				map<IdentifierType, double> tmp;

				// update children
				const ElementsWeightType * children = getChildren(element);

				if (children->size() > 0) {
					for (ElementsWeightType::const_iterator i = children->begin();  i != children->end();  i++) {
						Element * child = i->first;
						double weight = i->second;
						//if (weight != 0.0) {
						const map<IdentifierType, double>* childBaseElements = child->getBaseElements(this);

						for (map<IdentifierType, double>::const_iterator c = childBaseElements->begin(); c != childBaseElements->end(); c++) {
							map<IdentifierType, double>::iterator find = tmp.find(c->first);
							if (find == tmp.end()) {
								tmp.insert( make_pair(c->first, weight * c->second));
							}
							else {
								find->second += weight * c->second;
							}
						}

						//}
					}
				}
				else {
					tmp.insert( make_pair(element->getIdentifier(), 1.0) );
				}

				element->setBaseElements(tmp);      
			}      
		}    
	}


	void BasicDimension::updateLevelIndentDepth () {
		if (! isValidLevel) {

			isValidLevel = true;

			// clear all info
			maxLevel  = 0;
			maxIndent = 0;
			maxDepth  = 0;

			// update the topological sorted list of elements
			updateTopologicalSortedElements();

			// update level
			updateLevel();

			// update depth and ident
			for (deque<Element*>::iterator i = sortedElements.begin(); i != sortedElements.end(); i++) {
				Element * element = *i;

				DepthType depth = 0;
				IndentType indent = 1;

				const ParentsType * parents = getParents(element);
				if (parents != 0 ) {
					// depth
					for (ParentsType::const_iterator i = parents->begin();  i != parents->end();  i++) {
						Element * parent = *i;

						DepthType d = parent->getDepth(this);
						if (depth <= d) {
							depth = d + 1;
						}
					}   

					// ident
					if (parents->size() > 0) {
						Element * firstParent = (*parents)[0];
						indent = firstParent->getIndent(this) + 1;
					}

				}

				if (maxDepth < depth) {
					maxDepth = depth;
				}

				if (maxIndent < indent) {
					maxIndent = indent;
				}

				element->setDepth(depth);
				element->setIndent(indent);
			}
		}
	}

	void BasicDimension::updateLevel () {    
		for (deque<Element*>::reverse_iterator i = sortedElements.rbegin(); i != sortedElements.rend(); i++) {
			Element * element = *i;

			ParentChildrenPair * pcp = parentToChildren.findKey(element);
			LevelType level = 0;

			if (pcp != 0) {
				ElementsWeightType * children = &pcp->children;
				for (ElementsWeightType::iterator childPair = children->begin(); childPair != children->end(); childPair++) {
					Element * child = childPair->first;
					LevelType l = child->getLevel(this);
					if (level <= l) {
						level = l + 1;
					}
				}        
			}

			if (maxLevel < level) {
				maxLevel = level;
			}

			element->setLevel(level);
		}
	}


	void BasicDimension::removeElementFromCubes (User* user,
		Element * element, 
		bool processStorageDouble,
		bool processStorageString,
		bool deleteRules,
		bool skipStorageStringInAttributeCubes) {

			vector<Cube*> cubes = database->getCubes(0);

			for (vector<Cube*>::const_iterator c = cubes.begin(); c != cubes.end(); c++)
			{		
				bool isAttributeCube = ((*c)->getType() == palo::SYSTEM) && (((SystemCube*)(*c))->getSubType() == SystemCube::ATTRIBUTES_CUBE);
				bool removeStrings = processStorageString && !(skipStorageStringInAttributeCubes && isAttributeCube);

				/*if (isAttributeCube && removeStrings)
					isAttributeCube = true;*/

				(*c)->deleteElement(Server::getUsername(user),
					Server::getEvent(),
					this,
					element->getIdentifier(),
					processStorageDouble,
					removeStrings,
					deleteRules);
			}        
	}

	void BasicDimension::removeElementFromCubes (User* user,
		std::vector<Element*> elements, 
		bool processStorageDouble,
		bool processStorageString,
		bool deleteRules,
		bool skipStorageStringInAttributeCubes) {

			vector<Cube*> cubes = database->getCubes(0);

			IdentifiersType elementIds; elementIds.reserve(elements.size());
			for (size_t i = 0; i<elements.size(); i++)
				elementIds.push_back(elements[i]->getIdentifier());

			for (vector<Cube*>::const_iterator c = cubes.begin(); c != cubes.end(); c++)
			{		
				bool isAttributeCube = ((*c)->getType() == palo::SYSTEM) && (((SystemCube*)(*c))->getSubType() == SystemCube::ATTRIBUTES_CUBE);
				bool removeStrings = processStorageString && !(skipStorageStringInAttributeCubes && isAttributeCube);

				
				(*c)->deleteElements(Server::getUsername(user),
					Server::getEvent(),
					this,
					elementIds,
					processStorageDouble,
					removeStrings,
					deleteRules);
			}        
	}



	size_t BasicDimension::getMemoryUsageIndex () {
		size_t result = 0;

		return result; 
	}



	size_t BasicDimension::getMemoryUsageStorage () {
		size_t result = sizeof(Element) * elements.size(); 

		return result;    
	}


	void BasicDimension::checkElementName (const string& name) {
		if (name.empty()) {
			throw ParameterException(ErrorException::ERROR_INVALID_ELEMENT_NAME,
				"element name is empty",
				"name", name);
		}

		if (name[0] == ' ' || name[name.length()-1] == ' ') {
			throw ParameterException(ErrorException::ERROR_INVALID_ELEMENT_NAME,
				"element name begins or ends with a space character",
				"name", name);
		}

		for (size_t i = 0; i < name.length(); i++) {
			if (0 <= name[i] && name[i] < 32) {
				throw ParameterException(ErrorException::ERROR_INVALID_ELEMENT_NAME,
					"element name contains an illegal character",
					"name", name);
			}
		}
	}  
}
