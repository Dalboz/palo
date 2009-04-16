////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node palo
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

#ifndef PARSER_FUNCTION_NODE_PALO_H
#define PARSER_FUNCTION_NODE_PALO_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNode.h"

#include "Olap/CellPath.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node palo
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodePalo : public FunctionNode {

    public:
      FunctionNodePalo () : FunctionNode() {
        server = 0;
        database = 0;
      }

      FunctionNodePalo (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
        server = 0;
        database = 0;
      }    

    public:
      bool validateParameter (Server* server, Database* database, Cube* cube, Node* destination, string& error, int numStrings, int numNumeric) {
        if (!params || params->size() < (size_t) (numStrings + numNumeric)) {
          error = "function '" + name + "' needs more parameters";
          return valid = false;
        }
        else if (params->size() > (size_t) (numStrings + numNumeric)) {
          error = "too many parameters in function '" + name + "'";
          return valid = false;
        }
    
        for (size_t i = 0; i < params->size(); i++) {
          Node* param = params->at(i);

          // validate parameter
          if (!param->validate(server, database, cube, destination, error)) {
            return valid = false;         
          }          

          // check data type
          if (i >= (size_t) numStrings) {
            if (param->getValueType() != Node::NUMERIC && 
                param->getValueType() != Node::UNKNOWN) {
              error = "parameter of function '" + name + "' has wrong data type";        
              return valid = false;
            }
          }
          else {
            if (param->getValueType() != Node::STRING && 
                param->getValueType() != Node::UNKNOWN) {
              error = "parameter of function '" + name + "' has wrong data type";        
              return valid = false;
            }
          }
        }

        this->server = server;
        this->database = database;

        return valid = true;  
      }
    

      Database* getDatabase (Server* server, Node* stringNode, CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        if (server) {
          // we assume that the string node is valid
          RuleValueType value = stringNode->getValue(cellPath, user, isCachable, ruleHistory);
          if (value.type == Node::STRING) {
            Database* d = server->lookupDatabaseByName(value.stringValue);
            
            // check for other database
            if (d && database && database != d) {
              *isCachable = false;
            }
            
            return d; 
          }
        }
        return 0;
      }
            


      Cube* getCube (Database* database, Node* stringNode, CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        if (database) {
          // we assume that the string node is valid
          RuleValueType value = stringNode->getValue(cellPath,user, isCachable, ruleHistory);
          if (value.type == Node::STRING) {
            return database->findCubeByName(value.stringValue, user);
          }
        }
        return 0;
      }
            


      Dimension* getDimension (Database* database, Node* stringNode, CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        if (database) {
          // we assume that the string node is valid
          RuleValueType value = stringNode->getValue(cellPath,user, isCachable, ruleHistory);
          if (value.type == Node::STRING) {
            return database->findDimensionByName(value.stringValue, user);
          }
        }
        return 0;
      }
            


      Element* getElement (Dimension * dimension, Node* stringNode, CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        if (dimension) {
          // we assume that the string node is valid
          RuleValueType value = stringNode->getValue(cellPath,user, isCachable, ruleHistory);
          if (value.type == Node::STRING) {
            return dimension->findElementByName(value.stringValue, user);
          }
        }
        return 0;
      }
            
    protected:
      Server* server;
      Database* database;
              
  };

}
#endif

