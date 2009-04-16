////////////////////////////////////////////////////////////////////////////////
/// @brief parser function node
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

#include "Parser/FunctionNode.h"

#include <sstream>

namespace palo {
  FunctionNode::FunctionNode ()
    : ExprNode(), name(""), params(0) {
  }  



  FunctionNode::FunctionNode (const string& name,  vector<Node*> *params) 
    : ExprNode(), name(name), params(params) {
  }  


  
  FunctionNode::~FunctionNode () {
    if (params) {
      for (vector<Node*>::iterator i = params->begin(); i!= params->end(); i++) {
        delete *i;
      }
      delete params;
    }
  }



  FunctionNode::ValueType FunctionNode::getValueType () {
    return Node::UNKNOWN;
  }



  bool FunctionNode::hasElement (Dimension* dimension, IdentifierType element) const {
    if (params) {
      for (vector<Node*>::iterator i = params->begin(); i!= params->end(); i++) {
        if ((*i)->hasElement(dimension, element)) {
          return true;
        }
      }      
    }

    return false;
  }
  


  void FunctionNode::appendXmlRepresentation (StringBuffer* sb, int ident, bool names) {
    identXML(sb,ident);
    sb->appendText("<function name=\"");
    sb->appendText(StringUtils::toupper(StringUtils::escapeXml(name)));
    sb->appendText("\">");
    sb->appendEol();        

    if (params) {
      for (vector<Node*>::iterator i = params->begin(); i!= params->end(); i++) {
        (*i)->appendXmlRepresentation(sb, ident + 1, names);
      }
    }

    identXML(sb,ident);
    sb->appendText("</function>");
    sb->appendEol();          
  }
  


  void FunctionNode::appendRepresentation (StringBuffer* sb, Cube* cube) const {
    sb->appendText(StringUtils::toupper(name));
    sb->appendChar('(');

    if (params) {
      for (vector<Node*>::const_iterator i = params->begin(); i!= params->end(); i++) {
        if (i != params->begin()) {
          sb->appendChar(',');
        }

        (*i)->appendRepresentation(sb, cube);
      }
    }

    sb->appendChar(')');
  }



  vector<Node*> * FunctionNode::cloneParameters () {
    vector<Node*> * cloned = new vector<Node*>(params->size(), 0);

    vector<Node*>::iterator i = params->begin();
    vector<Node*>::iterator j = cloned->begin();

    for (;  i != params->end();  i++, j++) {
      if (*i != 0) {
        *j = (*i)->clone();
      }
    }

    return cloned;
  }
}
