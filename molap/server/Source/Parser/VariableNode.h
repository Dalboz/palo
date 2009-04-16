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

#ifndef PARSER_VARIABLE_NODE_H
#define PARSER_VARIABLE_NODE_H 1

#include "palo.h"

#include <string>
#include "Parser/ExprNode.h"
#include "Olap/Server.h"
#include "Olap/Database.h"
#include "Olap/Cube.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser variable node
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS VariableNode : public ExprNode {
  
    public:
      VariableNode (string* value);

      ~VariableNode ();

      Node * clone ();
  
    public:
      bool isDimensionTransformation (Cube* cube, Dimension** dimension) {
        if (valid) {
          *dimension = this->dimension;
          return true;
        }

        return false;
      }

      map<Element*, string> computeDimensionTransformations (Cube* cube) {
        map<Element*, string> elements;

        if (dimension != 0) {
          vector<Element*> dimensionElements = dimension->getElements(0);

          for (vector<Element*>::iterator iter = dimensionElements.begin();  iter != dimensionElements.end();  iter++) {
            Element* element = *iter;

            elements[element] = element->getName();
          }
        }

        return elements;
      }

      NodeType getNodeType () const {
        return NODE_VARIABLE_NODE;
      }

      ValueType getValueType () {
        return Node::STRING;
      }

      string getStringValue () {
        return *value;
      }
  
      RuleValueType getValue (CellPath* cellPath, User* user, bool*, set< pair<Rule*, IdentifiersType> >* ruleHistory);

      bool validate (Server*, Database*, Cube*, Node*, string& );
  
      void appendXmlRepresentation (StringBuffer*, int, bool);
  
      void appendRepresentation (StringBuffer*, Cube*) const;

    private:
      string* value;
      int num;
      Dimension* dimension;
  };
}
#endif
