////////////////////////////////////////////////////////////////////////////////
/// @brief parser rule node
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

#ifndef PARSER_RULE_NODE_H
#define PARSER_RULE_NODE_H 1

#include "palo.h"

#include <string>
#include "Parser/Node.h"
#include "Parser/ExprNode.h"
#include "Parser/DestinationNode.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser rule node
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS RuleNode : public Node {
  public:
    enum RuleOption {
      NONE,
      CONSOLIDATION,
      BASE
    };
  
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief constructs a new rule node
    ////////////////////////////////////////////////////////////////////////////////

    RuleNode (RuleOption, DestinationNode*, ExprNode*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief constructs a new rule node with markers
    ////////////////////////////////////////////////////////////////////////////////

    RuleNode (RuleOption, DestinationNode*, ExprNode*, vector<Node*>* markers);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief destructs a rule node
    ////////////////////////////////////////////////////////////////////////////////

    ~RuleNode ();
  
    Node * clone ();

  public:
      bool validate (Server*, Database*, Cube*, string& );

      bool hasElement (Dimension* dimension, IdentifierType element) const;

      void appendXmlRepresentation (StringBuffer*, int, bool);  

      void appendRepresentation (StringBuffer*, Cube*) const;

  public:
      ValueType getValueType () {
        if (exprNode) {
          return exprNode->getValueType();
        }

        return Node::UNKNOWN;
      }



      RuleValueType getValue (CellPath* cellPath,
                              User* user,
                              bool* isCachable,
                              set< pair<Rule*, IdentifiersType> >* ruleHistory) {
        *isCachable = true;
        
        if (exprNode) {
          return exprNode->getValue(cellPath, user, isCachable, ruleHistory);
        }

        RuleValueType result;
        result.type = Node::UNKNOWN; 

        return result;
      }

  public:

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief returns the underlying expression
      ////////////////////////////////////////////////////////////////////////////////

      ExprNode* getExprNode () {
        return exprNode;
      }      

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief returns the rule option
      ////////////////////////////////////////////////////////////////////////////////

      RuleOption getRuleOption () {
        return ruleOption;
      }      
      
      ////////////////////////////////////////////////////////////////////////////////
      /// @brief returns the destination area
      ////////////////////////////////////////////////////////////////////////////////

      AreaNode::Area * getDestinationArea() {
        if (destinationNode) {
          return destinationNode->getDestinationArea();
        }

        return 0;
      }      

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief returns the destination node
      ////////////////////////////////////////////////////////////////////////////////

      DestinationNode* getDestinationNode () {
        return destinationNode;
      }

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief returns the external markers
      ////////////////////////////////////////////////////////////////////////////////

      const vector<Node*>& getExternalMarkers () const {
        return externalMarkers;
      }
    
      ////////////////////////////////////////////////////////////////////////////////
      /// @brief returns the internal markers
      ////////////////////////////////////////////////////////////////////////////////

      const vector<Node*>& getInternalMarkers () const {
        return internalMarkers;
      }
    
    private:
      bool validate (Server*, Database*, Cube*, Node*, string& );

    private:
      DestinationNode *destinationNode;
      ExprNode *exprNode;
      RuleOption ruleOption;
      vector<Node*> externalMarkers;
      vector<Node*> internalMarkers;
  };
}

#endif
