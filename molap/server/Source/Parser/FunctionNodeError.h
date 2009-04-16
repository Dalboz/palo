////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node error
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

#ifndef PARSER_FUNCTION_NODE_ERROR_H
#define PARSER_FUNCTION_NODE_ERROR_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNode.h"

#include "Olap/Server.h"
#include "Olap/Database.h"
#include "Olap/Cube.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node error
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeError : public FunctionNode {
      
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeError(name, params);
      }

    public:
      FunctionNodeError () : FunctionNode() {
      }

      FunctionNodeError (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
      }

      Node * clone () {
        FunctionNodeError * cloned = new FunctionNodeError(name, cloneParameters());
        cloned->valid = this->valid;
        return cloned;
      }

    public:
      ValueType getValueType () {
        return Node::UNKNOWN;
      }
            
      bool validate (Server*, Database*, Cube*, Node*, string& message) {
        message = "unknown function '" + name + "'"; 
        return valid = false;
      };

      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        RuleValueType result;
        result.type = Node::UNKNOWN; 
        return result;
      }
  
  };

}
#endif

