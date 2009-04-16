////////////////////////////////////////////////////////////////////////////////
/// @brief rule optimizer
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

#include "Parser/RuleOptimizer.h"

#include "Parser/DoubleNode.h"
#include "Parser/ExprNode.h"
#include "Parser/FunctionNodeSimple.h"
#include "Parser/RuleNode.h"
#include "Parser/SourceNode.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  // auxillary functions
  ////////////////////////////////////////////////////////////////////////////////

  static string rule2string (Cube* cube, Node* node) {
    StringBuffer sb;
    sb.initialize();
    node->appendRepresentation(&sb, cube);

    return sb.c_str();
  }
  
  ////////////////////////////////////////////////////////////////////////////////
  // constructors and destructors
  ////////////////////////////////////////////////////////////////////////////////

  RuleOptimizer::RuleOptimizer (Cube* cube) 
    : cube(cube),
      restrictedRule(0),
      linearRule(false) {
  }



  RuleOptimizer::~RuleOptimizer () {
    if (restrictedRule != 0) {
      delete restrictedRule;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // optimizer
  ////////////////////////////////////////////////////////////////////////////////

  bool RuleOptimizer::optimize (RuleNode* node) {
    Logger::debug << "starting to optimize " << rule2string(cube, node) << endl;

    if (restrictedRule != 0) {
      delete restrictedRule;
    }

    // STET rule check (sets restrictedRule and other variables)
    checkStetRule(node->getExprNode(), node->getDestinationArea());
    
    // linear rule check
    linearRule = false;

    if (node->getRuleOption() == RuleNode::BASE) {
      if (restrictedRule != 0) {
        checkLinearRule(restrictedRule, node->getDestinationArea());
      }
      else {
        checkLinearRule(node->getExprNode(), node->getDestinationArea());
      }
    }

    // if STET test was successful
    return restrictedRule != 0;
  }



  ExprNode * RuleOptimizer::getRestrictedRule () const {
    if (restrictedRule == 0) {
      return 0;
    }

    return dynamic_cast<ExprNode*>(restrictedRule->clone());
  }



  const set<IdentifierType>& RuleOptimizer::getRestrictedIdentifiers () const {
    return restrictedIdentifiers;
  }



  Dimension* RuleOptimizer::getRestrictedDimension () const {
    return restrictedDimension;
  }



  bool RuleOptimizer::isLinearRule () {
    return linearRule;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // auxillary methods
  ////////////////////////////////////////////////////////////////////////////////

  void RuleOptimizer::checkLinearRule (ExprNode * node, AreaNode::Area * area) {
    Logger::debug << "checking for linearity " << rule2string(cube, node) << endl;

    if (! node->isValid()) {
      Logger::debug << "rule is invalid, stopping linearity check" << endl;
      return;
    }

    // the rule should be "[] = [] * CONSTANT" or "[] = CONSTANT * []"
    Node::NodeType t = node->getNodeType();

    if (t != Node::NODE_FUNCTION_MULT && t != Node::NODE_FUNCTION_DIV) {
      Logger::debug << "linearity works only for '*' or '/', stopping linearity check" << endl;
      return;
    }

    FunctionNodeSimple * op = dynamic_cast<FunctionNodeSimple*>(node);
    Node* left = op->getLeftNode();
    Node* right = op->getRightNode();
    DoubleNode* doubleNode = 0;
    SourceNode* sourceNode = 0;

    // case "[] = CONSTANT * []"
    if (t == Node::NODE_FUNCTION_MULT
        && left->getNodeType() == Node::NODE_DOUBLE_NODE
        && right->getNodeType() == Node::NODE_SOURCE_NODE) {
      Logger::debug << "found DOUBLE_NODE * SOURCE_NODE" << endl;

      doubleNode = dynamic_cast<DoubleNode*>(left);
      sourceNode = dynamic_cast<SourceNode*>(right);
    }

    // case "[] = [] * CONSTANT" or "[] = CONSTANT * []"
    else if (left->getNodeType() == Node::NODE_SOURCE_NODE
             && right->getNodeType() == Node::NODE_DOUBLE_NODE) {
      Logger::debug << "found SOURCE_NODE op DOUBLE_NODE" << endl;

      doubleNode = dynamic_cast<DoubleNode*>(right);
      sourceNode = dynamic_cast<SourceNode*>(left);
    }

    // something else
    else {
      Logger::debug << "neither SOURCE_NODE op DOUBLE_NODE nor DOUBLE_NODE * SOURCE_NODE, stopping linearity check" << endl;
      return;
    }

    // check that the destination area contains only base elements
    const vector<Dimension*> * dimensions = cube->getDimensions();
    vector<Dimension*>::const_iterator i = dimensions->begin();
    AreaNode::Area::iterator j = area->begin();

    for (;  i != dimensions->end();  i++, j++) {
      Dimension* dimension = *i;
      set<IdentifierType>::iterator k = j->begin();

      for (;  k != j->end();  k++) {
        Element* element = dimension->lookupElement(*k);

        if (element == 0) {
          Logger::warning << "cannot find dimension element with identifier "
                          << *k << " in dimension " << dimension->getName()
                          << endl;
        }

        if (element->getElementType() != Element::NUMERIC) {
          Logger::debug << "found non-numeric element " << element->getName()
                        << " in dimension " << dimension->getName() 
                        << ", stopping linearity check" << endl;
          return;
        }
      }
    }

    // check that the destination and source area are of the same shape
    AreaNode::Area * sourceArea = sourceNode->getSourceArea();
    AreaNode::Area::iterator k = sourceArea->begin();
    j = area->begin();

    for (; j != area->end() && k != sourceArea->end();  j++, k++) {
      const set<IdentifierType>& jj = *j;
      const set<IdentifierType>& kk = *k;

      if (jj.size() != kk.size()) {
        Logger::debug << "source and destination area have different shape, stopping linearity check" << endl;
        return;
      }
    }

    Logger::debug << "rule is linear" << endl;
    linearRule = true;
  }



  void RuleOptimizer::checkStetRule (ExprNode * node, AreaNode::Area * area) {
    Logger::debug << "checking for STET rule " << rule2string(cube, node) << endl;

    if (! node->isValid()) {
      Logger::debug << "rule is invalid, stopping STET rule check" << endl;
      return;
    }

    // the rule should be "[] = IF(...,STET(),...)" or "[] = IF(...,...,STET())"
    Node::NodeType t = node->getNodeType();

    if (t != Node::NODE_FUNCTION_IF) {
      Logger::debug << "no IF clause, stopping STET rule check" << endl;
      return;
    }

    FunctionNode * ifNode = dynamic_cast<FunctionNode*>(node);
    ExprNode* clause = dynamic_cast<ExprNode*>(ifNode->getParameters()->at(0));

    if (clause == 0) {
      Logger::warning << "something is wrong, corrupted rule " << rule2string(cube, node) << endl;
      return;
    }

    Node* trueNode   = ifNode->getParameters()->at(1);
    Node* falseNode  = ifNode->getParameters()->at(2);

    // either true or false node must be STET
    Node* nonStetNode = 0;
    bool isInclusive = false;

    if (trueNode->getNodeType() == Node::NODE_FUNCTION_STET) {
      nonStetNode = falseNode;
      isInclusive = false; // use complementary clause-set
    }
    else if (falseNode->getNodeType() == Node::NODE_FUNCTION_STET) {
      nonStetNode = trueNode;
      isInclusive = true; // use clause-set
    }
    else {
      Logger::debug << "no STET as true or false value, stopping STET rule check" << endl;
      return;
    }

    // check if clause is a dimension restriction
    Logger::debug << "checking if-clause " << rule2string(cube,clause) << endl;

    Dimension* dimension;
    bool restriction = clause->isDimensionRestriction(cube, &dimension);

    if (! restriction) {
      Logger::debug << "if-clause is no dimension restriction, stopping STET rule check" << endl;
      return;
    }

    // ok find the dimension restriction
    set<Element*> elements = clause->computeDimensionRestriction(cube);

    // find dimension position
    int pos = 0;
    const vector<Dimension*> * dimensions = cube->getDimensions();

    for (vector<Dimension*>::const_iterator iter = dimensions->begin();  iter != dimensions->end();  iter++, pos++) {
      if (*iter == dimension) {
        break;
      }
    }

    Logger::debug << "dimension restriction for dimension '" << dimension->getName() << "', position " << pos << endl;

    // find existing restriction
    const set<IdentifierType>& givenRestriction = (*area)[pos];
    set<IdentifierType> computedRestriction;

    // intersect with computed restriction
    if (isInclusive) {

      // no given restriction
      if (givenRestriction.empty()) {
        for (set<Element*>::iterator iter = elements.begin();  iter != elements.end();  iter++) {
          Element* element = *iter;
          IdentifierType id = element->getIdentifier();

          computedRestriction.insert(id);
          Logger::debug << "dimension element " << element->getName() << endl;
        }
      }

      // restriction given in rule
      else {
        for (set<Element*>::iterator iter = elements.begin();  iter != elements.end();  iter++) {
          Element* element = *iter;
          IdentifierType id = element->getIdentifier();

          if (givenRestriction.find(id) != givenRestriction.end()) {
            computedRestriction.insert(id);
            Logger::debug << "dimension element " << element->getName() << endl;
          }
        }
      }
    }

    // intersect with complement of computed restriction
    else {

      // no given restriction
      if (givenRestriction.empty()) {
        vector<Element*> allElements = dimension->getElements(0);

        for (vector<Element*>::iterator iter = allElements.begin();  iter != allElements.end();  iter++) {
          Element* element = *iter;

          if (elements.find(element) == elements.end()) {
            IdentifierType id = element->getIdentifier();
            computedRestriction.insert(id);
            Logger::debug << "dimension element " << element->getName() << endl;
          }
        }
      }

      // restriction given in rule
      else {
        set<IdentifierType> idList;

        for (set<Element*>::iterator iter = elements.begin();  iter != elements.end();  iter++) {
          Element* element = *iter;
          IdentifierType id = element->getIdentifier();

          idList.insert(id);
        }

        for (set<IdentifierType>::const_iterator iter = givenRestriction.begin();
             iter != givenRestriction.end();
             iter++) {
          IdentifierType id = *iter;

          if (idList.find(id) == idList.end()) {
            computedRestriction.insert(id);
            Logger::debug << "dimension element identifier " << id << endl;
          }
        }
      }
    }

    restrictedRule = dynamic_cast<ExprNode*>(nonStetNode->clone());
    restrictedDimension = dimension;
    restrictedIdentifiers = computedRestriction;

    Logger::debug << "using rule " << rule2string(cube, restrictedRule) << " for restricted area" << endl;
  }
}
