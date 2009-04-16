////////////////////////////////////////////////////////////////////////////////
/// @brief palo cell path
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

#ifndef OLAP_CELL_PATH_H
#define OLAP_CELL_PATH_H 1

#include "palo.h"

#include <vector>

#include "Olap/Element.h"

namespace palo {
  class Cube;
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief OLAP cell path
  ///
  /// A cell path consists of a list of elements and a path type. 
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CellPath {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates cell path from list of identifiers
    ///
    /// @throws ParameterException on double or missing dimensions
    /// 
    /// The object constructor verifies the elements and computes the path type.
    /// If the list of elements is not a suitable cell path for the given cube 
    /// the constructor throws a ParameterException. 
    ////////////////////////////////////////////////////////////////////////////////

    CellPath (Cube* cube, const IdentifiersType * identifiers);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates cell path from list of identifiers
    ///
    /// @throws ParameterException on double or missing dimensions
    /// 
    /// The object constructor verifies the elements and computes the path type.
    /// If the list of elements is not a suitable cell path for the given cube 
    /// the constructor throws a ParameterException. 
    ////////////////////////////////////////////////////////////////////////////////

    CellPath (Cube* cube, const IdentifierType * identifiers);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates cell path from list of elements
    ///
    /// @throws ParameterException on double or missing dimensions
    /// 
    /// The object constructor verifies the elements and computes the path type.
    /// If the list of elements is not a suitable cell path for the given cube 
    /// the constructor throws a ParameterException. 
    ////////////////////////////////////////////////////////////////////////////////

    CellPath (Cube* cube, const PathType * elements);

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get type of path
    ////////////////////////////////////////////////////////////////////////////////

    Element::ElementType getPathType () const {
      return pathType;
    };


    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get type of path
    ////////////////////////////////////////////////////////////////////////////////

    const Cube * getCube () const {
      return cube;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get path Elements
    ////////////////////////////////////////////////////////////////////////////////

    const PathType * getPathElements () const { 
      return &pathElements;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get identifiers of path elements
    ////////////////////////////////////////////////////////////////////////////////

    const IdentifiersType * getPathIdentifier () const {
      return &pathIdentifiers;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief is base path
    ////////////////////////////////////////////////////////////////////////////////

    bool isBase () const {
      return base;
    }

  private:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief pointer to the cube
    ////////////////////////////////////////////////////////////////////////////////

    Cube * cube;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief list of path elements
    ////////////////////////////////////////////////////////////////////////////////

    PathType pathElements;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief list of identifiers (identifiers of the elements) 
    ////////////////////////////////////////////////////////////////////////////////

    IdentifiersType pathIdentifiers;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief path type (NUMERIC, STRING, CONSOLIDATED)
    ////////////////////////////////////////////////////////////////////////////////

    Element::ElementType pathType;


    ////////////////////////////////////////////////////////////////////////////////
    /// @brief true if path contains only base elements
    ////////////////////////////////////////////////////////////////////////////////

    bool base;
  };
  
}

#endif 
