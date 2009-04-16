////////////////////////////////////////////////////////////////////////////////
/// @brief
///
/// @file
///
/// Developed by Marko Stijak, Banja Luka on behalf of Jedox AG.
/// Copyright and exclusive worldwide exploitation right has
/// Jedox AG, Freiburg.
///
/// @author Marko Stijak, Banja Luka, Bosnia and Herzegovina
////////////////////////////////////////////////////////////////////////////////


#ifndef PARSER_FUNCTION_NODE_STR_H
#define PARSER_FUNCTION_NODE_STR_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node str
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeStr : public FunctionNode {
      
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeStr(name, params);
      }

    public:
      FunctionNodeStr () : FunctionNode() {};
      FunctionNodeStr (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {};    

    public:
    
      ValueType getValueType () {
        return Node::STRING;
      }      

	  Node * clone () {
        FunctionNodeStr * cloned = new FunctionNodeStr(name, cloneParameters());
        cloned->valid = this->valid;
        return cloned;
      }
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
    
        // add has two parameters
        if (!params || params->size() < 3) {
          error = "function '" + name + "' needs three parameters";
          return valid = false;
        }
        if (params->size() > 3) {
          error = "too many parameters for function '" + name + "'";
          return valid = false;
        }    
    
        p1 = params->at(0);
        p2 = params->at(1);
		p3 = params->at(2);
        
        // validate parameters
        if (!p1->validate(server, database, cube, destination, error) ||
            !p2->validate(server, database, cube, destination, error) || 
			!p3->validate(server, database, cube, destination, error)) {
          return valid = false;         
        }
          
        // check data type p1
        if (p1->getValueType() != Node::NUMERIC && 
            p1->getValueType() != Node::UNKNOWN) {
          error = "first parameter of function '" + name + "' has wrong data type";        
          return valid = false;
        }
              
        // check data type p2
		if (p2->getValueType() != Node::NUMERIC && 
            p2->getValueType() != Node::UNKNOWN ) {
          error = "second parameter of function '" + name + "' has wrong data type";        
          return valid = false;
        }

		// check data type p3
		if (p3->getValueType() != Node::NUMERIC && 
            p3->getValueType() != Node::UNKNOWN ) {
          error = "third parameter of function '" + name + "' has wrong data type";        
          return valid = false;
		}
        
        return valid = true;  
	  }
    
      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {        

		RuleValueType result;
        result.type = Node::STRING; 

        if (valid) {
          RuleValueType v1  = p1->getValue(cellPath,user,isCachable, ruleHistory); 
          RuleValueType v2  = p2->getValue(cellPath,user,isCachable, ruleHistory); 
		  RuleValueType v3  = p3->getValue(cellPath,user,isCachable, ruleHistory); 

		  if (v1.type == Node::NUMERIC && v2.type==Node::NUMERIC && v3.type==Node::NUMERIC)
		  {
			double v = v1.doubleValue;
			int l = (int)v2.doubleValue;
			int d = (int)v3.doubleValue;
			if (l>=0 && d>=0 && l<256 && d<200) {
				char format[16];				
				sprintf(format, "%%%d.%df", l, d);
				char str[256];
				char* oldLocale = setlocale(LC_ALL, NULL);
				setlocale(LC_NUMERIC, "");
				sprintf(str, format, v);
				setlocale(LC_NUMERIC, oldLocale); 
				result.stringValue = str;
				return result;
			}
		  }
		}

		result.stringValue = "";
        return result;
      }
      
    protected:
      Node* p1;
      Node* p2;  
	  Node* p3;
      
  };

}
#endif
