////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node datevalue
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

#ifndef PARSER_FUNCTION_NODE_DATEVALUE_H
#define PARSER_FUNCTION_NODE_DATEVALUE_H 1

#include "palo.h"

#include "math.h"
#include "Collections/StringUtils.h"

#include <string>
#include "Parser/FunctionNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node datevalue
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeDatevalue : public FunctionNode {
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeDatevalue(name, params);
      }

    public:
      FunctionNodeDatevalue () : FunctionNode() {
      }

      FunctionNodeDatevalue (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
      }

      Node * clone () {
        FunctionNodeDatevalue * cloned = new FunctionNodeDatevalue(name, cloneParameters());
        cloned->valid = this->valid;
        return cloned;
      }

    public:
      ValueType getValueType () {
        return Node::NUMERIC;
      }      
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
        if (!params || params->size() != 1) {
          error = "function '" + name + "' needs one parameter";
          valid = false;
          return valid;
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
        result.type = Node::NUMERIC; 
        
        if (valid) {
          RuleValueType date = params->at(0)->getValue(cellPath,user,isCachable, ruleHistory);
          
          if (date.type == Node::STRING && date.stringValue.length() == 8) {
            try {
              int m =  StringUtils::stringToInteger(date.stringValue.substr(0,2));
              int d =  StringUtils::stringToInteger(date.stringValue.substr(3,2));
              int y =  StringUtils::stringToInteger(date.stringValue.substr(6,2));
              
              struct tm t;
              t.tm_sec   = 0;
              t.tm_min   = 0;
              t.tm_hour  = 12;
              t.tm_wday  = 0;
              t.tm_yday  = 0;
              t.tm_isdst = -1;      
              t.tm_year  = y + 100; 
              t.tm_mon   = m - 1; 
              t.tm_mday  = d;
        
              result.doubleValue = (double) (mktime(&t) / 86400 * 86400);
            }
            catch (ParameterException e) {
              result.doubleValue = 0.0;
            }
            
            return result;
          }          
        }
        
        result.doubleValue = 0.0;
        return result;
      }
  };

}
#endif

