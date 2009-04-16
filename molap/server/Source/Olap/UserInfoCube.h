////////////////////////////////////////////////////////////////////////////////
/// @brief palo user info cube
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

#ifndef OLAP_USER_INFO_CUBE_H
#define OLAP_USER_INFO_CUBE_H 1

#include "palo.h"

#include "Olap/Cube.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief user info OLAP cube
  ///
  /// An OLAP cube is an ordered list of elements
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS UserInfoCube : public Cube {
  public:
    static const uint32_t CUBE_TYPE = 6;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates empty cube
    ///
    /// @throws ParameterException on double or missing dimensions
    ////////////////////////////////////////////////////////////////////////////////

    UserInfoCube (IdentifierType identifier,
                const string& name,
                Database* database,
                vector<Dimension*>* dimensions);

  public:
    void saveCubeType (FileWriter* file);

    ItemType getType() const {
      return USER_INFO;
    }
    
  protected:

    void checkCubeAccessRight(User* user, RightsType minimumRight) {
      if (user != 0 && user->getRoleUserInfoRight() < minimumRight) {
        throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                 "insufficient access rights for cube",
                                 "user", (int) user->getIdentifier());
      }
    }
    
    
    void checkPathAccessRight (User* user, const CellPath* cellPath, RightsType minimumRight) {
      if (user != 0 && user->getRoleUserInfoRight() < minimumRight) {
        throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                 "insufficient access rights for cube",
                                 "user", (int) user->getIdentifier());
      }
    }
    
    RightsType getMinimumAccessRight (User* user) {
      if (user != 0) {
        return user->getRoleUserInfoRight();
      }
      return RIGHT_DELETE;
    }
        
  };

}

#endif
