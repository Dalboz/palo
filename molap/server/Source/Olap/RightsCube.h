////////////////////////////////////////////////////////////////////////////////
/// @brief palo rights cube
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

#ifndef OLAP_RIGHTS_CUBE_H
#define OLAP_RIGHTS_CUBE_H 1

#include "palo.h"

#include "Olap/SystemCube.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief rights OLAP cube
  ///
  /// An OLAP cube is an ordered list of elements
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS RightsCube : public SystemCube {
  public:
    static const uint32_t CUBE_TYPE = 1;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates empty cube
    ///
    /// @throws ParameterException on double or missing dimensions
    ////////////////////////////////////////////////////////////////////////////////

    RightsCube (IdentifierType identifier,
                const string& name,
                Database* database,
                vector<Dimension*>* dimensions)
      : SystemCube (identifier, name, database, dimensions) {
    }

  public:
    void saveCubeType (FileWriter* file);

  public:
    ItemType getType () const {
      return SYSTEM;
    }

    SystemCubeType getSubType () const {
      return RIGHTS_CUBE;
    }
  
    void saveCube ();

    void loadCube (bool processJournal);

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief clears all cells
    ////////////////////////////////////////////////////////////////////////////////

    void clearCells (User* user);

    void clearCells (vector<IdentifiersType> * baseElements, User * user);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets NUMERIC value to a cell
    ////////////////////////////////////////////////////////////////////////////////

    semaphore_t setCellValue (CellPath* cellPath,
                              double value,
                              User* user,
                              PaloSession * session,
                              bool checkArea, 
                              bool sepRight,
                              bool addValue,
                              SplashMode splashMode,
                              Lock * lock);
    
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
    /// @brief clears value of cell in cube
    ////////////////////////////////////////////////////////////////////////////////

    semaphore_t clearCellValue (CellPath* cellPath,
                                User* user,
                                PaloSession * session,
                                bool checkArea,
                                bool sepRight,
                                Lock * lock);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes element
    ////////////////////////////////////////////////////////////////////////////////

    void deleteElement (const string& username,
			                  const string& event,
			                  Dimension* dimension,
                        IdentifierType element, 
                        bool processStorageDouble,
                        bool processStorageString,
                        bool deleteRules);
                        
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief updates user rights
    ////////////////////////////////////////////////////////////////////////////////

    void updateUserRights ();

  protected:
  
    void checkPathAccessRight (User* user, const CellPath* cellPath, RightsType minimumRight);    

    RightsType getMinimumAccessRight (User* user) {
      return RIGHT_NONE;
    } 

  private:
    void saveGroupCubeCells (FileWriter* file);

    void loadGroupCubeCells (FileReader* file);
  };

}

#endif
