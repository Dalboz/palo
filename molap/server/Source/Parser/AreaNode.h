////////////////////////////////////////////////////////////////////////////////
/// @brief base class for parser source or destination node
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

#ifndef PARSER_AREA_NODE_H
#define PARSER_AREA_NODE_H

#include "palo.h"

#include <iostream>
#include <string>
#include <vector>

#include "Parser/Node.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief base class for parser source or destination node
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS AreaNode : virtual public Node {
    public:
      typedef vector< set<IdentifierType> > Area;

    public:
      AreaNode (vector< pair<string*, string*> *> * elements, vector< pair<int, int> *> * elementsIds);

      virtual ~AreaNode ();

    public:
      bool validate (Server*, Database*, Cube*, Node*, string&);

      AreaNode::Area* getNodeArea () {
        return nodeArea;
      }

    protected:
      virtual bool validateNames (Server*, Database*, Cube*, Node*, string&) = 0;

      virtual bool validateIds (Server*, Database*, Cube*, Node*, string&) = 0;

    protected:
      void appendRepresentation (StringBuffer*, Cube*, bool isMarker) const;

      bool validateNamesArea (Server*, Database*, Cube*, Node*, string&);
    
      bool validateIdsArea (Server*, Database*, Cube*, Node*, string&);
       
      bool isSimple () const {
        return true;
      }

      void appendXmlRepresentationType (StringBuffer*, int indent, bool names, const string& type);

    protected:
      // first = name of dimension (can be empty)
      // second = name of dimension element (can be empty)
      // or:
      // first = id of dimension
      // second = id of dimension element (-1 for empty)

      vector< pair<string*, string*> *> *elements;      
      vector< pair<int, int> *>         *elementsIds;

      vector<IdentifierType> dimensionIDs;
      vector<IdentifierType> elementIDs;
      vector<bool> isRestricted;      
      vector<bool> isQualified;
      vector<int> elementSequence;

      // true if the source node has an unrestricted dimension 
      bool unrestrictedDimensions;
       
      AreaNode::Area* nodeArea;
  };
}

#endif
