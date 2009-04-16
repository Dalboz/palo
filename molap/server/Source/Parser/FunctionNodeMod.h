////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node mod
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

#ifndef PARSER_FUNCTION_NODE_MOD_H
#define PARSER_FUNCTION_NODE_MOD_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNodeSimple.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node mod
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeMod : public FunctionNodeSimple {
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeMod(name, params);
      }

    public:
      FunctionNodeMod () : FunctionNodeSimple() {
      }

      FunctionNodeMod (const string& name,  vector<Node*> *params) : FunctionNodeSimple(name, params) {
      }

      Node * clone () {
        FunctionNodeMod * cloned = new FunctionNodeMod(name, cloneParameters());
        cloned->valid = this->valid;
        cloned->infix = this->infix;
        cloned->left  = this->valid ? (*(cloned->params))[0] : 0;
        cloned->right = this->valid ? (*(cloned->params))[1] : 0;
        return cloned;
      }

    public:
      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        RuleValueType result;

        result.type = Node::NUMERIC; 
        result.doubleValue = 0.0;

        if (valid) {
          RuleValueType l = left->getValue(cellPath,user,isCachable, ruleHistory);
          RuleValueType r = right->getValue(cellPath,user,isCachable, ruleHistory);          

          if (l.type == Node::NUMERIC && r.type == Node::NUMERIC && r.doubleValue != 0.0) {
            result.doubleValue = l.doubleValue - trunc(l.doubleValue / r.doubleValue) * r.doubleValue;
          }  
        }

        return result;
      }  
      
  };

}

#endif

