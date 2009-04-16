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


#ifndef PARSER_FUNCTION_NODE_ISERROR_H
#define PARSER_FUNCTION_NODE_ISERROR_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node str
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeIsError : public FunctionNode {
      
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeIsError(name, params);
      }

    public:
      FunctionNodeIsError () : FunctionNode() {};
      FunctionNodeIsError (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {};    

    public:
    
      ValueType getValueType () {
		  return Node::NUMERIC;
      }      

	  Node * clone () {
        FunctionNodeIsError * cloned = new FunctionNodeIsError(name, cloneParameters());
        cloned->valid = this->valid;
        return cloned;
      }
    
      bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
    
        // add has two parameters
        if (!params || params->size() !=1) {
          error = "function '" + name + "' needs exactly one parameter";
          return valid = false;
        }        
    
        p1 = params->at(0);        
        
        // validate parameters
        if (!p1->validate(server, database, cube, destination, error)) {
          return valid = false;         
        }
        
        return valid = true;  
	  }
    
      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {        

		RuleValueType result;
		result.type = Node::NUMERIC; 

        if (valid) {
			try {
				RuleValueType v1  = p1->getValue(cellPath,user,isCachable, ruleHistory); 
				if (v1.type == Node::STRING || v1.type == Node::NUMERIC) {
					result.doubleValue = 0;
					return result;
				}
			} catch(...) {

			}          
		}

		result.doubleValue = 1;
        return result;
      }
      
    protected:
      Node* p1;
  };

}
#endif
