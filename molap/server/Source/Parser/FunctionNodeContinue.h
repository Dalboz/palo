////////////////////////////////////////////////////////////////////////////////
/// @brief function node continue
///
/// @file
///
/// Developed by Marko Stijak, Banja Luka on behalf of Jedox AG.
/// Copyright and exclusive worldwide exploitation right has
/// Jedox AG, Freiburg.
///
/// @author Marko Stijak, Banja Luka, Bosnia and Herzegovina
////////////////////////////////////////////////////////////////////////////////

#ifndef PARSER_FUNCTION_NODE_CONTINUE_H
#define PARSER_FUNCTION_NODE_CONTINUE_H 1

#include "palo.h"

#include <string>
#include "Parser/FunctionNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node simple continue
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeContinue : public FunctionNode {

    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeContinue(name, params);
      }
      
    public:
      FunctionNodeContinue () : FunctionNode() {
      }

      FunctionNodeContinue (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
      }

      Node * clone () {
        FunctionNodeContinue * cloned = new FunctionNodeContinue(name, cloneParameters());
        cloned->valid = this->valid;
        return cloned;
      }

    public:

      ValueType getValueType () {
		  return Node::CONTINUE;
      }
      
      bool validate (Server*, Database*, Cube*, Node*, string& error) {
        // add has two parameters
        if (params && params->size() > 0) {
          error = "function '" + name + "' has no parameter";
          return valid = false;
        }
          
        return valid = true;
      }

      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        RuleValueType result;
		result.type = Node::CONTINUE; 
        return result;
      }
      
  };

}
#endif

