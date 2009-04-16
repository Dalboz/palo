////////////////////////////////////////////////////////////////////////////////
/// @brief palo enterprise rule
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

#include "Olap/Rule.h"

#include "Exceptions/ParameterException.h"

#include "Parser/FunctionNodePaloMarker.h"
#include "Parser/RuleNode.h"
#include "Parser/SourceNode.h"

namespace palo {

  Rule::~Rule () {
    delete rule;

    if (restrictedRule != 0) {
      delete restrictedRule;
    }

    vector<RuleMarker*> m = markers;
    markers.clear();

    for (vector<RuleMarker*>::iterator i = m.begin();  i != m.end();  i++) {
      RuleMarker * marker = *i;

      marker->getFromCube()->removeFromMarker(marker);
      marker->getToCube()->removeToMarker(marker);
    }

    for (vector<RuleMarker*>::iterator i = m.begin();  i != m.end();  i++) {
      RuleMarker * marker = *i;

      delete marker;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // destination related methods
  ////////////////////////////////////////////////////////////////////////////////

  bool Rule::withinArea (CellPath* cellPath) {
    checkCubeToken();

    return Cube::isInArea(cellPath, rule->getDestinationArea());
  }



  bool Rule::withinRestricted (CellPath* cellPath) {
    checkCubeToken();

    return Cube::isInArea(cellPath, &restrictedArea);
  }



  bool Rule::within (CellPath* cellPath) {
    checkCubeToken();

    // use destination area of definition
    if (! Cube::isInArea(cellPath, rule->getDestinationArea())) {
      return false;
    }

    // Check the rule type of the original rule. Linear Base Rules which are
    // applied to consolidations are handled at a latter stage in Cube.cpp.
    RuleNode::RuleOption option = rule->getRuleOption();

    if (option == RuleNode::NONE) {
      return true;
    }
    else if (option == RuleNode::CONSOLIDATION) {
      return ! cellPath->isBase();
    }
    else if (option == RuleNode::BASE) {
      return cellPath->isBase();
    }

    return false;
  }



  bool Rule::contains (CellPath* cellPath) {
    checkCubeToken();

    return Cube::isInArea(cellPath, &containsArea);
  }



  bool Rule::containsRestricted (CellPath* cellPath) {
    checkCubeToken();
    
    return Cube::isInArea(cellPath, &containsRestrictedArea);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // evaluate rule and optimization
  ////////////////////////////////////////////////////////////////////////////////

  Cube::CellValueType Rule::getValue( CellPath* cellPath,
	                                    User* user,
	                                    bool* skipRule, //CONTINUE 
	                                    bool* skipAllRules, //STET 
	                                    bool* usesDifferentDatabase,
	                                    set< pair<Rule*, IdentifiersType> > * ruleHistory ) const {

    // result type of getValue
    Node::RuleValueType result;

    // true, if we encounter the STET or CONTINUE command
	*skipRule = false;	//CONTINUE
	*skipAllRules = false;	//STET

    // use optimized rule
    if (isOptimized) {
      result = restrictedRule->getValue(cellPath, user, usesDifferentDatabase, ruleHistory);
    }

    // use standard rule
    else {
      result = rule->getValue(cellPath, user, usesDifferentDatabase, ruleHistory);
    }

    // construct the return value
    Cube::CellValueType value;

    // check result type
	if ( result.type == Node::STET ) {
		*skipAllRules = true;
		value.rule = NO_RULE;
	} else if ( result.type == Node::CONTINUE ) {
		*skipRule = true;
		value.rule = NO_RULE;
	} else if (cellPath->getPathType() == Element::STRING) {
      value.type = Element::STRING;
      
      if (result.type == Node::STRING) {
        value.charValue = result.stringValue;
      }
      else {
        value.charValue.clear();
      }

      value.rule = identifier;
    }
    else {
      value.type = Element::NUMERIC;

      if (result.type == Node::NUMERIC) {
        value.doubleValue = result.doubleValue;
      }
      else {
        value.doubleValue = 0.0;
      }

      value.rule = identifier;
    }

    // and return
    return value;
  }



  bool Rule::isLinearRule () {
    checkCubeToken();

    return linearRule;
  }



  bool Rule::isRestrictedRule () {
    checkCubeToken();

    return isOptimized;
  }



  void Rule::optimizeRule () {

    // reset optimization
    if (isOptimized) {
      delete restrictedRule;

      restrictedRule = 0;
      restrictedDimension = 0;
      restrictedIdentifiers.clear();
    }

    isOptimized = false;
    linearRule = false;

    // only optimize rules without markers
    if (! markers.empty()) {
      Logger::trace << "cannot optimize rules with markers" << endl;
      return;
    }

    // try to optimize the rule
    isOptimized = ruleOptimizer.optimize(rule);

    // update parameter
    if (isOptimized) {
      restrictedRule        = ruleOptimizer.getRestrictedRule();
      restrictedDimension   = ruleOptimizer.getRestrictedDimension();
      restrictedIdentifiers = ruleOptimizer.getRestrictedIdentifiers();

      restrictedArea = *(rule->getDestinationArea());

      int pos = 0;
      const vector<Dimension*> * dimensions = cube->getDimensions();

      for (vector<Dimension*>::const_iterator iter = dimensions->begin();  iter != dimensions->end();  iter++, pos++) {
        if (*iter == restrictedDimension) {
          restrictedArea[pos] = restrictedIdentifiers;
          break;
        }
      }
    }

    linearRule = ruleOptimizer.isLinearRule();
  }

  ////////////////////////////////////////////////////////////////////////////////
  // auxillary functions
  ////////////////////////////////////////////////////////////////////////////////

  void Rule::checkCubeToken () {
    if (cube->getToken() == cubeToken) {
      return;
    }
    
    optimizeRule();
    computeContains();
    
    if (isOptimized) {
      computeContainsRestricted();
    }

    cubeToken = cube->getToken();
  }



  void Rule::computeContains () {
    AreaNode::Area* area = rule->getDestinationArea();

    containsArea.clear();
    containsArea.resize(area->size());

    AreaNode::Area::iterator dst = containsArea.begin();
    AreaNode::Area::iterator src = area->begin();
    vector<Dimension*>::const_iterator dims = cube->getDimensions()->begin();

    for (; src != area->end(); dst++, src++, dims++) {
      set<IdentifierType>& ddt = *dst;
      set<IdentifierType>& sst = *src;

      if (sst.empty()) {
        // empty means no restriction, keep that in destination
      }
      else {
        Dimension * d = *dims;

        for (set<IdentifierType>::iterator iter = sst.begin();  iter != sst.end();  iter++) {
          IdentifierType id = *iter;
          Element * e = d->findElement(id, 0);
          set<Element*> ac = d->ancestors(e);

          for (set<Element*>::iterator res = ac.begin();  res != ac.end();  res++) {
            Element * re = *res;
            ddt.insert(re->getIdentifier());
          }
        }
      }
    }
  }



  void Rule::computeContainsRestricted () {
    AreaNode::Area* area = &restrictedArea;

    containsRestrictedArea.clear();
    containsRestrictedArea.resize(area->size());

    AreaNode::Area::iterator dst = containsRestrictedArea.begin();
    AreaNode::Area::iterator src = area->begin();
    vector<Dimension*>::const_iterator dims = cube->getDimensions()->begin();

    for (; src != area->end(); dst++, src++, dims++) {
      set<IdentifierType>& ddt = *dst;
      set<IdentifierType>& sst = *src;

      if (sst.empty()) {
        // empty means no restriction, keep that in destination
      }
      else {
        Dimension * d = *dims;

        for (set<IdentifierType>::iterator iter = sst.begin();  iter != sst.end();  iter++) {
          IdentifierType id = *iter;
          Element * e = d->findElement(id, 0);
          set<Element*> ac = d->ancestors(e);

          for (set<Element*>::iterator res = ac.begin();  res != ac.end();  res++) {
            Element * re = *res;
            ddt.insert(re->getIdentifier());
          }
        }
      }
    }
  }



  static RuleMarker * convertMarker (Cube* cube,
                                     AreaNode::Area * destination,
                                     Node* node) {
    if (node->getNodeType() == Node::NODE_SOURCE_NODE) {
      SourceNode* source = dynamic_cast<SourceNode*>(node);
      AreaNode::Area* area = source->getNodeArea();

      RuleMarker * marker = new RuleMarker(cube, area, destination);
      Logger::trace << "using " << *marker << endl;

      return marker;
    }
    else if (node->getNodeType() == Node::NODE_FUNCTION_PALO_MARKER) {
      FunctionNodePaloMarker* source = dynamic_cast<FunctionNodePaloMarker*>(node);
      const string& databaseName = source->getDatabaseName();
      const string& cubeName = source->getCubeName();
      const vector<Node*>& path = source->getPath();

      Database* database = cube->getDatabase()->getServer()->findDatabaseByName(databaseName, 0);
      Cube* fromCube = database->findCubeByName(cubeName, 0);

      if (fromCube->getDimensions()->size() != path.size()) {
        throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                 "length of path does not match dimension of cube",
                                 "number dimensions", (int) fromCube->getDimensions()->size());
      }

      RuleMarker * marker = new RuleMarker(fromCube, cube, path, destination);
      Logger::trace << "using " << *marker << endl;

      return marker;
    }
    else {
      throw ErrorException(ErrorException::ERROR_INTERNAL, "unknown node type as marker");
    }
  }



  void Rule::computeMarkers () {
    markersComputed = true;

    // get internal and external marker and the destination
    const vector<Node*>& external = rule->getExternalMarkers();
    const vector<Node*>& internal = rule->getInternalMarkers();
    AreaNode::Area * destination = rule->getDestinationArea();
    
    // remove old markers, start with a fresh list
    vector<RuleMarker*> m = markers;
    markers.clear();

    for (vector<RuleMarker*>::iterator i = m.begin();  i != m.end();  i++) {
      RuleMarker * marker = *i;

      marker->getFromCube()->removeFromMarker(marker);
      marker->getToCube()->removeToMarker(marker);
    }

    for (vector<RuleMarker*>::iterator i = m.begin();  i != m.end();  i++) {
      RuleMarker * marker = *i;

      delete marker;
    }

    if (destination == 0) {
      return;
    }

    // construct the list of markers, push markers to cube
    for (vector<Node*>::const_iterator i = external.begin();  i != external.end();  i++) {
      try {
        RuleMarker * marker = convertMarker(cube, destination, *i);

        marker->getFromCube()->addFromMarker(marker);
        marker->getToCube()->addToMarker(marker);

        markers.push_back(marker);
      }
      catch (const ErrorException& ex) {
        Logger::warning << "cannot convert marker: " << ex.getMessage() << " (" << ex.getDetails() << ")" << endl;
      }
    }

    for (vector<Node*>::const_iterator i = internal.begin();  i != internal.end();  i++) {
      try {
        RuleMarker * marker = convertMarker(cube, destination, *i);

        marker->getFromCube()->addFromMarker(marker);
        marker->getToCube()->addToMarker(marker);

        markers.push_back(marker);
      }
      catch (const ErrorException& ex) {
        Logger::warning << "cannot convert marker: " << ex.getMessage() << " (" << ex.getDetails() << ")" << endl;
      }
    }
  }



  void Rule::removeMarkers () {
    vector<RuleMarker*> m = markers;
    markers.clear();

    for (vector<RuleMarker*>::iterator i = m.begin();  i != m.end();  i++) {
      RuleMarker* marker = *i;

      if (marker != 0) {
        marker->getFromCube()->removeFromMarker(marker);
        marker->getToCube()->removeToMarker(marker);
      }
    }
  }
}
