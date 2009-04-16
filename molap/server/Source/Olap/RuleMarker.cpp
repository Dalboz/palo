////////////////////////////////////////////////////////////////////////////////
/// @brief palo rule marker
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

#include "Olap/RuleMarker.h"

#include "Olap/MarkerStorage.h"

#include "Parser/StringNode.h"
#include "Parser/VariableNode.h"

namespace palo {
  RuleMarker::RuleMarker(Cube* allCube,
                         const AreaNode::Area * fromArea,
                         const AreaNode::Area * toArea)
    : fromCube(allCube),
      toCube(allCube) {

    size_t nd = toCube->getDimensions()->size();

    // [[ ]] does not allow for permuations
    permutations = new uint32_t[nd];
    fixed = new uint32_t[nd];

    for (size_t i = 0;  i < nd;  i++) {
      permutations[i] = (uint32_t) i;
    }

    // convert FROM AREA description into a list of sets of base elements
    vector<Dimension*>::const_iterator d = fromCube->getDimensions()->begin();

    for (AreaNode::Area::const_iterator i = fromArea->begin();  i != fromArea->end();  i++, d++) {
      const set<IdentifierType>& s = *i;
      Dimension* dimension = *d;

      // s is either empty or contains one element, unfold to base elements
      if (s.empty()) {
        fromBase.push_back(s);
      }
      else {
        set<Element*> e = dimension->getBaseElements(dimension->lookupElement(*s.begin()), 0);
        set<IdentifierType> base;

        for (set<Element*>::iterator j = e.begin();  j != e.end();  j++) {
          Element* element = *j;

          base.insert(element->getIdentifier());
        }

        fromBase.push_back(base);
      }
    }


    // convert TO AREA description into a list of identifiers
    size_t f = 0;

    for (AreaNode::Area::const_iterator i = toArea->begin();  i != toArea->end();  i++, f++) {
      const set<IdentifierType>& s = *i;

      if (! s.empty()) {
        fixed[f] = *s.begin();
        permutations[f] = NO_IDENTIFIER;
      }
    }

    // no mapping needed
    useMapping = false;
  }



  // path contains either constants or variables. A constant is a
  // string denoting a dimension element in the fromCube. A variable
  // is a dimension shared by both the fromCube and the toCube or at
  // least two dimensions with the same elements.

  RuleMarker::RuleMarker(Cube* fromCube,
                         Cube* toCube,
                         const vector<Node*>& path,
                         const AreaNode::Area * toArea)
    : fromCube(fromCube),
      toCube(toCube) {

    // split into constants and variables
    vector<string> constants;
    vector<string> variables;

    for (vector<Node*>::const_iterator i = path.begin();  i != path.end();  i++) {
      Node* node = *i;

      if (node->getNodeType() == Node::NODE_STRING_NODE) {
        constants.push_back(dynamic_cast<StringNode*>(node)->getStringValue());
        variables.push_back("");
      }
      else if (node->getNodeType() == Node::NODE_VARIABLE_NODE) {
        constants.push_back("");
        variables.push_back(dynamic_cast<VariableNode*>(node)->getStringValue());
      }
      else {
        constants.push_back("");
        variables.push_back("");
      }
    }

    // it is possible that we have a variable which is constant
    // because of a restriction in the toArea
    const vector<Dimension*> * toDimensions = toCube->getDimensions();
    vector<string>::iterator j = constants.begin();

    for (vector<string>::iterator i = variables.begin();  i != variables.end();  i++, j++) {
      const string& name = *i;

      if (name.empty()) {
        continue;
      }

      Dimension * variableDimension = toCube->getDatabase()->findDimensionByName(name, 0);

      vector<Dimension*>::const_iterator d = toDimensions->begin();
      AreaNode::Area::const_iterator a = toArea->begin();

      for (;  d != toDimensions->end();  d++, a++) {
        if (*d == variableDimension) {
          break;
        }
      }

      if (d == toDimensions->end()) {
        throw ParameterException(ErrorException::ERROR_DIMENSION_NOT_FOUND,
                                 "cannot find variable dimension",
                                 "dimension", name);
      }

      const set<IdentifierType>& s = *a;

      if (! s.empty()) {
        Element* element = variableDimension->findElement(*s.begin(), 0);
        Logger::trace << "variable '" << name << "' in rule is constant because of destination '" 
                      << element->getName() << "'" << endl;

        *i = "";
        *j = element->getName();
      }
    }

    // convert FROM AREA description into a list of sets of base elements
    const vector<Dimension*> * fromDimensions = fromCube->getDimensions();
    vector<Dimension*>::const_iterator d = fromCube->getDimensions()->begin();
    set<IdentifierType> empty;

    for (vector<string>::iterator i = constants.begin();  i != constants.end();  i++, d++) {
      const string& name = *i;
      Dimension* dimension = *d;

      // s is either empty or contains one element, unfold to base elements
      if (name.empty()) {
        fromBase.push_back(empty);
      }
      else {
        set<Element*> e = dimension->getBaseElements(dimension->findElementByName(name, 0), 0);
        set<IdentifierType> base;

        for (set<Element*>::iterator j = e.begin();  j != e.end();  j++) {
          Element* element = *j;

          base.insert(element->getIdentifier());
        }

        fromBase.push_back(base);
      }
    }

    // convert variable names into dimensions
    vector<Dimension*> varDimensions;

    for (vector<string>::iterator i = variables.begin();  i != variables.end();  i++) {
      const string& name = *i;

      if (name.empty()) {
        varDimensions.push_back(0);
      }
      else {
        Dimension * vd = toCube->getDatabase()->findDimensionByName(name,0);
        varDimensions.push_back(vd);
      }
    }

    // convert TO AREA description into a list of identifiers
    size_t nd = toDimensions->size();

    permutations = new uint32_t[nd];
    fixed = new uint32_t[nd];

    size_t f = 0;

    vector<Dimension*>::const_iterator dd = toDimensions->begin();
    vector< pair<Dimension*, Dimension*> > dimPairs;

    for (AreaNode::Area::const_iterator i = toArea->begin();  i != toArea->end();  i++, f++, dd++) {
      const set<IdentifierType>& s = *i;

      if (! s.empty()) {
        fixed[f] = *s.begin();
        permutations[f] = NO_IDENTIFIER;
        dimPairs.push_back( pair<Dimension*,Dimension*>(0,0) );
      }
      else {

        // find matching variable dimension
        Dimension * td = *dd;
        vector<Dimension*>::iterator vIter = varDimensions.begin();
        uint32_t pos = 0;

        for (;  vIter != varDimensions.end();  vIter++, pos++) {
          if (*vIter == td) {
            break;
          }
        }

        if (vIter == varDimensions.end()) {
          throw ParameterException(ErrorException::ERROR_DIMENSION_NOT_FOUND,
                                   "cannot find variable dimension",
                                   "dimension", td->getName());
        }

        permutations[f] = pos;

        dimPairs.push_back( pair<Dimension*,Dimension*>(fromDimensions->at(pos), td) );
      }
    }

    // mapping needed
    useMapping = true;
    mapping.resize(nd);

    vector< vector<uint32_t> >::iterator m = mapping.begin();

    for (vector< pair<Dimension*, Dimension*> >::iterator i = dimPairs.begin();  i != dimPairs.end();  i++, m++) {
      pair<Dimension*, Dimension*>& p = *i;

      if (p.first == 0) {
        continue;
      }

      vector<uint32_t>& mm = *m;
      Dimension * fd = p.first;
      Dimension * td = p.second;
      vector<Element*> elements = fd->getElements(0);

      mm.resize(fd->getMaximalIdentifier() + 1, MarkerStorage::NO_MAPPING);

      for (vector<Element*>::iterator e = elements.begin();  e != elements.end();  e++) {
        Element * fe = *e;

        if (fe == 0) {
          continue;
        }

        Element * te = td->lookupElementByName(fe->getName());

        if (te == 0) {
          continue;
        }

        Logger::trace << "using mapping " << fe->getIdentifier() << " -> " << te->getIdentifier() 
                      << " for name '" << fe->getName() << "'" << endl;

        mm[fe->getIdentifier()] = te->getIdentifier();
      }
    }
  }



  RuleMarker::~RuleMarker () {
    delete[] permutations;
    delete[] fixed;
  }
}
