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

#include "Olap/SystemDatabase.h"

#include <iostream>

#include "Collections/md5.h"

#include "Olap/RightsCube.h"
#include "Olap/RightsDimension.h"
 
namespace palo {
  const string SystemDatabase::NAME_USER_DIMENSION            = "#_USER_";
  const string SystemDatabase::NAME_USER_PROPERTIES_DIMENSION = "#_USER_PROPERTIES_";
  const string SystemDatabase::NAME_GROUP_DIMENSION           = "#_GROUP_";
  const string SystemDatabase::NAME_GROUP_PROPERTIES_DIMENSION= "#_GROUP_PROPERTIES_";
  const string SystemDatabase::NAME_ROLE_DIMENSION            = "#_ROLE_";
  const string SystemDatabase::NAME_ROLE_PROPERTIES_DIMENSION = "#_ROLE_PROPERTIES_";

  const string SystemDatabase::NAME_RIGHT_OBJECT_DIMENSION    = "#_RIGHT_OBJECT_";
  const string SystemDatabase::NAME_CUBE_DIMENSION            = "#_CUBE_";
    
  const string SystemDatabase::NAME_USER_USER_PROPERTIERS_CUBE = "#_USER_USER_PROPERTIES";
  const string SystemDatabase::NAME_GROUP_GROUP_PROPERTIES_CUBE = "#_GROUP_GROUP_PROPERTIES";
  const string SystemDatabase::NAME_ROLE_ROLE_PROPERTIES_CUBE	= "#_ROLE_ROLE_PROPERTIES";
  const string SystemDatabase::NAME_USER_GROUP_CUBE            = "#_USER_GROUP";
  const string SystemDatabase::NAME_ROLE_RIGHT_OBJECT_CUBE     = "#_ROLE_RIGHT_OBJECT";
  const string SystemDatabase::NAME_GROUP_ROLE                 = "#_GROUP_ROLE";

  const string SystemDatabase::NAME_CLIENT_CACHE_ELEMENT       = "ClientCache";
  const string SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT      = "HideElements";
  const string SystemDatabase::NAME_CONFIGURATION_DIMENSION    = "#_CONFIGURATION_";    

  const string SystemDatabase::NAME_DIMENSION_DIMENSION        = "#_DIMENSION_";    
  const string SystemDatabase::NAME_SUBSET_DIMENSION           = "#_SUBSET_";    
  const string SystemDatabase::NAME_VIEW_DIMENSION             = "#_VIEW_";
   
  const string SystemDatabase::NAME_ADMIN     = "admin";
  const string SystemDatabase::PASSWORD_ADMIN = "admin";
  
  const string SystemDatabase::PASSWORD    = "password";
  const string SystemDatabase::EXPIRED     = "expired";
  const string SystemDatabase::MUST_CHANGE = "must change";
  const string SystemDatabase::EDITOR      = "editor";
  const string SystemDatabase::VIEWER      = "viewer";
  const string SystemDatabase::POWER_USER  = "poweruser";

  const string SystemDatabase::ROLE[14] = { 
    // never change the order of the elements!
    "user", "password", "group", "database", "cube", "dimension", "dimension element", "cell data", "rights", "system operations", "event processor", "sub-set view" , "user info", "rule" 
  };  
  
  ////////////////////////////////////////////////////////////////////////////////
  // constructors and destructors
  ////////////////////////////////////////////////////////////////////////////////

  SystemDatabase::~SystemDatabase () {
    identifierToUser.clearAndDelete();
  }
  
  ////////////////////////////////////////////////////////////////////////////////
  // functions to load and save a dimension
  ////////////////////////////////////////////////////////////////////////////////

  void SystemDatabase::saveDatabaseType (FileWriter* file) {
    file->appendInteger(identifier);
    file->appendEscapeString(name);
    file->appendInteger(DATABASE_TYPE);
    file->appendInteger(deletable ? 1 : 0);
    file->appendInteger(renamable ? 1 : 0);
    file->appendInteger(extensible ? 1 : 0);
    file->nextLine();        
  }

  ////////////////////////////////////////////////////////////////////////////////
  // other stuff
  ////////////////////////////////////////////////////////////////////////////////

  void SystemDatabase::createSystemItems (bool forceCreate) {
    extensible = true;
        
    // user dimension
    userDimension = checkAndCreateDimension(NAME_USER_DIMENSION);

    // user elements
    adminUserElement     = checkAndCreateElement(userDimension, NAME_ADMIN, Element::STRING, true);
    Element* powerUser   = checkAndCreateElement(userDimension, POWER_USER, Element::STRING, forceCreate);
    Element* editorUser  = checkAndCreateElement(userDimension, EDITOR,     Element::STRING, forceCreate);
    Element* viewerUser  = checkAndCreateElement(userDimension, VIEWER,     Element::STRING, forceCreate);

    if (forceCreate && (adminUserElement == 0 || powerUser == 0 || editorUser == 0 || viewerUser == 0)) {
      throw ErrorException(ErrorException::ERROR_INTERNAL, "system database corrupted");
    }

    // user properties dimension
    userPropertiesDimension = checkAndCreateDimension(NAME_USER_PROPERTIES_DIMENSION);
    // allowed changing of the user properties dimension 
	userPropertiesDimension->setChangable(true);

    // user properties elements
    passwordElement = checkAndCreateElement(userPropertiesDimension, PASSWORD, Element::STRING, true);
    checkAndDeleteElement(userPropertiesDimension, EXPIRED);
    checkAndDeleteElement(userPropertiesDimension, MUST_CHANGE);
      
    // group dimension
    groupDimension = checkAndCreateDimension(NAME_GROUP_DIMENSION);	

    // group elements
    Element* adminGroupElement = checkAndCreateElement(groupDimension, NAME_ADMIN, Element::STRING, true);
    Element* poweruserGroup    = checkAndCreateElement(groupDimension, POWER_USER, Element::STRING, forceCreate);
    Element* editorGroup       = checkAndCreateElement(groupDimension, EDITOR,     Element::STRING, forceCreate);
    Element* viewerGroup       = checkAndCreateElement(groupDimension, VIEWER,     Element::STRING, forceCreate);
      
    if (forceCreate && (adminGroupElement == 0 || poweruserGroup == 0 || editorGroup == 0 || viewerGroup == 0)) {
      throw ErrorException(ErrorException::ERROR_INTERNAL, "system database corrupted");
    }

    // role dimension
    roleDimension = checkAndCreateDimension(NAME_ROLE_DIMENSION);

    // role elements
    Element* adminRoleElement = checkAndCreateElement(roleDimension, NAME_ADMIN, Element::STRING, true);
    Element* poweruserRole    = checkAndCreateElement(roleDimension, POWER_USER, Element::STRING, forceCreate);
    Element* editorRole       = checkAndCreateElement(roleDimension, EDITOR,     Element::STRING, forceCreate);
    Element* viewerRole       = checkAndCreateElement(roleDimension, VIEWER,     Element::STRING, forceCreate);
      
    if (forceCreate && (adminRoleElement == 0 || poweruserRole == 0 || editorRole == 0 || viewerRole == 0)) {
      throw ErrorException(ErrorException::ERROR_INTERNAL, "system database corrupted");
    }

    // right objects dimension
    rightObjectDimension = checkAndCreateDimension(NAME_RIGHT_OBJECT_DIMENSION);
    rightObjectDimension->setChangable(false);

    // right objects elements
    Element* rightObjects[sizeof(ROLE) / sizeof(ROLE[0])];

    for (size_t i = 0; i < sizeof(ROLE) / sizeof(ROLE[0]); i++) {
      rightObjects[i] = checkAndCreateElement(rightObjectDimension, ROLE[i], Element::STRING, true);
    }
    
    // setting up some cubes
    userUserPropertiesCube = checkAndCreateCube(NAME_USER_USER_PROPERTIERS_CUBE, userDimension, userPropertiesDimension);

                           setCell(userUserPropertiesCube, adminUserElement, passwordElement, PASSWORD_ADMIN, false);
    if (powerUser  != 0) { setCell(userUserPropertiesCube, powerUser,        passwordElement, POWER_USER,     false); }
    if (editorUser != 0) { setCell(userUserPropertiesCube, editorUser,       passwordElement, EDITOR,         false); }
    if (viewerUser != 0) { setCell(userUserPropertiesCube, viewerUser,       passwordElement, VIEWER,         false); }

    userGroupCube = checkAndCreateCube(NAME_USER_GROUP_CUBE, userDimension, groupDimension);

                                                  setCell(userGroupCube, adminUserElement, adminGroupElement, "1", true);
    if (powerUser != 0  && poweruserGroup != 0) { setCell(userGroupCube, powerUser,        poweruserGroup,    "1", false); }
    if (editorUser != 0 && editorGroup != 0)    { setCell(userGroupCube, editorUser,       editorGroup,       "1", false); }
    if (viewerUser != 0 && viewerGroup != 0)    { setCell(userGroupCube, viewerUser,       viewerGroup,       "1", false); }

    roleRightObjectCube = checkAndCreateCube(NAME_ROLE_RIGHT_OBJECT_CUBE, roleDimension, rightObjectDimension);

    string     adminDefault[sizeof(ROLE) / sizeof(ROLE[0])] = {"D", "D", "D", "D", "D", "D", "D", "S", "D", "D", "D", "D", "D", "D"};
    string poweruserDefault[sizeof(ROLE) / sizeof(ROLE[0])] = {"R", "N", "R", "R", "D", "D", "D", "S", "R", "W", "N", "D", "D", "D"};
    string    editorDefault[sizeof(ROLE) / sizeof(ROLE[0])] = {"N", "N", "N", "R", "R", "R", "R", "W", "N", "N", "N", "W", "W", "W"};
    string    viewerDefault[sizeof(ROLE) / sizeof(ROLE[0])] = {"N", "N", "N", "R", "R", "R", "R", "R", "N", "N", "N", "R", "R", "R"};

    for (size_t i = 0;  i < sizeof(ROLE) / sizeof(ROLE[0]);  i++) {
                                setCell(roleRightObjectCube, adminRoleElement, rightObjects[i], adminDefault[i],     true);
      if (poweruserRole != 0) { setCell(roleRightObjectCube, poweruserRole,    rightObjects[i], poweruserDefault[i], false); }
      if (editorRole != 0)    { setCell(roleRightObjectCube, editorRole,       rightObjects[i], editorDefault[i],    false); }
      if (viewerRole != 0)    { setCell(roleRightObjectCube, viewerRole,       rightObjects[i], viewerDefault[i],    false); }
    }
            
    groupRoleCube = checkAndCreateCube(NAME_GROUP_ROLE, groupDimension, roleDimension);

                                                     setCell(groupRoleCube, adminGroupElement, adminRoleElement, "1", true);
    if (poweruserGroup != 0 && poweruserRole != 0) { setCell(groupRoleCube, poweruserGroup,    poweruserRole,    "1", false); }
    if (editorGroup != 0 && editorRole != 0)       { setCell(groupRoleCube, editorGroup,       editorRole,       "1", false); }
    if (viewerGroup != 0 && viewerRole != 0)       { setCell(groupRoleCube, viewerGroup,       viewerRole,       "1", false); }

	//more dimensions
	groupPropertiesDimension = checkAndCreateDimension(NAME_GROUP_PROPERTIES_DIMENSION);
	rolePropertiesDimension = checkAndCreateDimension(NAME_ROLE_PROPERTIES_DIMENSION);

	//more cubes
	roleRolePropertiesCube = checkAndCreateCube(NAME_ROLE_ROLE_PROPERTIES_CUBE, roleDimension, rolePropertiesDimension);
	groupGroupPropertiesCube = checkAndCreateCube(NAME_GROUP_GROUP_PROPERTIES_CUBE, groupDimension, groupPropertiesDimension);

    extensible = false;

    saveDatabase();
  }



  Dimension* SystemDatabase::checkAndCreateDimension (const string& name) {
    Dimension* dimension = lookupDimensionByName(name);

    if (dimension == 0) {
      dimension = new RightsDimension(fetchDimensionIdentifier(), name, this);
      dimension->setDeletable(false);
      dimension->setRenamable(false);
      addDimension(dimension, true);
    }

    return dimension;
  }
  
  
  
  Cube* SystemDatabase::checkAndCreateCube (const string& name, Dimension* d1, Dimension* d2) {
    Cube* cube = lookupCubeByName(name);    
    
    if (cube) {
      if (cube->getStatus() == Cube::UNLOADED) {
        cube->loadCube(true);
      }
      return cube;
    }    

    vector<Dimension*> dims;
    dims.push_back(d1);      
    dims.push_back(d2);      

    // use new identifier
    IdentifierType identifier = fetchCubeIdentifier();
    
    // create new cube and add cube to cube vector
    cube = new RightsCube(identifier, name, this, &dims);
    cube->setDeletable(false);
    cube->setRenamable(false);

    // and add cube to structure
    addCube(cube, true);

    // return new rights cube
    return cube;
  }
  
  
  
  Element* SystemDatabase::checkAndCreateElement (Dimension* dimension, const string& name, Element::ElementType type, bool forceCreate) {
    Element* element = dimension->lookupElementByName(name);

    if (element == 0 && forceCreate) {
      element = dimension->addElement(name, type, 0);
    }
    else if (element != 0 && element->getElementType() != type) {
      dimension->changeElementType(element, type, 0, true);
    }

    return element;   
  }



  void SystemDatabase::checkAndDeleteElement (Dimension* dimension, const string& name) {
    Element* element = dimension->lookupElementByName(name);

    if (element != 0) {
      dimension->deleteElement(element, 0);
    }
  }



  void SystemDatabase::setCell (Cube* cube, Element* e1, Element* e2, double value, bool overwrite) {
    vector<Element*> path;
    path.push_back(e1);
    path.push_back(e2);
    CellPath cp(cube, &path);

    bool found;
    set< pair<Rule*, IdentifiersType> > ruleHistory;    
    Cube::CellValueType oldValue = cube->getCellValue(&cp, &found, 0, 0, &ruleHistory);

    if (found) {
      if (oldValue.doubleValue != value && overwrite) { 
        cube->setCellValue(&cp, value, 0, 0, false, false, false, Cube::DEFAULT, 0);
      }
    }
    else {
      cube->setCellValue(&cp, value, 0, 0, false, false, false, Cube::DEFAULT, 0);
    }
  }



  void SystemDatabase::setCell (Cube* cube, Element* e1, Element* e2, const string& value, bool overwrite) {
    vector<Element*> path;
    path.push_back(e1);
    path.push_back(e2);
    CellPath cp(cube, &path);

    bool found;
    set< pair<Rule*, IdentifiersType> > ruleHistory;        
    Cube::CellValueType oldValue = cube->getCellValue(&cp, &found, 0, 0, &ruleHistory);

    if (found) {
      string s = oldValue.charValue;        
      if (s != value && overwrite) { 
        cube->setCellValue(&cp, value, 0, 0, false, false, 0);
      }
    }
    else {
      cube->setCellValue(&cp, value, 0, 0, false, false, 0);
    }
  }
  
  
  
  User* SystemDatabase::getUser (const string& name, const string& password, bool useMD5) {
    if (useMD5 && password.length() != 32) {
      throw ParameterException(ErrorException::ERROR_AUTHORIZATION_FAILED,
                               "wrong password",
                               "password", password);
    }

    Element* user = userDimension->lookupElementByName(name);

    if (user) {
      vector<Element*> path;
      path.push_back(user);
      path.push_back(passwordElement);
      CellPath cp(userUserPropertiesCube, &path);

      bool found;
      set< pair<Rule*, IdentifiersType> > ruleHistory;        
      Cube::CellValueType value = userUserPropertiesCube->getCellValue(&cp, &found, 0, 0, &ruleHistory);

      if (found && value.type == Element::STRING) {

        // found password, use MD5 coding
        if (useMD5) {
        
          // check password (in md5) 
          string s = value.charValue;        

          md5_state_t state;
          md5_byte_t digest[16];
          char hex_output[16*2 + 1];

          md5_init(&state);
          md5_append(&state, (const md5_byte_t *)s.c_str(), (int)s.length());
          md5_finish(&state, digest);

          for (int di = 0; di < 16; ++di) {
            sprintf(hex_output + di * 2, "%02x", digest[di]);
          }

          if (strncasecmp(hex_output, password.c_str(),32) == 0) {
            Logger::info << "user '" << name << "' logged in" << endl; 
            User* u = identifierToUser.findKey(user->getIdentifier());

            if (u) {
              return u;
            }

            return createUser(user);
          }
          else {
            Logger::info << "password '" << password << "' of user '" << name << "' should be '" << hex_output << "'" << endl;          
          }
        }

        // found password, use plain text
        else {

          // check password (plain text) 
          string s = value.charValue;

          if (s == password) {
            Logger::info << "user '" << name << "' logged in." << endl;          
            User* u = identifierToUser.findKey(user->getIdentifier());

            if (u) {
              return u;
            }

            return createUser(user);
          }
          else {
            Logger::info << "password '" << password << "' of user '" << name << "' should be '" << s << "'" << endl;          
          }
        }
      }
      else {
        Logger::info << "no password for user '" << name << "' found." << endl;          
      }
    }

    return 0;  
  }
  


  User* SystemDatabase::getUser (const string& name) {
    Element* user = userDimension->lookupElementByName(name);

    if (user) {
      User* u = identifierToUser.findKey(user->getIdentifier());

      if (u) {
        return u;
      }

      return createUser(user);
    }

    return 0;
  }
  

  
  User* SystemDatabase::getExternalUser (const string& name, vector<string>* groups) {
    map< string, IdentifierType >::iterator user = externalUser2Id.find(name);
    
    if (user == externalUser2Id.end()) {
      return createExternalUser(name, groups);      
    }
    
    User* u = identifierToUser.findKey(user->second);

    if (u) {
      return u;
    }
    
    // TODO: internal error
    return 0;
  }
  
  
  
  
  User* SystemDatabase::getUser (IdentifierType identifier) {
    if (useExternalUser) {
      return identifierToUser.findKey(identifier);
    }      
    
    Element* user = userDimension->lookupElement(identifier);

    if (user) {
      User* u = identifierToUser.findKey(user->getIdentifier());

      if (u) {
        return u;
      }

      return createUser(user);
    }

    return 0;  
  }
  
      
  User* SystemDatabase::createUser (Element* userElement) {
    User* user = new User(this, userElement);
    identifierToUser.addElement(userElement->getIdentifier(), user);
    return user;
  }
  

  
  User* SystemDatabase::createExternalUser (const string& name, vector<string>* groups) {    
    IdentifierType newId = (IdentifierType) (identifierToUser.size() + 1);
    useExternalUser = true;
    User* user = new User(this, name, groups, newId);
    externalUser2Id[name] = newId;
    identifierToUser.addElement(newId, user);
    return user;
  }
  

  
  void SystemDatabase::refreshUsers () {
    User * const * table = identifierToUser.getTable();
    size_t capacity = identifierToUser.capacity();

    for (; 0 < capacity;  capacity--, table++) {
      User * user = *table;

      if (user != 0) {
        
        if (useExternalUser) {
          user->refresh();
        }
        else {
          Element* element = userDimension->lookupElement(user->getIdentifier());
          user->refresh(element);
        }
      }
    }
  }  
}
