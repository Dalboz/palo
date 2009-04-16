////////////////////////////////////////////////////////////////////////////////
/// @brief palo rights dimension
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

#ifndef OLAP_RIGHTS_DIMENSION_H
#define OLAP_RIGHTS_DIMENSION_H 1

#include "palo.h"

#include "Olap/SystemDimension.h"
#include "Olap/SystemDatabase.h"
#include "Olap/Server.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief rights OLAP dimension
  ///
  /// An OLAP dimension is an ordered list of elements
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS RightsDimension : public SystemDimension {
  public:
    static const uint32_t DIMENSION_TYPE = 2;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates new dimension with given identifier
    ////////////////////////////////////////////////////////////////////////////////

    RightsDimension (IdentifierType identifier, const string& name, Database* database)
      : SystemDimension(identifier, name, database) {
    }
    
  public:
    void saveDimensionType (FileWriter* file);

  public:
    Element* addElement (const string& name, Element::ElementType elementType, User* user, bool useJournal = true);

    void deleteElement (Element * element, User* user, bool useJournal = true);

    void changeElementName (Element * element, const string& name, User* user);

    void changeElementType (Element * element, Element::ElementType elementType, User* user, bool setConsolidated);

    void clearElements (User* user);
    
  public:
    SystemDimensionType getSubType () const {
      return RIGHTS_DIMENSION;
    }

  protected:

    void checkCubeAccessRight (User* user, RightsType minimumRight) {
      if (user != 0 && user->getRoleRightsRight() < minimumRight) {
        throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                 "insufficient access rights for rights cube",
                                 "user", (int) user->getIdentifier());
      }
    }

    
    void checkElementAccessRight (User* user, RightsType minimumRight) {
      
      SystemDatabase* system = (database->getServer())->getSystemDatabase();
      if (user == 0 || system == 0) {
        return;
      }
      
      RightsType rt;
      if (this == system->getUserDimension()) {
        rt = user->getRoleUserRight();
        if (rt == RIGHT_NONE && user->getRoleSubSetViewRight() > RIGHT_NONE) {
          rt = RIGHT_READ;
        }
      }
      else if (this == system->getGroupDimension()) {
        rt = user->getRoleGroupRight();
        if (rt == RIGHT_NONE && user->getRoleSubSetViewRight() > RIGHT_NONE) {
          rt = RIGHT_READ;
        }
      } 
      else if (this == system->getRoleDimension()) {
        rt = user->getRoleRightsRight();
      } 
      else {
        rt = user->getRoleElementRight();
      }
      
      if (rt < minimumRight) {
        throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                 "insufficient access rights for dimension element",
                                 "user", (int) user->getIdentifier());
      }
    }  
  
  };

}

#endif
