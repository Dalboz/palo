////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node add
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

#ifndef PARSER_FUNCTION_NODE_ADD_H
#define PARSER_FUNCTION_NODE_ADD_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNodeSimple.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node add
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeAdd : public FunctionNodeSimple {
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeAdd(name, params, false);
      }

      static FunctionNode* createNodeInfix (const string& name, vector<Node*> *params) {
        return new FunctionNodeAdd(name, params, true);
      }

    public:
      FunctionNodeAdd () : FunctionNodeSimple() {
      }

      FunctionNodeAdd (const string& name,  vector<Node*> *params, bool infix) : FunctionNodeSimple(name, params, infix) {
      }

      Node * clone () {
        FunctionNodeAdd * cloned = new FunctionNodeAdd(name, cloneParameters(), this->infix);
        cloned->valid = this->valid;
        cloned->left  = this->valid ? (*(cloned->params))[0] : 0;
        cloned->right = this->valid ? (*(cloned->params))[1] : 0;
        return cloned;
      }

    public:
      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        RuleValueType result;
        result.type = Node::NUMERIC; 

        if (valid) {
          RuleValueType l = left->getValue(cellPath,user,isCachable, ruleHistory);
          RuleValueType r = right->getValue(cellPath,user,isCachable, ruleHistory);          

          if (l.type == Node::NUMERIC && r.type == Node::NUMERIC) {
            result.doubleValue = l.doubleValue + r.doubleValue;
            return result;
          }  
        }

        result.doubleValue = 0.0;

        return result;
      }
  };

}
#endif

