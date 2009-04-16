////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node palo elevel
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

#ifndef PARSER_FUNCTION_NODE_PALO_ELEVEL_H
#define PARSER_FUNCTION_NODE_PALO_ELEVEL_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNodePalo.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node palo elevel
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodePaloElevel : public FunctionNodePalo {

    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodePaloElevel(name, params);
      }
      
    public:
      FunctionNodePaloElevel () : FunctionNodePalo() {
      }

      FunctionNodePaloElevel (const string& name,  vector<Node*> *params) : FunctionNodePalo(name, params) {
      }

      Node * clone () {
        FunctionNodePaloElevel * cloned = new FunctionNodePaloElevel(name, cloneParameters());
        cloned->valid    = this->valid;
        cloned->server   = this->server;
        cloned->database = this->database;
        return cloned;
      }

    public:
      ValueType getValueType () {
        return Node::NUMERIC;
      }          
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
        return validateParameter(server, database, cube, destination, error, 3, 0);
      }
    
      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        RuleValueType result;
        result.type = Node::NUMERIC; 
        
        if (valid) {
          Database* database   = getDatabase(server, params->at(0), cellPath, user,isCachable, ruleHistory);
          Dimension* dimension = getDimension(database, params->at(1), cellPath, user,isCachable, ruleHistory);   
          Element* element     = getElement(dimension, params->at(2), cellPath, user,isCachable, ruleHistory);   

          if (element) {
            result.doubleValue = (double) element->getLevel(dimension);
            return result;
          }
        }
        
        result.doubleValue = 0.0;
        return result;
      }
      
  };

}

#endif
