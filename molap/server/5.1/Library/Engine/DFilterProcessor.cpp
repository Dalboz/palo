/* 
 *
 * Copyright (C) 2006-2013 Jedox AG
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (Version 2) as published
 * by the Free Software Foundation at http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * If you are developing and distributing open source applications under the
 * GPL License, then you are free to use Palo under the GPL License.  For OEMs,
 * ISVs, and VARs who distribute Palo with their products, and do not license
 * and distribute their source code under the GPL, Jedox provides a flexible
 * OEM Commercial License.
 *
 * \author Jiri Junek, qBicon s.r.o., Prague, Czech Republic
 * \author Ladislav Morva, qBicon s.r.o., Prague, Czech Republic
 * 
 *
 */

#include "Olap/Cube.h"
#include "InputOutput/Condition.h"
#include "InputOutput/ConstantCondition.h"
#include "Engine/DFilterProcessor.h"

namespace palo {

DFilterQuantificationProcessor::DFilterQuantificationProcessor(PEngineBase engine, CPPlanNode node) :
	ProcessorBase(true, engine), init(true), numCellsPerElement(0), strCellsPerElement(0), validValue(false)
{
	const QuantificationPlanNode *plan = dynamic_cast<const QuantificationPlanNode *>(node.get());
	area = plan->getFilteredArea();
	quantType = plan->getQuantificationType();
	filteredDim = plan->getDimIndex();
	if (filteredDim == (uint32_t)NO_DFILTER) {
		throw ErrorException(ErrorException::ERROR_INTERNAL, "DFilterQuantificationProcessor: invalid filtered dimension");
	}
	filteredDimSize = (uint32_t)area->getDim(filteredDim)->size();
	calcRules = plan->getCalcRules();
	key = *area->pathBegin();
	maxElemCount = plan->getMaxCount();
	condition = plan->getCondition().get();
	if (quantType != QuantificationPlanNode::EXISTENCE && !condition) {
		throw ErrorException(ErrorException::ERROR_INTERNAL, "DFilterQuantificationProcessor: invalid condition");
	}
	isVirtual = plan->isVirtual();
	if (!isVirtual) {
		numCellsPerElement = plan->getNumCellCount();
		strCellsPerElement = plan->getStrCellCount();
	}
}

bool DFilterQuantificationProcessor::next()
{
	if (init) {
		bool bCalc;
		if (isVirtual || quantType == QuantificationPlanNode::EXISTENCE) {
			bCalc = true;
		} else if (quantType == QuantificationPlanNode::ANY_STR) {
			bCalc = strCellsPerElement != 0;
		} else {
			bCalc = numCellsPerElement != 0;
		}
		if (bCalc) {
			if (quantType == QuantificationPlanNode::ALL) {
				CPSet filteredSet = area->getDim(filteredDim);
				for (Set::Iterator fit = filteredSet->begin(); fit != filteredSet->end(); ++fit) {
					subset.insert(*fit);
				}
			}

			RulesType rulesType = calcRules ? RulesType(ALL_RULES | NO_RULE_IDS) : NO_RULES;
			PPlanNode plan = area->getCube()->createPlan(area, cellType(), rulesType, true, UNLIMITED_SORTED_PLAN);
			vector<PPlanNode> children;
			if (plan->getType() == UNION) {
				children = plan->getChildren();
			} else {
				children.push_back(plan);
			}

			CalcNodeType calcType = CALC_UNDEF;
			bool isComplete = false;
			while (nextCalcNodeType(calcType, false)) {
				for (size_t i = 0; i < children.size(); i++) {
					bool isAggr;
					bool isRule;
					PlanNodeType childType = children[i]->getType();
					if (!matchingPlanNode(calcType, childType, isAggr, isRule)) {
						continue;
					}
					set<IdentifierType> &rset = quantType == QuantificationPlanNode::ALL ? complement : subset;
					PArea rarea = isVirtual ? PArea(new Area(*children[i]->getArea())) : children[i]->getArea()->reduce(filteredDim, rset);
					if (rarea->getSize()) {
						if (quantType == QuantificationPlanNode::EXISTENCE && isAggr && !isVirtual) {
							processAggregation(children[i], rarea, isComplete);
						} else {
							processReducedArea(children[i], rarea, 0, isRule, isComplete);
						}
					}
					if (isComplete) {
						break;
					}
				}
				if (isComplete) {
					break;
				}
			}
			if (quantType != QuantificationPlanNode::EXISTENCE && !isComplete && !isVirtual) {
				processEmptyCells(isComplete);
			}
		}
		pos = subset.begin();
		init = false;
	} else {
		++pos;
	}

	if (pos == subset.end()) {
		return false;
	} else {
		key[filteredDim] = *pos;
		validValue = quantType == QuantificationPlanNode::EXISTENCE;
		return true;
	}
}

const CellValue &DFilterQuantificationProcessor::getValue() {
	if (!validValue) {
		map<IdentifierType, CellValue>::iterator vit = values.find(*pos);
		if (vit == values.end()) {
			throw ErrorException(ErrorException::ERROR_INTERNAL, "missing value in DFilterQuantificationProcessor!");
		} else {
			cellValue = vit->second;
			validValue = true;
		}
	}
	return cellValue;
}

void DFilterQuantificationProcessor::reset()
{
	init = true;
	subset.clear();
	complement.clear();
	counterTrue.clear();
	counterFalse.clear();
	values.clear();
	validValue = false;
}

void DFilterQuantificationProcessor::processAggregation(CPPlanNode planNode, PArea rarea, bool &isComplete)
{
	PCubeArea ca(new CubeArea(area->getDatabase(), area->getCube(), *rarea));
	PAggregationMaps aggrMaps(new AggregationMaps());
	ca->expandBase(aggrMaps.get(), &filteredDim);

	const vector<PPlanNode> children = planNode->getChildren();
	CalcNodeType calcType = CALC_UNDEF;

	while (nextCalcNodeType(calcType, true)) {
		for (size_t i = 0; i < children.size(); i++) {
			bool isAggr;
			bool isRule;
			if (!matchingPlanNode(calcType, children[i]->getType(), isAggr, isRule)) {
				continue;
			}

			rarea = PArea(new Area(*children[i]->getArea()->reduce(filteredDim, subset)));
			if (rarea->getSize()) {
				processReducedArea(children[i], rarea, &(*aggrMaps)[filteredDim], isRule, isComplete);
				if (isComplete) {
					return;
				}
			}
		}
	}
}

void DFilterQuantificationProcessor::processReducedArea(CPPlanNode planNode, PArea rarea, AggregationMap *aggrMap, bool isRule, bool &isComplete)
{
	PCellStream cs;
	if (planNode->getType() == CACHE) {
		const CachePlanNode *childPlan = dynamic_cast<const CachePlanNode *>(planNode.get());
		// TODO: -jj- check if default value is needed - childPlan->getDefaultValue()
		cs = childPlan->getCacheStorage()->getCellValues(rarea);
	} else if (planNode->getType() == QUERY_CACHE) {
		const QueryCachePlanNode *childPlan = dynamic_cast<const QueryCachePlanNode *>(planNode.get());
		cs = Context::getContext()->getQueryCache(childPlan->getCube())->getFilteredValues(planNode->getArea(), childPlan->getDefaultValue());
	} else {
		PCubeArea childArea(new CubeArea(area->getDatabase(), area->getCube(), *rarea));
		RulesType rulesType = calcRules || planNode->getType() != SOURCE ? RulesType(ALL_RULES | NO_RULE_IDS) : NO_RULES;
		PPlanNode childPlan = area->getCube()->createPlan(childArea, cellType(), rulesType, !isVirtual, UNLIMITED_SORTED_PLAN);
		if (childPlan) {
			cs = area->getCube()->evaluatePlan(childPlan, EngineBase::ANY, true);
		} else {
			return;
		}
	}

	while (cs->next()) {
		IdentifierType id = cs->getKey()[filteredDim];
		const CellValue &val = cs->getValue();
		if (aggrMap) {
			for (AggregationMap::TargetReader targets = aggrMap->getTargets(id); !targets.end(); ++targets) {
				checkValue(*targets, val, isComplete);
				if (isComplete) {
					break;
				}

			}
		} else {
			checkValue(id, val, isComplete);
		}
		if (isComplete) {
			return;
		}
	}
}

void DFilterQuantificationProcessor::processEmptyCells(bool &isComplete)
{
	bool zeroCond = condition->check(quantType == QuantificationPlanNode::ANY_STR ? CellValue::NullString : CellValue::NullNumeric);
	CPSet filteredSet = area->getDim(filteredDim);

	if (quantType == QuantificationPlanNode::ALL) {
		complement.clear();
		for (Set::Iterator fit = filteredSet->begin(); fit != filteredSet->end(); ++fit) {
			IdentifierType id = *fit;

			set<IdentifierType>::iterator sit = subset.find(id);
			if (sit != subset.end()) {
				if (counterFalse.find(id) != counterFalse.end()) { // the condition failed for at least one value
					values.erase(id);
					subset.erase(id);
					continue;
				}
				map<IdentifierType, double>::iterator mit = counterTrue.find(id);
				if (mit == counterTrue.end() || mit->second < numCellsPerElement) { // the condition was not met for all values
					if (zeroCond) {
						values.insert(make_pair(id, CellValue::NullNumeric));
					} else {
						values.erase(id);
						subset.erase(id);
					}
				}
			}
		}
		checkLimit();
	} else { // QuantificationPlanNode::ANY_NUM and ANY_STR
		if (zeroCond) {
			for (Set::Iterator fit = filteredSet->begin(); fit != filteredSet->end(); ++fit) {
				IdentifierType id = *fit;

				set<IdentifierType>::iterator sit = subset.find(id);
				if (sit == subset.end()) {
					//if is there at least one #N/A add 'id' to 'subset'
					map<IdentifierType, double>::iterator mit = counterFalse.find(id);
					if (mit == counterFalse.end() || mit->second < (quantType == QuantificationPlanNode::ANY_STR ? strCellsPerElement : numCellsPerElement)) {
						insertId(id, quantType == QuantificationPlanNode::ANY_NUM ? CellValue::NullNumeric : CellValue::NullString, isComplete);
						if (isComplete) {
							break;
						}
					}
				}
			}
		}
	}
}

bool DFilterQuantificationProcessor::checkValue(IdentifierType id, const CellValue &value, bool &isComplete)
{
	bool changed = false;
	if (quantType == QuantificationPlanNode::ALL) {
		if (subset.find(id) != subset.end()) {
			bool cond = condition->check(value);
			increaseCounter(id, cond);
			if (cond) {
				values.insert(make_pair(id, value));
			} else {
				values.erase(id);
				subset.erase(id);
				complement.insert(id);
				changed = true;
			}
		}
	} else {
		bool cond = false;
		if (quantType == QuantificationPlanNode::ANY_NUM || quantType == QuantificationPlanNode::ANY_STR) {
			cond = condition->check(value);
			increaseCounter(id, cond);
		}
		if (subset.find(id) == subset.end()) {
			if (quantType == QuantificationPlanNode::EXISTENCE || cond) {
				insertId(id, value, isComplete);
				changed = true;
			}
		}
	}
	return changed;
}

void DFilterQuantificationProcessor::insertId(IdentifierType id, const CellValue &value, bool &isComplete)
{
	checkLimit();
	subset.insert(id);
	values.insert(make_pair(id, value));
	isComplete = subset.size() == filteredDimSize;
}

void DFilterQuantificationProcessor::increaseCounter(IdentifierType id, bool cond)
{
	map<IdentifierType, double> &counter = cond ? counterTrue : counterFalse;
	map<IdentifierType, double>::iterator it = counter.find(id);
	if (it == counter.end()) {
		counter.insert(make_pair(id, 1.0));
	} else {
		it->second = it->second + 1.0;
	}
}

CubeArea::CellType DFilterQuantificationProcessor::cellType() const
{
	CubeArea::CellType result = CubeArea::NONE;
	switch (quantType) {
	case QuantificationPlanNode::ALL:
	case QuantificationPlanNode::ANY_NUM:
		result = CubeArea::NUMERIC;
		break;
	case QuantificationPlanNode::ANY_STR:
		result = CubeArea::BASE_STRING;
		break;
	case QuantificationPlanNode::EXISTENCE:
		result = CubeArea::ALL;
		break;
	}
	return result;
}

void DFilterQuantificationProcessor::checkLimit() const
{
	if (maxElemCount && subset.size() >= maxElemCount) {
		throw ErrorException(ErrorException::ERROR_MAX_ELEM_REACHED, "subset too big");
	}
}

bool DFilterQuantificationProcessor::nextCalcNodeType(CalcNodeType &calcType, bool skipAggregation)
{
	switch (calcType) {
	case CALC_UNDEF:
		calcType = CALC_SOURCE;
		break;
	case CALC_SOURCE:
		calcType = CALC_CACHE;
		break;
	case CALC_CACHE:
		calcType = skipAggregation ? CALC_RULE : CALC_AGGREGATION;
		break;
	case CALC_AGGREGATION:
		calcType = CALC_RULE;
		break;
	case CALC_RULE:
		return false;
	}
	return true;
}

bool DFilterQuantificationProcessor::matchingPlanNode(CalcNodeType calcType, PlanNodeType planType, bool &isAggr, bool &isRule)
{
	switch (planType) {
	case SOURCE:
	case CACHE:
	case QUERY_CACHE:
		isAggr = false;
		isRule = false;
		break;
	case ADDITION:
	case SUBTRACTION:
	case MULTIPLICATION:
	case DIVISION:
	case LEGACY_RULE:
	case TRANSFORMATION:
	case CONSTANT:
	case COMPLETE:
		isAggr = false;
		isRule = true;
		break;
	case AGGREGATION:
		isAggr = true;
		isRule = false;
		break;
	case CELLMAP:
	case CELL_RIGHTS:
	case ROUNDCORRECT:
	case QUANTIFICATION:
		throw ErrorException(ErrorException::ERROR_INTERNAL, "invalid PlanNode type in DFilterQuantificationProcessor!");
	default:
		throw ErrorException(ErrorException::ERROR_INTERNAL, "unknown PlanNode type in DFilterQuantificationProcessor!");
	}

	switch (calcType) {
	case CALC_UNDEF:
		return false;
	case CALC_SOURCE:
		return planType == SOURCE;
	case CALC_CACHE:
		return planType == CACHE || planType == QUERY_CACHE;
	case CALC_AGGREGATION:
		return planType == AGGREGATION;
	case CALC_RULE:
		return planType != SOURCE && planType != CACHE && planType != AGGREGATION;
	}
	return false;
}

}
