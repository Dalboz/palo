////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node upper
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

#ifndef PARSER_FUNCTION_NODE_UPPER_H
#define PARSER_FUNCTION_NODE_UPPER_H 1

#include "palo.h"

#include "Collections/StringUtils.h"

#include <string>
#include "Parser/FunctionNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node upper
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeUpper : public FunctionNode {
      
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeUpper(name, params);
      }

    public:
      FunctionNodeUpper () : FunctionNode() {
      }

      FunctionNodeUpper (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
      }

      Node * clone () {
        FunctionNodeUpper * cloned = new FunctionNodeUpper(name, cloneParameters());
        cloned->valid = this->valid;
        return cloned;
      }

    public:
      bool isDimensionTransformation (Cube* cube, Dimension** dimension) {
        if (valid) {
          return params->at(0)->isDimensionTransformation(cube, dimension);
        }

        return false;
      }

      map<Element*, string> computeDimensionTransformations (Cube* cube) {
        map<Element*, string> elements = params->at(0)->computeDimensionTransformations(cube);

        for (map<Element*, string>::iterator i = elements.begin();
             i != elements.end();
             i++) {
          i->second = StringUtils::toupper(i->second);
        }

        return elements;
      }

      ValueType getValueType () {
        return Node::STRING;
      }      
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
    
        // upper has one params
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

    
      RuleValueType getValue (CellPath* cellPath,
                              User* user, 
                              bool* isCachable, 
                              set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        RuleValueType result;
        result.type = Node::STRING; 
        
        if (valid) {
          RuleValueType value = params->at(0)->getValue(cellPath,user,isCachable, ruleHistory);
          
          if (value.type == Node::STRING) {            
            result.stringValue = StringUtils::toupper(value.stringValue);
            return result;
          }          
        }
        
        result.stringValue = "";
        return result;
      }
  };

}
#endif

