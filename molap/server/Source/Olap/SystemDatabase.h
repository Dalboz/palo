////////////////////////////////////////////////////////////////////////////////
/// @brief palo system database
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

#ifndef OLAP_SYSTEM_DATABASE_H
#define OLAP_SYSTEM_DATABASE_H 1

#include "palo.h"

#include <map>

#include "Collections/AssociativeArray.h"

#include "Olap/Database.h"
#include "Olap/User.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief OLAP system database
  ///
  /// An OLAP database consists of dimensions and cubes
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS SystemDatabase : public Database {
  public:
    static const uint32_t DATABASE_TYPE = 2;

  public:
    static const string NAME_USER_DIMENSION;
    static const string NAME_USER_PROPERTIES_DIMENSION;
    static const string NAME_GROUP_DIMENSION;
	static const string NAME_GROUP_PROPERTIES_DIMENSION;
    static const string NAME_ROLE_DIMENSION;
	static const string NAME_ROLE_PROPERTIES_DIMENSION;
    static const string NAME_RIGHT_OBJECT_DIMENSION;
    static const string NAME_CUBE_DIMENSION;
    
    static const string NAME_USER_USER_PROPERTIERS_CUBE;
	static const string NAME_GROUP_GROUP_PROPERTIES_CUBE;
	static const string NAME_ROLE_ROLE_PROPERTIES_CUBE;
	static const string NAME_USER_GROUP_CUBE;
    static const string NAME_ROLE_RIGHT_OBJECT_CUBE;
    static const string NAME_GROUP_ROLE;
    
    static const string NAME_CLIENT_CACHE_ELEMENT;
	static const string NAME_HIDE_ELEMENTS_ELEMENT;
    static const string NAME_CONFIGURATION_DIMENSION;    

    static const string NAME_DIMENSION_DIMENSION;    
    static const string NAME_SUBSET_DIMENSION;    
    static const string NAME_VIEW_DIMENSION;
            
    static const string NAME_ADMIN;
    static const string PASSWORD_ADMIN;
  
    static const string PASSWORD;
    static const string EXPIRED;
    static const string MUST_CHANGE;
    static const string EDITOR;
    static const string VIEWER;
    static const string NOTHING;
    static const string POWER_USER;
    static const string ROLE[14];  

  private:
    struct Identifier2UserDesc {
      uint32_t hash (IdentifierType key) {
        return key;
      }

      bool isEmptyElement (User * const & user) {
        return user == 0;
      }

      uint32_t hashElement (User * const & user) {
        return hash(user->getIdentifier());
      }

      uint32_t hashKey (const IdentifierType key) {
        return hash(key);
      }

      bool isEqualElementElement (User * const & left, User * const & right) {
        return left->getIdentifier() == right->getIdentifier();
      }

      bool isEqualKeyElement (const IdentifierType key, User * const & user) {
        return key == user->getIdentifier();
      }

      void clearElement (User * & user) {
        user = 0;
      }

      void deleteElement (User * user) {
        delete user;
      }
    };


  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates new database with given identifier
    ////////////////////////////////////////////////////////////////////////////////

    SystemDatabase (IdentifierType identifier, Server* server, const string& name)
      : Database(identifier, server, name), 
        identifierToUser(100, Identifier2UserDesc()) {
      deletable = false;
      renamable = false;
      extensible = false;
      useExternalUser = false;

      userDimension = 0;
      groupDimension = 0;
	  groupPropertiesDimension = 0;

      userPropertiesDimension = 0;
      roleDimension = 0;
	  rolePropertiesDimension = 0;
      rightObjectDimension = 0;    
  
      adminUserElement = 0;
      passwordElement = 0;

      userUserPropertiesCube = 0;
      userGroupCube = 0;
      roleRightObjectCube = 0;
      groupRoleCube = 0;
	  roleRolePropertiesCube = 0;
	  groupGroupPropertiesCube = 0;

    }

    ~SystemDatabase ();
    
  public:
    void saveDatabaseType (FileWriter*);

  public:
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns type of database
    ////////////////////////////////////////////////////////////////////////////////

    ItemType getType () {
      return SYSTEM;
    }

  public:
        
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks and creates system dimensions and cubes
    ////////////////////////////////////////////////////////////////////////////////

    void createSystemItems (bool forceCreate = false);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets user by name and password
    ////////////////////////////////////////////////////////////////////////////////

    User* getUser (const string& name, const string& password, bool useMD5 = true);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets user by name
    ////////////////////////////////////////////////////////////////////////////////

    User* getUser (const string& name);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets user by identifier
    ////////////////////////////////////////////////////////////////////////////////

    User* getUser (IdentifierType);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get external users by name (and groups)
    ////////////////////////////////////////////////////////////////////////////////

    User* getExternalUser (const string& name, vector<string>* groups);    
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the system group dimension  
    ////////////////////////////////////////////////////////////////////////////////

    Dimension* getGroupDimension () {
      return groupDimension;
    }

	Dimension* getGroupPropertiesDimension () {
      return groupPropertiesDimension;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the system user dimension  
    ////////////////////////////////////////////////////////////////////////////////

    Dimension* getUserDimension () {
      return userDimension;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the system user properties dimension  
    ////////////////////////////////////////////////////////////////////////////////

    Dimension* getUserPropertiesDimension() {
      return userPropertiesDimension;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the system role dimension  
    ////////////////////////////////////////////////////////////////////////////////

    Dimension* getRoleDimension() {
      return roleDimension;
    }

	Dimension* getRolePropertiesDimension() {
		return rolePropertiesDimension;
	}
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the system rights object dimension  
    ////////////////////////////////////////////////////////////////////////////////

    Dimension* getRightsObjectDimension () {
      return rightObjectDimension;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the user user-properties cube
    ////////////////////////////////////////////////////////////////////////////////

    Cube* getUserUserPropertiesCube () {
      return userUserPropertiesCube;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the user to group cube
    ////////////////////////////////////////////////////////////////////////////////

    Cube* getUserGroupCube () {
      return userGroupCube;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the group to role cube
    ////////////////////////////////////////////////////////////////////////////////

    Cube* getGroupRoleCube () {
      return groupRoleCube;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the role to right object cube
    ////////////////////////////////////////////////////////////////////////////////

    Cube* getRoleRightObjectCube () {
      return roleRightObjectCube;
    }


	Cube* getRoleRolePropertiesCube() {
		return roleRolePropertiesCube;
	}

	Cube* getGroupGroupPropertiesCube() {
		return groupGroupPropertiesCube;
	}



    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the admin user
    ////////////////////////////////////////////////////////////////////////////////

    Element* getAdminUser () {
      return adminUserElement;
    }
            
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief refreshes all user objects
    ////////////////////////////////////////////////////////////////////////////////

    void refreshUsers ();

  private:
    Dimension* checkAndCreateDimension (const string&);

    Cube* checkAndCreateCube (const string&, Dimension*, Dimension*);
    
    Element* checkAndCreateElement (Dimension*, const string&, Element::ElementType, bool forceCreate);
    void checkAndDeleteElement (Dimension*, const string&);

    void setCell (Cube*, Element*, Element*, double, bool overwrite);
    void setCell (Cube*, Element*, Element*, const string&, bool overwrite);
    
    User* createUser (Element* userElement);

    User* createExternalUser (const string&, vector<string>*);
     
  private:
    Dimension* userDimension;
    Dimension* groupDimension;
    Dimension* userPropertiesDimension;
	Dimension* rolePropertiesDimension;
	Dimension* groupPropertiesDimension;
    Dimension* roleDimension;
    Dimension* rightObjectDimension;    
  
    Element* adminUserElement;
    Element* passwordElement;

    Cube* userUserPropertiesCube;
    Cube* userGroupCube;
    Cube* roleRightObjectCube;
    Cube* groupRoleCube;
	Cube* roleRolePropertiesCube;
	Cube* groupGroupPropertiesCube;

    AssociativeArray<IdentifierType, User*, Identifier2UserDesc> identifierToUser;
    
    bool useExternalUser;
    map < string, IdentifierType > externalUser2Id;
  };

}

#endif 
