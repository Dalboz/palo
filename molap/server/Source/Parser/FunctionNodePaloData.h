////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node palo data
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

#ifndef PARSER_FUNCTION_NODE_PALO_DATA_H
#define PARSER_FUNCTION_NODE_PALO_DATA_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNodePalo.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node palo data
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodePaloData : public FunctionNodePalo {

    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodePaloData(name, params);
      }
      
    public:
      FunctionNodePaloData () : FunctionNodePalo() {
      }

      FunctionNodePaloData (const string& name,  vector<Node*> *params) : FunctionNodePalo(name, params) {
      }    

      Node * clone () {
        FunctionNodePaloData * cloned = new FunctionNodePaloData(name, cloneParameters());
        cloned->valid    = this->valid;
        cloned->server   = this->server;
        cloned->database = this->database;
        return cloned;
      }

    public:
      ValueType getValueType () {
        return Node::UNKNOWN;
      }  
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
    
        // has three parameters
        if (!params || params->size() < 3) {
          error = "function '" + name + "' needs more than two parameters";
          return valid = false;
        }
    
        for (size_t i = 0; i < params->size(); i++) {
          Node* param = params->at(i);

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
        }

        this->server = server;

        return valid = true;  
      }

    
      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        RuleValueType result;
        
        if (valid) {
          Database* database = getDatabase(server, params->at(0), cellPath, user, isCachable, ruleHistory);

          if (database == 0) {
            throw ParameterException(ErrorException::ERROR_DATABASE_NOT_FOUND,
                                     "database unknown",
                                     "database", params->at(0)->getValue(cellPath, user, isCachable, ruleHistory).stringValue);
          }

          Cube* cube = getCube(database, params->at(1), cellPath, user, isCachable, ruleHistory);   

          if (cube == 0) {
            throw ParameterException(ErrorException::ERROR_CUBE_NOT_FOUND,
                                     "cube unknown",
                                     "database", params->at(1)->getValue(cellPath, user, isCachable, ruleHistory).stringValue);
          }

          const vector<Dimension*>* dimensions = cube->getDimensions();
            
          if (dimensions->size() != params->size()-2) {
            throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                     "wrong number of path elements",
                                     "number of coordinates", (int) (params->size() - 2));
          } 
          
          PathType path;
          
          vector<Dimension*>::const_iterator dim = dimensions->begin();
          vector<Node*>::iterator node = params->begin()+2;
          
          for (;dim != dimensions->end(); dim++, node++) {
            Element* element = getElement(*dim, *node, cellPath, user, isCachable, ruleHistory);
            
            if (!element) {
              RuleValueType value = (*node)->getValue(cellPath, user, isCachable, ruleHistory);
              string name = "decimal value";
              
              if (value.type == Node::STRING) {
                name = value.stringValue;
              }
              
              throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_FOUND,
                                       "element of dimension not found",
                                       "element", name);
            }
            
            path.push_back(element);
          }
          
          CellPath cp(cube, &path);            
          bool found;                        
          Cube::CellValueType value = cube->getCellValue(&cp, &found, user, 0, ruleHistory);
          
          if (found) {
            if (value.type == Element::STRING) {
              result.type = Node::STRING;
              result.stringValue = value.charValue;
            } 
            else {
              result.type = Node::NUMERIC;
              result.doubleValue = value.doubleValue;
            }
          }
          else {
            if (cp.getPathType() == Element::STRING) {
              result.type = Node::STRING;
              result.stringValue = "";
            }
            else {
              result.type = Node::NUMERIC;
              result.doubleValue = 0.0;
            }
          }

          return result;
        }
        
        result.type = Node::NUMERIC; 
        result.doubleValue = 0.0;

        return result;
      }
      
  };

}
#endif

