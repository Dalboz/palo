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

#include "Parser/RuleNode.h"

#include "Collections/DeleteObject.h"

#include "Parser/SourceNode.h"

namespace palo {

  RuleNode::RuleNode (RuleOption ruleOption, DestinationNode *destination, ExprNode *expr)
    : destinationNode(destination), exprNode(expr), ruleOption(ruleOption) {
  }

  
  
  RuleNode::RuleNode (RuleOption ruleOption, DestinationNode *destination, ExprNode *expr, vector<Node*>* markers)
    : destinationNode(destination), exprNode(expr), ruleOption(ruleOption), externalMarkers(*markers) {
  }

  
  
  RuleNode::~RuleNode () {
    if (destinationNode) {
      delete destinationNode;
    }

    if (exprNode) {
      delete exprNode;
    }

    for_each(externalMarkers.begin(), externalMarkers.end(), DeleteObject());
  }
  

  
  Node * RuleNode::clone () {
    RuleNode * cloned = new RuleNode(this->ruleOption,
                                     dynamic_cast<DestinationNode*>(this->destinationNode->clone()),
                                     dynamic_cast<ExprNode*>(this->exprNode));
    cloned->valid = this->valid;
    return cloned;
  }



  bool RuleNode::validate (Server* server, Database* database, Cube* cube, Node* destination, string& message) {
    if (destinationNode->validate(server, database, cube, 0, message) 
        && exprNode->validate(server, database, cube, destinationNode, message) ) {
      valid = true;
    }

    if (valid && ! externalMarkers.empty() && ruleOption != BASE) {
      message = "external markers are only allowed in N: rules";
      valid = false;
    }

    if (valid && ! externalMarkers.empty()) {
      for (vector<Node*>::iterator i = externalMarkers.begin();  i != externalMarkers.end();  i++) {
        Node* node = *i;

        if (! node->validate(server, database, cube, 0, message)) {
          valid = false;
          break;
        }

        if (node->getNodeType() == NODE_SOURCE_NODE) {
          SourceNode* src = dynamic_cast<SourceNode*>(node);

          if (! src->isMarker()) {
            message = "only markered source areas or PALO.MARKER are allowed, got unmarked source area";
            valid = false;
            break;
          }
        }
        else if (node->getNodeType() != NODE_FUNCTION_PALO_MARKER) {
          message = "only markered source areas or PALO.MARKER are allowed";
          valid = false;
          break;
        }
      }
    }

    if (valid) {
      internalMarkers.clear();
      exprNode->collectMarkers(internalMarkers);
    }

    return valid;
  }
  
  
  bool RuleNode::validate (Server* server, Database* database, Cube* cube, string& message) {
    return validate(server, database, cube, 0, message);
  }
  
  

  bool RuleNode::hasElement (Dimension* dimension, IdentifierType element) const {
    if (destinationNode != 0 && destinationNode->hasElement(dimension, element)) {
      return true;
    }

    if (exprNode != 0 && exprNode->hasElement(dimension, element)) {
      return true;
    }

    for (vector<Node*>::const_iterator i = externalMarkers.begin();  i != externalMarkers.end();  i++) {
      Node* node = *i;

      if (node->hasElement(dimension, element)) {
        return true;
      }
    }

    return false;
  }
  


  void RuleNode::appendXmlRepresentation (StringBuffer* sb, int ident, bool names) {
    switch (ruleOption) {
      case CONSOLIDATION:
        sb->appendText("<rule path=\"none-base\">");
        break;
      case BASE:
        sb->appendText("<rule path=\"base\">");
        break;
      default:
        sb->appendText("<rule>");
    }        
    sb->appendEol();        

    destinationNode->appendXmlRepresentation(sb, 1, names);

    identXML(sb,1);
    sb->appendText("<definition>");
    sb->appendEol();        

    exprNode->appendXmlRepresentation(sb, 2, names);

    identXML(sb,1);
    sb->appendText("</definition>");
    sb->appendEol();

    if (! externalMarkers.empty()) {
      identXML(sb,1);
      sb->appendText("<external-markers>");
      sb->appendEol();
      
      for (vector<Node*>::iterator i = externalMarkers.begin();  i != externalMarkers.end();  i++) {
        (*i)->appendXmlRepresentation(sb, 2, names);
      }

      identXML(sb,1);
      sb->appendText("</external-markers>");
      sb->appendEol();
    }

    sb->appendText("</rule>");
    sb->appendEol();          
  }


  
  void RuleNode::appendRepresentation (StringBuffer* sb, Cube* cube) const {
    destinationNode->appendRepresentation(sb, cube);

    sb->appendText(" = ");

    switch (ruleOption) {
      case CONSOLIDATION:
        sb->appendText("C:");
        break;

      case BASE:
        sb->appendText("N:");
        break;

      default:
        break;
    }

    exprNode->appendRepresentation(sb, cube);

    if (! externalMarkers.empty()) {
      sb->appendText(" @ ");

      const char * sep = "";

      for (vector<Node*>::const_iterator i = externalMarkers.begin();  i != externalMarkers.end();  i++) {
        Node* node = *i;

        sb->appendText(sep);
        node->appendRepresentation(sb, cube);
        sep = ",";
      }
    }
  }
}
