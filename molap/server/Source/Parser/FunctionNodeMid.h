////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node mid
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

#ifndef PARSER_FUNCTION_NODE_MID_H
#define PARSER_FUNCTION_NODE_MID_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node mid
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeMid : public FunctionNode {
      
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeMid(name, params);
      }

    public:
      FunctionNodeMid () : FunctionNode() {
      }

      FunctionNodeMid (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
      }

      Node * clone () {
        FunctionNodeMid * cloned = new FunctionNodeMid(name, cloneParameters());
        cloned->valid = this->valid;
        return cloned;
      }

    public:
      bool isDimensionTransformation (Cube* cube, Dimension** dimension) {
        if (valid) {
          return params->at(0)->isDimensionTransformation(cube, dimension) 
              && params->at(1)->isConstant(cube)
              && params->at(2)->isConstant(cube);
        }

        return false;
      }

      map<Element*, string> computeDimensionTransformations (Cube* cube) {
        map<Element*, string> elements = params->at(0)->computeDimensionTransformations(cube);
        int start = 0;
        int num = 1;

        if (params->at(1)->getNodeType() == Node::NODE_DOUBLE_NODE) {
          start = (int) dynamic_cast<DoubleNode*>(params->at(1))->getDoubleValue();
        }

        if (params->at(2)->getNodeType() == Node::NODE_DOUBLE_NODE) {
          num = (int) dynamic_cast<DoubleNode*>(params->at(2))->getDoubleValue();
        }

        for (map<Element*, string>::iterator i = elements.begin();
             i != elements.end();
             i++) {
          string value = i->second;
          int len = (int) value.size();

          if (start < len && start > 0 && num > 0) {
            i->second = value.substr(start - 1, num);
          }
          else {
            i->second = "";
          }
        }

        return elements;
      }

      ValueType getValueType () {
        return Node::STRING;
      }      
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
        if (!params || params->size() < 3) {
          error = "function '" + name + "' needs three parameters";
          return valid = false;
        }
        if (params->size() > 3) {
          error = "too many parameters for function '" + name + "'";
          return valid = false;
        }    
    
        for (int i = 0; i < 3; i++) {
          Node* param = params->at(i);
          // validate parameter
          if (!param->validate(server, database, cube, destination, error)) {
            return valid = false;         
          }
          
          // validate data type
          if (i == 0) {
            if (param->getValueType() != Node::STRING && 
                param->getValueType() != Node::UNKNOWN) {
              error = "a parameter of function '" + name + "' has a wrong data type";        
              return valid = false;
            }
          }
          else {
            if (param->getValueType() != Node::NUMERIC && 
                param->getValueType() != Node::UNKNOWN) {
              error = "a parameter of function '" + name + "' has a wrong data type";        
              return valid = false;
            }
          }
        }
        
        return valid = true;  
      }

      RuleValueType getValue (CellPath* cellPath, 
                              User* user,
                              bool* isCachable, 
                              set< pair<Rule*, IdentifiersType> >* ruleHistory) {        
        if (valid) {
          RuleValueType text  = params->at(0)->getValue(cellPath,user,isCachable, ruleHistory);
          RuleValueType start = params->at(1)->getValue(cellPath,user,isCachable, ruleHistory);
          RuleValueType num   = params->at(2)->getValue(cellPath,user,isCachable, ruleHistory);
          
          if (text.type == Node::STRING
           && text.stringValue.length() > 0 
           && start.type == Node::NUMERIC 
           && num.type == Node::NUMERIC) {

            int len = (int) text.stringValue.length();
            int s   = (int) start.doubleValue;
            int n   = (int) num.doubleValue; 

            if (s < len && s > 0 && n > 0) {
              RuleValueType result;
              result.type = Node::STRING; 
              result.stringValue = text.stringValue.substr(s - 1, n);
              return result;
            }
          }
        }
        
        RuleValueType result;
        result.type = Node::STRING; 
        result.stringValue = "";

        return result;
      }
 
      
  };

}
#endif

