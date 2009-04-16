////////////////////////////////////////////////////////////////////////////////
/// @brief palo configuration cube
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

#ifndef OLAP_CONFIGURATION_CUBE_H
#define OLAP_CONFIGURATION_CUBE_H 1

#include "palo.h"

#include "Olap/SystemCube.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief configuration OLAP cube
  ///
  /// An OLAP cube is an ordered list of elements
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS ConfigurationCube : public SystemCube {
  public:
    static const uint32_t CUBE_TYPE = 4;

    enum ClientCacheType {
      NO_CACHE,
      NO_CACHE_WITH_RULES,
      CACHE_ALL
    };

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates empty cube
    ///
    /// @throws ParameterException on double or missing dimensions
    ////////////////////////////////////////////////////////////////////////////////

    ConfigurationCube (IdentifierType identifier,
                const string& name,
                Database* database,
                vector<Dimension*>* dimensions);

  public:
  
    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name functions dealing with cells and cell values
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief clears all cells
    ////////////////////////////////////////////////////////////////////////////////

    void clearCells (User* user);

    void clearCells (vector<IdentifiersType> * baseElements, User * user);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets STRING value to a cell
    ////////////////////////////////////////////////////////////////////////////////

    semaphore_t setCellValue (CellPath* cellPath,
                              const string& value,
                              User* user, 
                              PaloSession * session,
                              bool checkArea,
                              bool sepRight,
                              Lock * lock);
                              
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief clear value
    ////////////////////////////////////////////////////////////////////////////////

    semaphore_t clearCellValue (CellPath* cellPath, 
                                User* user, 
                                PaloSession * session, 
                                bool checkArea, 
                                bool sepRight,
                                Lock * lock);                              

    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:  
  
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get value of client cache cell and set client cache type in parent database
    ////////////////////////////////////////////////////////////////////////////////

    void updateDatabaseClientCacheType ();

	////////////////////////////////////////////////////////////////////////////////
    /// @brief get value of hide elements cell and set hide elements value in parent database
    ////////////////////////////////////////////////////////////////////////////////

    void updateDatabaseHideElements ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get type of cube
    ////////////////////////////////////////////////////////////////////////////////

    ItemType getType () const {
      return SYSTEM;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get sub-type of cube
    ////////////////////////////////////////////////////////////////////////////////

    SystemCubeType getSubType () const {
      return CONFIGURATION_CUBE;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief save cube data
    ////////////////////////////////////////////////////////////////////////////////

    void saveCubeType (FileWriter* file);
    
  private:
  
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get value of cell path and set client cache type in parent database
    /// 
    /// if the value is missing the path ist set to 'N' (= NO_CACHE)
    ////////////////////////////////////////////////////////////////////////////////

    void updateDatabaseClientCacheType (CellPath* cellPath);

	void checkDatabaseHideElementsElement();
	void updateDatabaseHideElements (CellPath* cellPath);
      
      
      
    void checkPathAccessRight (User* user, const CellPath* cellPath, RightsType minimumRight) {
      if (user != 0 && minimumRight > RIGHT_READ && user->getRoleSysOpRight() < minimumRight) {
        throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                 "insufficient access rights for configuration cube",
                                 "user", (int) user->getIdentifier());
      }
    } 
  
  
    RightsType getMinimumAccessRight (User* user) {
      if (user == 0) {
        return RIGHT_SPLASH;
      }
      
      return user->getRoleSysOpRight();
    } 
  
  };

}

#endif
