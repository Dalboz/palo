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
 * 
 *
 */

#include "PaloDocumentation/CubeAnalyzerDocumentation.h"

#include <iostream>
#include <sstream>

#include "Olap/Dimension.h"
#include "Olap/Rule.h"
#include "Engine/EngineBase.h"

namespace palo {

void CubeAnalyzerDocumentation::analyze(PPlanNode plan, int level)
{
	if (!plan) {
		return;
	}
	if (plan->getType() == UNION) {
		for (vector<PPlanNode>::const_iterator nit = plan->getChildren().begin(); nit != plan->getChildren().end(); ++nit) {
			analyze(*nit, level+1);
		}
	} else {
		double areaSize = plan->getArea() ? plan->getArea()->getSize() : 0;
		totalCells += areaSize;
		if (areaSize) {
			totalAreas++;
		}
		if (plan->getType() == SOURCE) {
			// type?
			baseCells += areaSize;
			baseAreas++;
		} else if (plan->getType() == AGGREGATION) {
			aggregatedCells += areaSize;
			aggregatedAreas++;
		} else if (plan->getType() == LEGACY_RULE) {
			const LegacyRulePlanNode *rpn = dynamic_cast<const LegacyRulePlanNode *>(plan.get());
			ruleCells += areaSize;
			ruleAreas++;
			if (rpn->useMarkers()) {
				markedRuleCells += areaSize;
				markedRuleAreas++;
			} else {
				legacyRuleCells += areaSize;
				legacyRuleAreas++;
			}
		} else if (plan->getType() == COMPLETE) {
			const CompletePlanNode *cpn = dynamic_cast<const CompletePlanNode *>(plan.get());
			if (cpn->getRuleId() != NO_IDENTIFIER) {
				ruleAreas++;
				ruleCells += areaSize;
				newRuleCells += areaSize;
				newRuleAreas++;
			}
		}
	}
}

CubeAnalyzerDocumentation::CubeAnalyzerDocumentation(PDatabase database, PCube cube, PPlanNode plan, const string& message) :
	BrowserDocumentation(message)
{
	totalCells = 0;
	aggregatedCells = 0;
	baseCells = 0;
	ruleCells = 0;
	newRuleCells = 0;
	markedRuleCells = 0;
	legacyRuleCells = 0;
	totalAreas = 0;
	aggregatedAreas = 0;
	baseAreas = 0;
	ruleAreas = 0;
	newRuleAreas = 0;
	markedRuleAreas = 0;
	legacyRuleAreas = 0;

	values["@database_identifier"] = vector<string>(1, StringUtils::convertToString(database->getIdentifier()));
	values["@cube_identifier"] = vector<string>(1, StringUtils::convertToString(cube->getId()));

	analyze(plan, 0);

	vector<string> sizesPerc;
	vector<string> sizesCells;
	vector<string> sizesAreas;
	vector<string> typeDescription;

	if (totalCells) {
		sizesPerc.push_back(UTF8Comparer::doubleToString(100*aggregatedCells/totalCells, 0, 2));
		sizesCells.push_back(StringUtils::convertToString(aggregatedCells));
		sizesAreas.push_back(StringUtils::convertToString(aggregatedAreas));
		typeDescription.push_back("Aggregated cells");

		sizesPerc.push_back(UTF8Comparer::doubleToString(100*baseCells/totalCells, 0, 2));
		sizesCells.push_back(StringUtils::convertToString(baseCells));
		sizesAreas.push_back(StringUtils::convertToString(baseAreas));
		typeDescription.push_back("Base cells");

		sizesPerc.push_back(UTF8Comparer::doubleToString(100*ruleCells/totalCells, 0, 2));
		sizesCells.push_back(StringUtils::convertToString(ruleCells));
		sizesAreas.push_back(StringUtils::convertToString(ruleAreas));
		typeDescription.push_back("Rule calculated cells - total");

		sizesPerc.push_back(UTF8Comparer::doubleToString(100*newRuleCells/totalCells, 0, 2));
		sizesCells.push_back(StringUtils::convertToString(newRuleCells));
		sizesAreas.push_back(StringUtils::convertToString(newRuleAreas));
		typeDescription.push_back("Rule calculated cells - new engine");

		sizesPerc.push_back(UTF8Comparer::doubleToString(100*markedRuleCells/totalCells, 0, 2));
		sizesCells.push_back(StringUtils::convertToString(markedRuleCells));
		sizesAreas.push_back(StringUtils::convertToString(markedRuleAreas));
		typeDescription.push_back("Rule calculated cells - marked rules");

		sizesPerc.push_back(UTF8Comparer::doubleToString(100*legacyRuleCells/totalCells, 0, 2));
		sizesCells.push_back(StringUtils::convertToString(legacyRuleCells));
		sizesAreas.push_back(StringUtils::convertToString(legacyRuleAreas));
		typeDescription.push_back("Rule calculated cells - slow rules");
	}

	values["@sizes_perc"] = sizesPerc;
	values["@sizes_cells"] = sizesCells;
	values["@sizes_area"] = sizesAreas;
	values["@types"] = typeDescription;
}

}
