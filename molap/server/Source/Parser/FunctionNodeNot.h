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

#ifndef PARSER_FUNCTION_NODE_NOT_H
#define PARSER_FUNCTION_NODE_NOT_H 1

#include "palo.h"
#include <string>
#include "Parser/FunctionNodeSimpleSingle.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser function node cos
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FunctionNodeNot : public FunctionNodeSimpleSingle {
    public:
      static FunctionNode* createNode (const string& name, vector<Node*> *params) {
        return new FunctionNodeNot(name, params);
      }

    public:
      FunctionNodeNot () : FunctionNodeSimpleSingle() {
      }

      FunctionNodeNot (const string& name,  vector<Node*> *params) : FunctionNodeSimpleSingle(name, params) {
      }

      Node * clone () {
        FunctionNodeNot * cloned = new FunctionNodeNot(name, cloneParameters());
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
            if (result.doubleValue!=0)
				result.doubleValue = 0.0;
			else
				result.doubleValue = 1.0;
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

