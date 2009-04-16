////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node ge (>=)
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

#ifndef PARSER_FUNCTION_NODE_GE_H
#define PARSER_FUNCTION_NODE_GE_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNodeComparison.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node ge (>=)
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeGe : public FunctionNodeComparison {
      
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeGe(name, params, false);
      }

      static FunctionNode* createNodeInfix (const string& name, vector<Node*> *params) {
        return new FunctionNodeGe(name, params, true);
      }

    public:
      FunctionNodeGe () : FunctionNodeComparison() {
      }

      FunctionNodeGe (const string& name,  vector<Node*> *params, bool infix) : FunctionNodeComparison(name, params, infix) {
      }

      Node * clone () {
        FunctionNodeGe * cloned = new FunctionNodeGe(name, cloneParameters(), this->infix);
        cloned->valid = this->valid;
        cloned->left  = this->valid ? (*(cloned->params))[0] : 0;
        cloned->right = this->valid ? (*(cloned->params))[1] : 0;
        return cloned;
      }

    public:
      bool isDimensionRestriction (Cube* cube, Dimension** dimension) {
        if (valid) {
          return (left->isDimensionTransformation(cube, dimension) && right->isConstant(cube))
              || (left->isConstant(cube) && right->isDimensionTransformation(cube, dimension));
        }

        return false;
      }

      set<Element*> computeDimensionRestriction (Cube* cube) {
        Dimension* dimension;
        map<Element*, string> elements;
        set<Element*> result;
        string constant;
        bool isLeft;

        if (left->isDimensionTransformation(cube, &dimension) && right->isConstant(cube)) {
          Logger::debug << "computing restrictions on dimension " << dimension->getName() << endl;

          elements = left->computeDimensionTransformations(cube);

          if (right->getNodeType() == Node::NODE_STRING_NODE) {
            constant = dynamic_cast<StringNode*>(right)->getStringValue();
            isLeft = false;
          }
          else {
            return result;
          }
        }
        else if (right->isDimensionTransformation(cube, &dimension) && left->isConstant(cube)) {
          Logger::debug << "computing restrictions on dimension " << dimension->getName() << endl;

          elements = right->computeDimensionTransformations(cube);

          if (left->getNodeType() == Node::NODE_STRING_NODE) {
            constant = dynamic_cast<StringNode*>(left)->getStringValue();
            isLeft = true;
          }
          else {
            return result;
          }
        }
        else {
          return result;
        }

        Logger::debug << "restriction '>=' for constant " << constant << endl;

        for (map<Element*, string>::iterator iter = elements.begin();  iter != elements.end();  iter++) {
          Element* element = iter->first;
          const string& value = iter->second;

          if ((isLeft && constant >= value) || (! isLeft && value >= constant)) {
            Logger::debug << "element " << element->getName() << " maps to " << value << endl;
            result.insert(element);
          }
        }

        return result;
      }

      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        RuleValueType result;
        result.type = Node::NUMERIC; 

        if (valid) {
          RuleValueType l = left->getValue(cellPath,user,isCachable, ruleHistory);
          RuleValueType r = right->getValue(cellPath,user,isCachable, ruleHistory);          
          if (l.type == Node::NUMERIC) {        
            if (r.type == Node::NUMERIC) {
              result.doubleValue = (l.doubleValue >= r.doubleValue)  ? 1.0 : 0.0;
            }
            // 10 >= "egal"       => false
            else {
              result.doubleValue = 0.0;
            }                      
          }
          else if (l.type == Node::STRING) {
            if (r.type == Node::STRING) {
              result.doubleValue = (l.stringValue >= r.stringValue) ? 1.0 : 0.0;
            }
            // "egal"  >= 100          => true
            else {
              result.doubleValue = 1.0;
            }                      
          }
          return result;          
        }
        
        // not valid or unknown type
        result.doubleValue = 0.0;
        return result;
      }
  };

}
#endif

