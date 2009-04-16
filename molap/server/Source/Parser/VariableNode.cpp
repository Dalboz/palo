////////////////////////////////////////////////////////////////////////////////
/// @brief parser variable node
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

#include "Parser/VariableNode.h"

#include "Collections/StringUtils.h"

namespace palo {
  VariableNode::VariableNode (string *value) 
    : value(value) {
    valid = false;
    num = -1;
    dimension = 0;
  }
  


  VariableNode::~VariableNode () {
    delete value;
  }
  


  Node * VariableNode::clone () {
    VariableNode * cloned = new VariableNode(new string(*value));
    cloned->valid     = this->valid;
    cloned->num       = this->num;
    cloned->dimension = this->dimension;
    return cloned;
  }



  bool VariableNode::validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
    if (database == 0 || cube == 0) {
      dimension = 0;
      return valid = true;
    }
    
    dimension = database->lookupDimensionByName(*value);

    if (! dimension) {
      error = "dimension '" + *value + "' not found";      
      return valid = false;
    }
    
    const vector<Dimension*>* dimensions = cube->getDimensions();
    int pos = 0;

    for (vector<Dimension*>::const_iterator i = dimensions->begin(); i!=dimensions->end(); i++, pos++) {
      if ((*i) == dimension) {
        num = pos;
        return valid = true;
      }
    }
    
    error = "dimension '" + *value + "' is not member of cube '" + cube->getName() + "'";      
    return valid = false;
  }



  Node::RuleValueType VariableNode::getValue (CellPath* cellPath, 
                                              User* user,
                                              bool* usesOtherDatabase, 
                                              set< pair<Rule*, IdentifiersType> >* ruleHistory) {
    RuleValueType result;
    result.type = Node::STRING;

    if (num >= 0) {
      const PathType * elements = cellPath->getPathElements();
      Element* element = elements->at(num);
      result.stringValue = element->getName();      
    }
    else {
      result.stringValue = "";
    }
    
    return result;
  }  


  
  void VariableNode::appendXmlRepresentation (StringBuffer* sb, int ident, bool) {
    identXML(sb,ident);
    sb->appendText("<variable>");
    sb->appendText(StringUtils::escapeXml(*value));
    sb->appendText("</variable>");
    sb->appendEol();    
  }



  void VariableNode::appendRepresentation (StringBuffer* sb, Cube* cube) const {
    sb->appendChar('!');
    sb->appendText(StringUtils::escapeStringSingle(*value));
  }
}
