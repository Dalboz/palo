////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node concatenate
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

#ifndef PARSER_FUNCTION_NODE_CONCATENATE_H
#define PARSER_FUNCTION_NODE_CONCATENATE_H 1

#include "palo.h"

#include <string>

#include "Parser/FunctionNode.h"
#include "Parser/StringNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node concatenate
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeConcatenate : public FunctionNode {
      
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeConcatenate(name, params);
      }

    public:
      FunctionNodeConcatenate () : FunctionNode() {
      }

      FunctionNodeConcatenate (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
      }    

      Node * clone () {
        FunctionNodeConcatenate * cloned = new FunctionNodeConcatenate(name, cloneParameters());
        cloned->valid = this->valid;
        cloned->left  = this->left;
        cloned->right = this->right;
        return cloned;
      }

    public:
      bool isDimensionTransformation (Cube* cube, Dimension** dimension) {
        if (valid) {
          if (left->isDimensionTransformation(cube, dimension)) {
            Dimension * dimension2;

            if (right->isDimensionTransformation(cube, &dimension2)) {
              return *dimension == dimension2;
            }
            else if (right->isConstant(cube)) {
              return true;
            }
          }
          else if (right->isDimensionTransformation(cube, dimension)) {
            if (left->isConstant(cube)) {
              return true;
            }
          }
        }

        return false;
      }

      map<Element*, string> computeDimensionTransformations (Cube* cube) {
        Dimension* dimension;
        map<Element*, string> elements;

        if (left->isDimensionTransformation(cube, &dimension) && right->isConstant(cube)) {
          elements = left->computeDimensionTransformations(cube);
          string cc = "";

          if (right->getNodeType() == Node::NODE_STRING_NODE) {
            cc = dynamic_cast<StringNode*>(right)->getStringValue();
          }

          for (map<Element*, string>::iterator i = elements.begin();
               i != elements.end();
               i++) {
            i->second = i->second + cc;
          }

          return elements;
        }
        else if (right->isDimensionTransformation(cube, &dimension) && left->isConstant(cube)) {
          elements = right->computeDimensionTransformations(cube);
          string cc = "";

          if (left->getNodeType() == Node::NODE_STRING_NODE) {
            cc = dynamic_cast<StringNode*>(left)->getStringValue();
          }

          for (map<Element*, string>::iterator i = elements.begin();
               i != elements.end();
               i++) {
            i->second = cc + i->second;
          }

          return elements;
        }
        else if (left->isDimensionTransformation(cube, &dimension)
                 && right->isDimensionTransformation(cube, &dimension)) {
          elements = right->computeDimensionTransformations(cube);
          map<Element*, string> append = right->computeDimensionTransformations(cube);
          map<Element*, string> intersection;

          for (map<Element*, string>::iterator i = elements.begin();
               i != elements.end();
               i++) {
            map<Element*, string>::iterator cc = append.find(i->first);

            if (cc != append.end()) {
              intersection[i->first] = i->second + cc->second;
            }
          }

          return intersection;
        }
        else {
          return elements;
        }
      }

      bool isConstant (Cube* cube) {
        if (valid) {
          return left->isConstant(cube) && right->isConstant(cube);
        }

        return false;
      }

      ValueType getValueType () {
        return Node::STRING;
      }      
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
    
        // concatenate has two parameters
        if (!params || params->size() < 2) {
          error = "function '" + name + "' needs two parameters";
          return valid = false;
        }
        if (params->size() > 2) {
          error = "too many parameters for function '" + name + "'";
          return valid = false;
        }    
    
        left  = params->at(0);
        right = params->at(1);
        
        // validate parameters
        if (!left->validate(server, database, cube, destination, error) ||
            !right->validate(server, database, cube, destination, error)) {
          return valid = false;         
        }
          
        // check data type left
        if (left->getValueType() != Node::STRING && 
            left->getValueType() != Node::UNKNOWN) {
          error = "first parameter of function '" + name + "' has wrong data type";        
          return valid = false;
        }
              
        // check data type right
        if (right->getValueType() != Node::STRING && 
            right->getValueType() != Node::UNKNOWN ) {
          error = "second parameter of function '" + name + "' has wrong data type";        
          return valid = false;
        }
        
        return valid = true;  
      }
    
      RuleValueType getValue (CellPath* cellPath, 
                              User* user,
                              bool* isCachable, 
                              set< pair<Rule*, IdentifiersType> >* ruleHistory) {        
        if (valid) {
          RuleValueType l  = left->getValue(cellPath,user,isCachable, ruleHistory); 
          RuleValueType r  = right->getValue(cellPath,user,isCachable, ruleHistory); 
          if (l.type == Node::STRING) {
            if (r.type == Node::STRING) {
              RuleValueType result;
              result.type = Node::STRING; 
              result.stringValue = l.stringValue + r.stringValue;
              return result;
            }
            else {
              return l;
            }
          }
          else if (r.type == Node::STRING) {
            return r;
          }
        }
        
        RuleValueType result;
        result.type = Node::STRING; 
        result.stringValue = "";
        return result;
      }
      
    protected:
      Node* left;
      Node* right;  
      
  };

}
#endif

