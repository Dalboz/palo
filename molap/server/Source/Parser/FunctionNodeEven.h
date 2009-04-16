////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node even
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

#ifndef PARSER_FUNCTION_NODE_EVEN_H
#define PARSER_FUNCTION_NODE_EVEN_H 1

#include "palo.h"

#include <math.h>

#include <string>

#include "Parser/FunctionNodeSimpleSingle.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node even
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeEven : public FunctionNodeSimpleSingle {
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeEven(name, params);
      }

    public:
      FunctionNodeEven () : FunctionNodeSimpleSingle() {
      }

      FunctionNodeEven (const string& name,  vector<Node*> *params) : FunctionNodeSimpleSingle(name, params) {
      }

      Node * clone () {
        FunctionNodeEven * cloned = new FunctionNodeEven(name, cloneParameters());
        cloned->valid = this->valid;
        cloned->node  = this->valid ? (*(cloned->params))[0] : 0;
        return cloned;
      }

    public:
    
      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        RuleValueType result;
        
        if (valid) {
          result = node->getValue(cellPath,user,isCachable, ruleHistory);

          if (result.type == Node::NUMERIC) {
            result.doubleValue = round(result.doubleValue / 2) * 2;

            return result;
          }          
        }
        
        result.type = Node::NUMERIC; 
        result.doubleValue = 0.0;
        return result;
      }
  };

}

#endif

