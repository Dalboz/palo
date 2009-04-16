////////////////////////////////////////////////////////////////////////////////
/// @brief palo noraml database
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

#ifndef OLAP_NORMAL_DATABASE_H
#define OLAP_NORMAL_DATABASE_H 1

#include "palo.h"

#include "Olap/Database.h"
#include "Olap/ConfigurationCube.h"

namespace palo {
  class User;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief OLAP normal database
  ///
  /// An OLAP database consists of dimensions and cubes
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS NormalDatabase : public Database {
  public:
    static const uint32_t DATABASE_TYPE = 1;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates new database with given identifier
    ////////////////////////////////////////////////////////////////////////////////

    NormalDatabase (IdentifierType, Server*, const string& name);
    //NormalDatabase (IdentifierType databaseIdentifier, Server* server, const FileName& fileName);

  public:
    void notifyAddDatabase ();

  public:
    void saveDatabaseType (FileWriter*);
    void loadDatabase ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name getter and setter
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets group dimension
    ////////////////////////////////////////////////////////////////////////////////

    Dimension * getGroupDimension () {
      if (groupDimension == 0) {
        addSystemDimension();
      }
      return groupDimension;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets cube dimension
    ////////////////////////////////////////////////////////////////////////////////

    Dimension * getCubeDimension () {
      if (cubeDimension == 0) {
        addSystemDimension();
      }
      return cubeDimension;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets configuration dimension
    ////////////////////////////////////////////////////////////////////////////////

    Dimension * getConfigurationDimension () {
      if (configurationDimension == 0) {
        addSystemDimension();
      }
      return configurationDimension;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets dimension dimension
    ////////////////////////////////////////////////////////////////////////////////

    Dimension * getDimensionDimension () {
      if (dimensionDimension == 0) {
        addSystemDimension();
      }
      return dimensionDimension;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:
    ItemType getType () {
      return NORMAL;
    }

  private:
    void addSystemDimension ();
    
  private:
    Dimension * userDimension;            // alias dimension for #_USER_
    Dimension * groupDimension;           // alias dimension for #_GROUP_
    Dimension * cubeDimension;            // alias dimension for #_CUBE_
    Dimension * configurationDimension ;  // configuration dimension for cube #_CONFIGURATION
    Dimension * dimensionDimension ;      // dimension dimension for cube #_SUBSET_* and #_VIEW_*
    Dimension * subsetDimension ;         // subset dimension for cube #_SUBSET_* and #_VIEW_*
    Dimension * viewDimension ;           // view dimension for cube #_SUBSET_* and #_VIEW_*

    Cube * groupCube;                     // cube #_GROUP_CUBE
    Cube * configurationCube;             // cube #_CONFIGURATION
    Cube * subsetLocalCube;               // cube #_SUBSET_LOCAL
    Cube * viewLocalCube;                 // cube #_VIEW_LOCAL
    Cube * subsetGlobalCube;              // cube #_SUBSET_GLOBAL
    Cube * viewGlobalCube;                // cube #_VIEW_GLOBAL
    
  };

}

#endif 
