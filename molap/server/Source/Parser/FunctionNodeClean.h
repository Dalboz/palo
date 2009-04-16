////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node clean
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

#ifndef PARSER_FUNCTION_NODE_CLEAN_H
#define PARSER_FUNCTION_NODE_CLEAN_H 1

#include "palo.h"

#include "Collections/StringUtils.h"

#include <string>
#include "Parser/FunctionNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node clean
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeClean : public FunctionNode {
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeClean(name, params);
      }

    public:
      FunctionNodeClean () : FunctionNode() {
      }

      FunctionNodeClean (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
      }

      Node * clone () {
        FunctionNodeClean * cloned = new FunctionNodeClean(name, cloneParameters());
        cloned->valid = this->valid;
        return cloned;
      }

    public:
      ValueType getValueType () {
        return Node::STRING;
      }      
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
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
            const string& str = value.stringValue;
            const char * ptr = str.c_str();
            size_t len = str.size();
            char * buffer = new char[len];
            char * qtr;
            
            for (qtr = buffer; 0 < len; len--, ptr++) {
              if (' ' <= *ptr) {
                *qtr++ = *ptr;
              }
            }

            *qtr = '\0';

            value.stringValue = buffer;

            delete[] buffer;

            return value;
          }          
        }
        
        result.stringValue = "";

        return result;
      }
  };

}

#endif

