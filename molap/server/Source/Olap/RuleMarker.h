////////////////////////////////////////////////////////////////////////////////
/// @brief palo rule marker
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

#ifndef OLAP_RULE_MARKER_H
#define OLAP_RULE_MARKER_H 1

#include "palo.h"

#include <iostream>

#include "Parser/AreaNode.h"
#include "Parser/RuleNode.h"

namespace palo {
  class Cube;
  class Node;
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief palo rule marker
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS RuleMarker { 
    friend ostream& operator << (ostream&, const RuleMarker&);

  public:
    RuleMarker (Cube*,
                const AreaNode::Area * from,
                const AreaNode::Area * toArea);

    RuleMarker (Cube* fromCube,
                Cube* toCube,
                const vector<Node*>& path,
                const AreaNode::Area * toArea);

    ~RuleMarker ();

  public:
    Cube* getFromCube () const {
      return fromCube;
    }

    Cube* getToCube () const {
      return toCube;
    }

    const AreaNode::Area * getFromBase () const {
      return &fromBase;
    }

    const uint32_t * getPermutations () const {
      return permutations;
    }

    const uint32_t * getFixed () const {
      return fixed;
    }

    const vector< vector<uint32_t> > * getMapping () const {
      return useMapping ? &mapping : 0;
    }
    
  private:
    Cube* fromCube;
    Cube* toCube;

    AreaNode::Area fromBase;

    uint32_t * permutations;
    uint32_t * fixed;

    bool useMapping;

    vector< vector<uint32_t> > mapping;
  };

  inline ostream& operator << (ostream& out, const RuleMarker& marker) {
    out << "MARKER from '"
        << marker.fromCube->getName()
        << "' to '"
        << marker.toCube->getName()
        << "': ";

    for (AreaNode::Area::const_iterator i = marker.fromBase.begin();
         i != marker.fromBase.end();
         i++) {
      const set<IdentifierType>& s = *i;

      out << "(";

      for (set<IdentifierType>::const_iterator j = s.begin();  j != s.end();  j++) {
        out << *j;
      }

      out << ") ";
    }

    out << "=> (";

    string sep = "";

    size_t nd = marker.toCube->getDimensions()->size();
    uint32_t* q = marker.fixed;
    for (uint32_t* p = marker.permutations;  0 < nd;  nd--, p++, q++) {
      out << sep;

      IdentifierType id = *p;

      if (id == NO_IDENTIFIER) {
        out << "[" << *q << "]";
      }
      else {
        out << id;
      }

      sep = " ";
    }

    out << ")";

	return out;
  }
}

#endif
