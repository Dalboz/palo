////////////////////////////////////////////////////////////////////////////////
/// @brief palo user
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

#ifndef OLAP_USER_H
#define OLAP_USER_H 1

#include "palo.h"

#include "Olap/Element.h"

namespace palo {
  class Cube;
  class Database;
  class Dimension;
  class SystemDatabase;
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief OLAP user
  ///
  /// An OLAP user stores access rights of an user
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS User {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief updates the global database token
    ////////////////////////////////////////////////////////////////////////////////

    static void updateGlobalDatabaseToken (Database* db);
    
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates empty OLAP user
    ////////////////////////////////////////////////////////////////////////////////

    User (SystemDatabase*, Element*);
    User (SystemDatabase* db, const string& name, vector<string>* groups, IdentifierType id);

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the identifier
    ////////////////////////////////////////////////////////////////////////////////

    IdentifierType getIdentifier () const {
      if (userElement) {
        return userElement->getIdentifier();
      }
      return identifier;
    }



    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the true if the user has "role rights"
    ////////////////////////////////////////////////////////////////////////////////

    bool canLogin ();
      

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// role rights
    /// @{
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "rights" access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleUserRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "rights" access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRolePasswordRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "rights" access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleGroupRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the database access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleDatabaseRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the cube access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleCubeRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the dimension access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleDimensionRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the element access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleElementRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "rights" access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleRightsRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "cell data" access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleCellDataRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "system operations" access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleSysOpRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "event processor" access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleEventProcessorRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "sub-set view" access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleSubSetViewRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "user info" access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleUserInfoRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "rule" access right from the system database
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getRoleRuleRight ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// role and data rights
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "cell data" access right from the #_GROUP_DIMENSION_DATA_<dim name> cube
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getDimensionDataRight (Database*, Dimension*, Element*);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the "cell data" access right from the #_GROUP_CUBE cube
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getCubeDataRight (Database*, Cube*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the minimum "cell data" access right from the #_GROUP_DIMENSION_DATA_<dim name> cube
    ////////////////////////////////////////////////////////////////////////////////

    RightsType getMinimumDimensionDataRight (Database*, Dimension*);



    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the user name
    ////////////////////////////////////////////////////////////////////////////////

    const string& getName () const {
      if (userElement) {
        return userElement->getName();
      }
      else {
        return name;
      }
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks for external user
    ////////////////////////////////////////////////////////////////////////////////

    bool isExternalUser () {
      return isExternal;
    }
    
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief refresh the database rights
    ////////////////////////////////////////////////////////////////////////////////

    void refresh ();
    void refresh (Element*);

  private:
    static vector<uint32_t> globalDatabaseToken; // database

  private:
    // compute all role rights from system database
    void computeRoleRights ();
    RightsType getRoleRightObject (Cube* roleRightObjectCube, vector<Element*>* roles, Element* rightObject);
    
    
    // compute "cell data" rights for a database
    void computeCubeDataRights (Database* db);
    void computeDimensionDataRights (Database* db);
    RightsType computeCubeDataRight (Cube* groupCubeDataCube, vector<Element*>* userGroups, Element* cubeElement);
    RightsType computeDimensionDataRight (Cube* groupDimensionDataCube, Element* group, Dimension*, Element* element);

    
    
    vector<Element*> getUserGroups ();
    
    string rightsTypeToString (RightsType rt);    
    RightsType stringToRightsType (const string& str);
    
    void checkDatabaseToken (Database* db);

    void checkDatabaseTokenSize (size_t size);
    
  private:
    vector<uint32_t> databaseToken; // database

  private:
    Element* userElement; // the user (element) of the user in dimension SystemDatabase::NAME_USER_DIMENSION
    SystemDatabase* systemDatabase; // the system database of the server

    RightsType userRight;
    RightsType passwordRight;
    RightsType groupRight;
    RightsType databaseRight;
    RightsType dimensionRight;
    RightsType elementRight;
    RightsType cubeRight;
    RightsType rightsRight;
    RightsType cellDataRight;
    RightsType sysOpRight;
    RightsType eventProcessorRight;
    RightsType subSetViewRight;
    RightsType userInfoRight;
    RightsType ruleRight;
  
    vector< vector<RightsType> > cubeRights; // stores the access right for each cube of all databases 
                                             // database, cube

    vector< vector< vector<RightsType> > > elementRights; // stores the access right for each element of all dimensions of all databases
                                                          // database, dimension, element

    vector< vector<RightsType> > minimumDimensionRights; // stores the minimum access right for dimensions of all databases
                                                         // database, dimension
    bool roleRightsValid;
    
    bool hasRoleRights;

    // for external users:
    IdentifierType identifier;
    string name;
    vector<string> groups;
    bool isExternal;

  };

}

#endif
