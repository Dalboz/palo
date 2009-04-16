////////////////////////////////////////////////////////////////////////////////
/// @brief palo system cube
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

#ifndef OLAP_SYSTEM_CUBE_H
#define OLAP_SYSTEM_CUBE_H 1

#include "palo.h"

#include "Olap/Cube.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief system OLAP cube
  ///
  /// An OLAP cube is an ordered list of elements
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS SystemCube : public Cube {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sub types of a system cube
    ////////////////////////////////////////////////////////////////////////////////

    enum SystemCubeType {
      RIGHTS_CUBE,
      ATTRIBUTES_CUBE,
      CONFIGURATION_CUBE,
      SUBSET_VIEW_CUBE
    };

  public:
    static const string PREFIX_GROUP_DIMENSION_DATA;
    static const string GROUP_CUBE_DATA;
    static const string CONFIGURATION_DATA;
    static const string NAME_VIEW_LOCAL_CUBE;
    static const string NAME_VIEW_GLOBAL_CUBE;
    static const string NAME_SUBSET_LOCAL_CUBE;
    static const string NAME_SUBSET_GLOBAL_CUBE;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates empty cube
    ///
    /// @throws ParameterException on double or missing dimensions
    ////////////////////////////////////////////////////////////////////////////////

    SystemCube (IdentifierType identifier,
                const string& name,
                Database* database,
                vector<Dimension*>* dimensions)
      : Cube (identifier, name, database, dimensions) {
    }

  public:
    ItemType getType () const {
      return SYSTEM;
    }
  
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the sub type of a system cube
    ////////////////////////////////////////////////////////////////////////////////

    virtual SystemCubeType getSubType () const = 0;
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief saves data to file  
    ////////////////////////////////////////////////////////////////////////////////

    void saveCube ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief reads data from file
    ////////////////////////////////////////////////////////////////////////////////

    void loadCube (bool processJournal);

    
  };

}

#endif
