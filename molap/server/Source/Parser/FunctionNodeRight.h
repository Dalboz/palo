////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node right
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

#ifndef PARSER_FUNCTION_NODE_RIGHT_H
#define PARSER_FUNCTION_NODE_RIGHT_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node right
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeRight : public FunctionNode {
      
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeRight(name, params);
      }

    public:
      FunctionNodeRight () : FunctionNode() {
      }

      FunctionNodeRight (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
      }

      Node * clone () {
        FunctionNodeRight * cloned = new FunctionNodeRight(name, cloneParameters());
        cloned->valid = this->valid;
        return cloned;
      }

    public:
      bool isDimensionTransformation (Cube* cube, Dimension** dimension) {
        if (valid) {
          return left->isDimensionTransformation(cube, dimension) && (right == 0 || right->isConstant(cube));
        }

        return false;
      }

      map<Element*, string> computeDimensionTransformations (Cube* cube) {
        map<Element*, string> elements = left->computeDimensionTransformations(cube);
        size_t pos = 1;

        if (right) {
          if (right->getNodeType() != Node::NODE_DOUBLE_NODE) {
            for (map<Element*, string>::iterator i = elements.begin();
                 i != elements.end();
                 i++) {
              i->second = "";
            }

            return elements;
          }

          pos = (unsigned int) dynamic_cast<DoubleNode*>(right)->getDoubleValue();
        }

        for (map<Element*, string>::iterator i = elements.begin();
             i != elements.end();
             i++) {
          string value = i->second;
          size_t len = value.size();

          if (pos < len) {
            i->second = value.substr(len - pos, pos);;
          }
        }

        return elements;
      }

      ValueType getValueType () {
        return Node::STRING;
      }      
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
    
        // has one or two parameters
        if (!params || params->size() < 1) {
          error = "function '" + name + "' needs one or two parameters";
          return valid = false;
        }
        if (params->size() > 2) {
          error = "too many parameters for function '" + name + "'";
          return valid = false;
        }    
    
        left  = params->at(0);
        
        // validate parameters
        if (!left->validate(server, database, cube, destination, error)) {
          return valid = false;         
        }
          
        // check data type left
        if (left->getValueType() != Node::STRING && 
            left->getValueType() != Node::UNKNOWN) {
          error = "first parameter of function '" + name + "' has wrong data type";        
          return valid = false;
        }
        
        if (params->size() == 1) {
          right = 0;
        }
        else {          
          right  = params->at(1);
          
          if (!right->validate(server, database, cube, destination, error)) {
            return valid = false;         
          }

          // check data type right
          if (right->getValueType() != Node::NUMERIC && 
              right->getValueType() != Node::UNKNOWN ) {
            error = "second parameter of function '" + name + "' has wrong data type";        
            return valid = false;
          }
        }
        
        return valid = true;  
      }
    
      RuleValueType getValue (CellPath* cellPath,
                              User* user,
                              bool* isCachable, 
                              set< pair<Rule*, IdentifiersType> >* ruleHistory) {        
        if (valid) {
          RuleValueType l  = left->getValue(cellPath,user,isCachable, ruleHistory);
          
          if (l.type == Node::STRING && l.stringValue.size() > 0) {
            unsigned int len = (unsigned int) l.stringValue.size();
            
            if (right) {           
              RuleValueType r  = right->getValue(cellPath,user,isCachable, ruleHistory);

              if (r.type == Node::NUMERIC) {
                unsigned int num = (unsigned int) r.doubleValue;
                
                if (num >= len) {
                  return l;
                }
                              
                RuleValueType result;
                result.type = Node::STRING; 
                result.stringValue = l.stringValue.substr(len-num, num);

                return result;
              }
            }
            else {
              // default = 1
              RuleValueType result;
              result.type = Node::STRING; 
              result.stringValue = l.stringValue.substr(len-1, 1);
              return result;
            }                  
          }
        }
        
        RuleValueType result;
        result.type = Node::STRING; 
        result.stringValue = "";
        return result;
      }
      
    protected:
      Node* left;
      Node* right;  
      
  };

}
#endif

