////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node palo esibling
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

#ifndef PARSER_FUNCTION_NODE_PALO_ESIBLING_H
#define PARSER_FUNCTION_NODE_PALO_ESIBLING_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNodePalo.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node palo esibling
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodePaloEsibling : public FunctionNodePalo {

    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodePaloEsibling(name, params);
      }
      
    public:
      FunctionNodePaloEsibling () : FunctionNodePalo() {
      }

      FunctionNodePaloEsibling (const string& name,  vector<Node*> *params) : FunctionNodePalo(name, params) {
      }

      Node * clone () {
        FunctionNodePaloEsibling * cloned = new FunctionNodePaloEsibling(name, cloneParameters());
        cloned->valid    = this->valid;
        cloned->server   = this->server;
        cloned->database = this->database;
        return cloned;
      }

    public:
      ValueType getValueType () {
        return Node::STRING;
      }          
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
        return validateParameter(server, database, cube, destination, error, 3, 1);
      }
    
      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        RuleValueType result;
        result.type = Node::STRING; 
        
        if (valid) {
          Database* database   = getDatabase(server, params->at(0), cellPath, user, isCachable, ruleHistory);
          Dimension* dimension = getDimension(database, params->at(1), cellPath, user, isCachable, ruleHistory);   
          Element* element     = getElement(dimension, params->at(2), cellPath, user, isCachable, ruleHistory);
          RuleValueType num    = params->at(3)->getValue(cellPath, user, isCachable, ruleHistory);

          if (element && num.type == Node::NUMERIC) {
            int32_t offset = (int32_t) num.doubleValue;

            // return element name for num == 0
            if (offset == 0) {
              result.stringValue = element->getName();
              return result;
            }

            // get parents
            const Dimension::ParentsType* parentsList = dimension->getParents(element);
            if (parentsList->size() > 0) {

              // get first parent
              Element* parent = (*parentsList)[0];
              const ElementsWeightType * childrenList = dimension->getChildren(parent);
        
              ssize_t childPos = 0;
        
              for (ElementsWeightType::const_iterator iter = childrenList->begin();  iter != childrenList->end();  childPos++, iter++) {
                if ((*iter).first == element) {
                  break;
                }
              }
        
              if (childPos >= (ssize_t) childrenList->size()) {
                result.stringValue = "";
                return result;
              }
        
              // in case of negative offset, try to find element left of the given element
              if (offset < 0) {
                if (childPos + offset < 0) {
                  result.stringValue = "";
                  return result;
                }
                else {
                  Element * resultElement = (*childrenList)[childPos + offset].first;
                  result.stringValue = resultElement->getName();
                  return result;
                }
              }
        
              // in case of positive offset, try to find element right of the given element
              if (childPos + offset < (ssize_t) childrenList->size()) {
                Element * resultElement = (*childrenList)[childPos + offset].first;
                result.stringValue = resultElement->getName();
                return result;
              }
        
              offset -= (ssize_t)(childrenList->size() - childPos);
        
              // try other parents
              for (Dimension::ParentsType::const_iterator iter = parentsList->begin() + 1;  iter != parentsList->end();  iter++) {
                childrenList = dimension->getChildren(*iter);
        
                for (ElementsWeightType::const_iterator c = childrenList->begin();  c != childrenList->end();  c++) {
                  if ((*c).first != element) {
                    if (offset == 0) {
                      result.stringValue = (*c).first->getName();
                      return result;
                      //return generateElementResult((*c).first);
                    }
        
                    offset--;
                  }
                }
              }
            }
          }
        }
        
        result.stringValue = "";
        return result;
      }
      
  };

}
#endif

