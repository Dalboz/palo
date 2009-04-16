////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node trim
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

#ifndef PARSER_FUNCTION_NODE_TRIM_H
#define PARSER_FUNCTION_NODE_TRIM_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node trim
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeTrim : public FunctionNode {
      
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeTrim(name, params);
      }

    public:
      FunctionNodeTrim () : FunctionNode() {
      }

      FunctionNodeTrim (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
      }

      Node * clone () {
        FunctionNodeTrim * cloned = new FunctionNodeTrim(name, cloneParameters());
        cloned->valid = this->valid;
        return cloned;
      }

    public:
      ValueType getValueType () {
        return Node::STRING;
      }      
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
    
        // has one parameters
        if (!params || params->size() != 1) {
          error = "function '" + name + "' needs one parameter";
          return valid = false;
        }
    
        Node* param = params->at(0);
        // validate parameter
        if (!param->validate(server, database, cube, destination, error)) {
          return valid = false;         
        }
        // check data type left
        if (param->getValueType() != Node::STRING && 
            param->getValueType() != Node::UNKNOWN) {
          error = "parameter of function '" + name + "' has wrong data type";        
          return valid = false;
        }
            
        return valid = true;  
      }

    
      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        RuleValueType result;
        result.type = Node::STRING; 
        
        if (valid) {
          RuleValueType value = params->at(0)->getValue(cellPath,user,isCachable, ruleHistory);
          
          if (value.type == Node::STRING) {            
            size_t s = value.stringValue.find_first_not_of(" \t\n\r");
            size_t e = value.stringValue.find_last_not_of(" \t\n\r");      
            if (s != std::string::npos) {
              result.stringValue = string(value.stringValue, s, e-s+1);
              return result;
            }
          }          
        }
        
        result.stringValue = "";
        return result;
      }
  };

}

#endif
