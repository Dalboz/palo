////////////////////////////////////////////////////////////////////////////////
/// @brief parser node
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

#ifndef PARSER_NODE_H
#define PARSER_NODE_H

#include "palo.h"

#include <string>

#include "Collections/StringBuffer.h"

#include "Olap/Server.h"
#include "Olap/Database.h"
#include "Olap/Cube.h"

using namespace std;

namespace palo {
  class Dimension;
  class Element;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser node
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS Node {
    public:
      enum ValueType {
        UNKNOWN,
        NUMERIC,
        STRING,
        STET,            // ignore rule for cell
		CONTINUE		 //use next rule for cell
      };

      enum NodeType {
        NODE_FUNCTION_PALO_MARKER,
        NODE_DOUBLE_NODE,
        NODE_FUNCTION_DIV,
        NODE_FUNCTION_IF,
        NODE_FUNCTION_MULT,
        NODE_FUNCTION_STET,
        NODE_SOURCE_NODE,
        NODE_STRING_NODE,
        NODE_UNKNOWN,
        NODE_VARIABLE_NODE,
      };

      struct RuleValueType {
        ValueType type;
        double    doubleValue;
        string    stringValue;
      };

    public:

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief constructs a new node
      ////////////////////////////////////////////////////////////////////////////////

      Node () : valid(false) {
      }

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief destructs a node
      ////////////////////////////////////////////////////////////////////////////////

      virtual ~Node () {
      }


      ////////////////////////////////////////////////////////////////////////////////
      /// @brief clones an expression node
      ////////////////////////////////////////////////////////////////////////////////

      virtual Node * clone () = 0;

    public:

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief returns true if rule is valid
      ////////////////////////////////////////////////////////////////////////////////

      virtual bool isValid () {
        return valid;
      }

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief returns true if rule is simple expression (constant or area)
      ////////////////////////////////////////////////////////////////////////////////

      virtual bool isSimple () const {
        return false;
      }

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief returns the node type
      ////////////////////////////////////////////////////////////////////////////////

      virtual NodeType getNodeType () const {
        return NODE_UNKNOWN;
      }

    public:

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief validates the expression
      ////////////////////////////////////////////////////////////////////////////////

      virtual bool validate (Server*, Database*, Cube*, Node*, string&) = 0;

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief returns the value type of the underlying expression
      ////////////////////////////////////////////////////////////////////////////////

      virtual ValueType getValueType () = 0;

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief returns the value of the underlying expression
      ////////////////////////////////////////////////////////////////////////////////

      virtual RuleValueType getValue (CellPath* cellPath, User* user, bool*, set< pair<Rule*, IdentifiersType> >* ruleHistory) = 0;

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief checks if element is involved in rule
      ////////////////////////////////////////////////////////////////////////////////

      virtual bool hasElement (Dimension* dimension, IdentifierType element) const {
        return false;
      }

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief appends XML representation to string buffer
      ////////////////////////////////////////////////////////////////////////////////
      
      virtual void appendXmlRepresentation (StringBuffer* sb, int indent, bool) = 0;

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief appends representation to string buffer
      ////////////////////////////////////////////////////////////////////////////////
      
      virtual void appendRepresentation (StringBuffer*, Cube*) const {} // = 0;

    public:
      virtual bool isDimensionRestriction (Cube*, Dimension**) {
        return false;
      }

      virtual bool isDimensionTransformation (Cube*, Dimension**) {
        return false;
      }

      virtual set<Element*> computeDimensionRestriction (Cube* cube) {
        set<Element*> elements;

        return elements;
      }

      virtual map<Element*, string> computeDimensionTransformations (Cube* cube) {
        map<Element*, string> elements;

        return elements;
      }

      virtual bool isConstant (Cube*) {
        return false;
      }

      virtual void collectMarkers (vector<Node*>& markers) {
      }

    protected:
      void identXML (StringBuffer* sb, int ident) {
        for (int i = 0; i < ident*2; i++) {
          sb->appendChar(' ');
        } 
      }

    protected:
      bool valid;   
  };
}

#endif

