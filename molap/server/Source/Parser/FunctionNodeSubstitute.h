////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node substitute
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

#ifndef PARSER_FUNCTION_NODE_SUBSTITUTE_H
#define PARSER_FUNCTION_NODE_SUBSTITUTE_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node substitute
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeSubstitute : public FunctionNode {
      
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeSubstitute(name, params);
      }

    public:
      FunctionNodeSubstitute () : FunctionNode() {
      }

      FunctionNodeSubstitute (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
      }

      Node * clone () {
        FunctionNodeSubstitute * cloned = new FunctionNodeSubstitute(name, cloneParameters());
        cloned->valid = this->valid;
        return cloned;
      }

    public:
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
          if (param->getValueType() != Node::STRING && 
              param->getValueType() != Node::UNKNOWN) {
            error = "a parameter of function '" + name + "' has a wrong data type";        
            return valid = false;
          }
        }
        
        return valid = true;  
      }

    
      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {        
        RuleValueType result;
        result.type = Node::STRING; 

        if (valid) {
          RuleValueType text = params->at(0)->getValue(cellPath,user,isCachable, ruleHistory);
          RuleValueType oldt = params->at(1)->getValue(cellPath,user,isCachable, ruleHistory);
          RuleValueType newt = params->at(2)->getValue(cellPath,user,isCachable, ruleHistory);
          
          if (text.type == Node::STRING && oldt.type == Node::STRING && newt.type == Node::STRING) {
            string r = text.stringValue;

            if (! oldt.stringValue.empty()) {
              size_t pos = 0;

              while ((pos = r.find(oldt.stringValue, pos)) != string::npos) {
                r = r.substr(0, pos) + newt.stringValue + r.substr(pos + oldt.stringValue.size());
                pos += newt.stringValue.size();
              }
            }

            result.stringValue = r;

            return result;
          }
        }
        
        result.stringValue = "";
        return result;
      }
 
      
  };

}
#endif

