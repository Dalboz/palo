////////////////////////////////////////////////////////////////////////////////
/// @brief parser source node
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

#ifndef PARSER_SOURCE_NODE_H
#define PARSER_SOURCE_NODE_H

#include "palo.h"

#include <string>
#include <vector>
#include <iostream>

#include "Parser/AreaNode.h"
#include "Parser/ExprNode.h"

using namespace std;

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief parser source node
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS SourceNode : public AreaNode, public ExprNode {
    public:
      SourceNode (vector< pair<string*, string*> *> *elements, vector< pair<int, int> *> *elementsIds);

      SourceNode (vector< pair<string*, string*> *> *elements, vector< pair<int, int> *> *elementsIds, bool marker);

      virtual ~SourceNode ();

      Node * clone ();
  
    public:
      NodeType getNodeType () const {
        return NODE_SOURCE_NODE;
      }

      Node::ValueType getValueType ();
  
      RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory);
      
      bool hasElement (Dimension*, IdentifierType) const;

      bool isSimple () const {
        return true;
      }

      bool isMarker () const {
        return marker;
      }

      void appendRepresentation (StringBuffer* sb, Cube* cube) const {
        AreaNode::appendRepresentation(sb, cube, marker);
      }

      void appendXmlRepresentation (StringBuffer*, int, bool);

      AreaNode::Area* getSourceArea() {
        return nodeArea;
      }

      bool validate (Server* server, Database* database, Cube* cube, Node* node, string& err) {
        return AreaNode::validate(server, database, cube, node, err);
      }

      void collectMarkers (vector<Node*>& markers) {
        if (marker) {
          markers.push_back(this);
        }
      }

    protected:
      bool validateNames (Server*, Database*, Cube*, Node*, string&);
    
      bool validateIds (Server*, Database*, Cube*, Node*, string&);
       
    private:
      Server* server;
      Database* database;
      Cube* cube;
      CellPath* fixedCellPath;
      bool marker;
  };
}
#endif

