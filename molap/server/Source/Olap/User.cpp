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

#include "Olap/User.h"

#include "Olap/SystemCube.h"
#include "Olap/SystemDatabase.h"
#include "Olap/SystemDimension.h"
#include "Olap/Cube.h"

namespace palo {
  vector<uint32_t> User::globalDatabaseToken; // database


  
  User::User (SystemDatabase* db, Element* user) {
    userElement    = user;
    systemDatabase = db;

    userRight           = RIGHT_NONE;
    passwordRight       = RIGHT_NONE;
    groupRight          = RIGHT_NONE;
    databaseRight       = RIGHT_NONE;
    dimensionRight      = RIGHT_NONE;
    elementRight        = RIGHT_NONE;
    cubeRight           = RIGHT_NONE;
    rightsRight         = RIGHT_NONE;
    cellDataRight       = RIGHT_NONE;
    sysOpRight          = RIGHT_NONE;
    eventProcessorRight = RIGHT_NONE;
    subSetViewRight     = RIGHT_NONE;
    userInfoRight       = RIGHT_NONE;
    ruleRight           = RIGHT_NONE;

    identifier     = 0;
    isExternal     = false;

    computeRoleRights();
  }



  User::User (SystemDatabase* db, const string& name, vector<string>* groups, IdentifierType id) {
    userElement    = 0;
    systemDatabase = db;
    this->name     = name;
    this->groups.insert(this->groups.end(), groups->begin(), groups->end());

    userRight           = RIGHT_NONE;
    passwordRight       = RIGHT_NONE;
    groupRight          = RIGHT_NONE;
    databaseRight       = RIGHT_NONE;
    dimensionRight      = RIGHT_NONE;
    elementRight        = RIGHT_NONE;
    cubeRight           = RIGHT_NONE;
    rightsRight         = RIGHT_NONE;
    cellDataRight       = RIGHT_NONE;
    sysOpRight          = RIGHT_NONE;
    eventProcessorRight = RIGHT_NONE;
    subSetViewRight     = RIGHT_NONE;
    userInfoRight       = RIGHT_NONE;
    ruleRight           = RIGHT_NONE;

    identifier     = id;
    isExternal     = true;
    
    computeRoleRights();
  }



  void User::refresh () {
    cubeRights.clear();
    elementRights.clear();
    minimumDimensionRights.clear();

    userRight           = RIGHT_NONE;
    passwordRight       = RIGHT_NONE;
    groupRight          = RIGHT_NONE;
    databaseRight       = RIGHT_NONE;
    dimensionRight      = RIGHT_NONE;
    elementRight        = RIGHT_NONE;
    cubeRight           = RIGHT_NONE;
    rightsRight         = RIGHT_NONE;
    cellDataRight       = RIGHT_NONE;
    sysOpRight          = RIGHT_NONE;
    eventProcessorRight = RIGHT_NONE;
    subSetViewRight     = RIGHT_NONE;
    userInfoRight       = RIGHT_NONE;
    ruleRight           = RIGHT_NONE;

    roleRightsValid = false;

    hasRoleRights       = true;
  }
  
  
  
  void User::refresh (Element* user) {
    userElement    = user;
    refresh();    
  }
  
  
  
  bool User::canLogin () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return hasRoleRights;
  }
  
  ////////////////////////////////////////////////////////////////////////////////
  /// role rights
  ////////////////////////////////////////////////////////////////////////////////
  
  RightsType User::getRoleUserRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return userRight;
  }



  RightsType User::getRolePasswordRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return passwordRight;
  }



  RightsType User::getRoleGroupRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return groupRight;
  }



  RightsType User::getRoleDatabaseRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return databaseRight;
  }



  RightsType User::getRoleCubeRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return cubeRight;
  }



  RightsType User::getRoleDimensionRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return dimensionRight;
  }




  RightsType User::getRoleElementRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return elementRight;
  }



  RightsType User::getRoleRightsRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return rightsRight;
  }



  RightsType User::getRoleCellDataRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return cellDataRight;
  }



  RightsType User::getRoleSysOpRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return sysOpRight;
  }



  RightsType User::getRoleEventProcessorRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return eventProcessorRight;
  }



  RightsType User::getRoleSubSetViewRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return subSetViewRight;
  }



  RightsType User::getRoleUserInfoRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return userInfoRight;
  }



  RightsType User::getRoleRuleRight () {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    return ruleRight;
  }



  void User::computeRoleRights () {
    if (!isExternal && userElement == 0) {
      return;
    }

    // get all groups of user    
    vector<Element*> userGroups = getUserGroups();
    
    // get all roles of the list of groups
    vector<Element*> groupRoles;
    Dimension* role = systemDatabase->getRoleDimension();
    Cube* groupRoleCube = systemDatabase->getGroupRoleCube();
    vector<Element*> roleElements = role->getElements(0);

    for (vector<Element*>::iterator j = userGroups.begin(); j!=userGroups.end();j++) {
      for (vector<Element*>::iterator i = roleElements.begin(); i!=roleElements.end();i++) {
        vector<Element*> path;
        path.push_back(*j);
        path.push_back(*i);

        CellPath cp(groupRoleCube, &path);

        bool found;
        set< pair<Rule*, IdentifiersType> > ruleHistory;        
        Cube::CellValueType value = groupRoleCube->getCellValue(&cp, &found, 0, 0, &ruleHistory);

        if (found && value.type == Element::STRING && value.charValue.compare("1") == 0) {
          groupRoles.push_back(*i);
        }
      }
    }    

    // get rights for objects 
    Dimension* rightObject = systemDatabase->getRightsObjectDimension();
    Cube* roleRightObjectCube = systemDatabase->getRoleRightObjectCube();
    
    userRight = getRoleRightObject(roleRightObjectCube,
                                   &groupRoles, 
                                   rightObject->findElementByName(SystemDatabase::ROLE[0], (User*) 0));

    passwordRight = getRoleRightObject(roleRightObjectCube,
                                       &groupRoles, 
                                       rightObject->findElementByName(SystemDatabase::ROLE[1], (User*) 0));

    groupRight = getRoleRightObject(roleRightObjectCube,
                                    &groupRoles, 
                                    rightObject->findElementByName(SystemDatabase::ROLE[2], (User*) 0));

    databaseRight = getRoleRightObject(roleRightObjectCube,
                                       &groupRoles, 
                                       rightObject->findElementByName(SystemDatabase::ROLE[3], (User*) 0));

    cubeRight = getRoleRightObject(roleRightObjectCube,
                                   &groupRoles, 
                                   rightObject->findElementByName(SystemDatabase::ROLE[4], (User*) 0));
                        
    dimensionRight = getRoleRightObject(roleRightObjectCube,
                                        &groupRoles, 
                                        rightObject->findElementByName(SystemDatabase::ROLE[5], (User*) 0));

    elementRight = getRoleRightObject(roleRightObjectCube,
                                      &groupRoles, 
                                      rightObject->findElementByName(SystemDatabase::ROLE[6], (User*) 0));

    cellDataRight = getRoleRightObject(roleRightObjectCube,
                                       &groupRoles, 
                                       rightObject->findElementByName(SystemDatabase::ROLE[7], (User*) 0));

    rightsRight = getRoleRightObject(roleRightObjectCube,
                                     &groupRoles, 
                                     rightObject->findElementByName(SystemDatabase::ROLE[8], (User*) 0));

    sysOpRight = getRoleRightObject(roleRightObjectCube,
                                    &groupRoles, 
                                    rightObject->findElementByName(SystemDatabase::ROLE[9], (User*) 0));

    eventProcessorRight = getRoleRightObject(roleRightObjectCube,
                                             &groupRoles, 
                                             rightObject->findElementByName(SystemDatabase::ROLE[10], (User*) 0));

    subSetViewRight = getRoleRightObject(roleRightObjectCube,
                                             &groupRoles, 
                                             rightObject->findElementByName(SystemDatabase::ROLE[11], (User*) 0));

    userInfoRight = getRoleRightObject(roleRightObjectCube,
                                             &groupRoles, 
                                             rightObject->findElementByName(SystemDatabase::ROLE[12], (User*) 0));

    ruleRight = getRoleRightObject(roleRightObjectCube,
                                             &groupRoles, 
                                             rightObject->findElementByName(SystemDatabase::ROLE[13], (User*) 0));

    roleRightsValid = true;
    
    hasRoleRights =  (groupRoles.size()>0);
  }







  ////////////////////////////////////////////////////////////////////////////////
  /// data rights
  ////////////////////////////////////////////////////////////////////////////////

  RightsType User::getDimensionDataRight (Database* db, Dimension* dim, Element* element) {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    // rebuild DimensionData rights and CellData rights if database token has changed
    checkDatabaseToken(db);
    
    try {    
      return elementRights.at(db->getIdentifier()).at(dim->getIdentifier()).at(element->getIdentifier());
    }
    catch (...) {
      computeDimensionDataRights(db);
      return elementRights.at(db->getIdentifier()).at(dim->getIdentifier()).at(element->getIdentifier());
    }      
  }


  RightsType User::getMinimumDimensionDataRight (Database* db, Dimension* dim) {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    // rebuild DimensionData rights and CellData rights if database token has changed
    checkDatabaseToken(db);
    
    try {    
      return minimumDimensionRights.at(db->getIdentifier()).at(dim->getIdentifier());
    }
    catch (...) {
      computeDimensionDataRights(db);
      return minimumDimensionRights.at(db->getIdentifier()).at(dim->getIdentifier());
    }      
  }


  RightsType User::getCubeDataRight (Database* db, Cube* cube) {
    if (! roleRightsValid) {
      computeRoleRights();
    }

    // rebuild DimensionData rights and CellData rights if database token has changed
    checkDatabaseToken(db);
    
    try {    
      return cubeRights.at(db->getIdentifier()).at(cube->getIdentifier());
    }
    catch (...) {
      computeCubeDataRights(db);
      return cubeRights.at(db->getIdentifier()).at(cube->getIdentifier());
    }      
  }





  ////////////////////////////////////////////////////////////////////////////////
  /// helper methods
  ////////////////////////////////////////////////////////////////////////////////

  vector<Element*> User::getUserGroups () {
    // get all groups of user    
    vector<Element*> userGroups;
    Dimension* group = systemDatabase->getGroupDimension();

    if (isExternal) {
      for (vector<string>::iterator i = groups.begin(); i!=groups.end();i++) {
        Element* e = group->lookupElementByName(*i);

        if (e) {
          userGroups.push_back(e);
        }
        else {
          Logger::error << "group '" << *i << "' not found in group dimension" << endl; 
        }
      }
      return userGroups;     
    }
    
    if (userElement==0) {
      return userGroups;
    }
    
    Cube* userGroupCube = systemDatabase->findCubeByName(SystemDatabase::NAME_USER_GROUP_CUBE, (User*) 0);
    vector<Element*> groupElements = group->getElements(0);

    for (vector<Element*>::iterator i = groupElements.begin(); i!=groupElements.end();i++) {
      vector<Element*> path;
      path.push_back(userElement);
      path.push_back(*i);

      CellPath cp(userGroupCube, &path);

      bool found;
      set< pair<Rule*, IdentifiersType> > ruleHistory;    
      Cube::CellValueType value = userGroupCube->getCellValue(&cp, &found, 0, 0, &ruleHistory);

      if (found && value.type == Element::STRING && value.charValue.compare("1") == 0) {
        userGroups.push_back(*i);
      }
    }
    return userGroups;
  }



  RightsType User::getRoleRightObject (Cube* roleRightObjectCube, vector<Element*>* roles, Element* rightObject) {        
    RightsType result = RIGHT_NONE;
    
    for (vector<Element*>::iterator i = roles->begin(); i!=roles->end();i++) {
      vector<Element*> path;
      path.push_back(*i);
      path.push_back(rightObject);

      CellPath cp(roleRightObjectCube, &path);

      bool found;
      set< pair<Rule*, IdentifiersType> > ruleHistory;    
      Cube::CellValueType value = roleRightObjectCube->getCellValue(&cp, &found, 0, 0, &ruleHistory);

      if (found && value.type == Element::STRING) {
        RightsType right = stringToRightsType(value.charValue);
        
        if (result < right) {
          result = right;
        }
        
      }
    }
    
    // TODO: RIGHTS_SPLASH only for rigth object "cell data"
    
    return result;
  }



  
  RightsType User::computeCubeDataRight (Cube* groupCubeDataCube, vector<Element*>* userGroups, Element* cubeElement) {

    RightsType result = RIGHT_NONE;

    // loop over all groups
    for (vector<Element*>::iterator group = userGroups->begin(); group != userGroups->end(); group++) {
      vector<Element*> path;
      path.push_back(*group);
      path.push_back(cubeElement);

      CellPath cp(groupCubeDataCube, &path);

      bool found;
      set< pair<Rule*, IdentifiersType> > ruleHistory;    
      Cube::CellValueType value = groupCubeDataCube->getCellValue(&cp, &found, 0, 0, &ruleHistory);

      if (found && value.type == Element::STRING) {
        string str = value.charValue;
        
        if (str.empty()) {
          // empty is maximum right
          return RIGHT_DELETE;
        }
        else {        
          RightsType right = stringToRightsType(str);
          
          // highest right
          if (result < right) {
            result = right;
          }
        }            
      }
      else if (!found) {
        // empty is maximum right
        return RIGHT_DELETE;
      }
    }

    if (result == RIGHT_SPLASH) {
      // maximum right is RIGHT_DELETE
      return RIGHT_DELETE;
    }
    
    return result;
  }
  
  
  
  
  RightsType User::computeDimensionDataRight (Cube* groupDimensionDataCube, Element* group, Dimension* dimension, Element* element) {
    vector<Element*> path;
    path.push_back(group);
    path.push_back(element);

    CellPath cp(groupDimensionDataCube, &path);

    bool found;
    set< pair<Rule*, IdentifiersType> > ruleHistory;    
    Cube::CellValueType value = groupDimensionDataCube->getCellValue(&cp, &found, 0, 0, &ruleHistory);

    if (found && value.type == Element::STRING) {
      return stringToRightsType(value.charValue);
    }
    else {
      const Dimension::ParentsType * parents = dimension->getParents(element);
      
      if (parents->size()==0) {
        return RIGHT_DELETE;
      }
      
      RightsType result = RIGHT_NONE;

      for (Dimension::ParentsType::const_iterator iter = parents->begin();  iter != parents->end();  iter++) {
        RightsType parentRT = computeDimensionDataRight(groupDimensionDataCube, group, dimension, *iter);

        if (parentRT > result) {
          result = parentRT;
        }
      }

      return result;
    }
  }
  
  
  
  void User::computeDimensionDataRights (Database* db) {
    if (!isExternal && userElement == 0) {
      return;
    }

    //cout << "compute dimension data rights for user '" << userElement->getName() << "'" << endl;

    IdentifierType idDb = db->getIdentifier();
     
    // get all groups of the user    
    vector<Element*> userGroups = getUserGroups();

    // resize elementRights (num databases)
    if (elementRights.size() <= idDb) {
      elementRights.resize(idDb + 1);
    }
    // resize minimumDimensionRights (num databases)
    if (minimumDimensionRights.size() <= idDb) {
      minimumDimensionRights.resize(idDb + 1);
    }

    vector< vector<RightsType> >& elementRightsDb = elementRights[idDb];
    vector< RightsType >& minimumDimensionRightsDb = minimumDimensionRights[idDb];

    // loop over all dimensions
    vector<Dimension*> dimensions = db->getDimensions(0);

    for (vector<Dimension*>::iterator dimIter = dimensions.begin(); dimIter != dimensions.end(); dimIter++) {
      Dimension* dimension = *dimIter;
      IdentifierType identifierDimension = dimension->getIdentifier();

      // resize elementRightsDb (num dimensions)
      if (elementRightsDb.size() <= identifierDimension) {
        elementRightsDb.resize(identifierDimension + 1);
      }
      // resize minimumDimensionRightsDb (num dimensions)
      if (minimumDimensionRightsDb.size() <= identifierDimension) {
        minimumDimensionRightsDb.resize(identifierDimension + 1);
      }

      vector<RightsType>& elementRightsDbDim = elementRightsDb[identifierDimension];

      // system dimension
      if (dimension->getType() == SYSTEM) {
        SystemDimension * system = dynamic_cast<SystemDimension*>(dimension);

        RightsType rt = RIGHT_NONE;

        if (system == 0) {
          // something went wrong here
        }
        // system rights dimension
        else if (system->getSubType() == SystemDimension::RIGHTS_DIMENSION) {

          // #_USER_ dimension
          if (system == systemDatabase->getUserDimension()) {
            rt = userRight;
          }
          
          // #_GROUP_ dimension
          else if (system == systemDatabase->getGroupDimension()) {
            rt = groupRight;
          }
          
          // #_USER_PROPERTIES_ dimension (not changable)
          else if (system == systemDatabase->getUserPropertiesDimension()) {
            rt = passwordRight;
          }

          // #_ROLE_ dimension
          else if (system == systemDatabase->getRoleDimension()) {
            // use rights of #_GROUP_ for #_GROUP_ROLE cube and
            // use rights of #_RIGHT_OBJECT_ for #_ROLE_RIGHTS_OBJECT cube and
            rt = RIGHT_DELETE;
          }

          // #_RIGHT_OBJECT_ dimension
          else if (system == systemDatabase->getRightsObjectDimension()) {
            rt = rightsRight;
          }

          // this is currently only #_CUBE_ dimension
          else {
            rt = rightsRight;
          }

        }

        // alias dimension 
        else if (system != 0 && system->getSubType() == SystemDimension::ALIAS_DIMENSION) {
          rt = rightsRight;
        }

        // attribute dimension
        else if (system != 0 && system->getSubType() == SystemDimension::ATTRIBUTE_DIMENSION) {
          rt = elementRight; // same as role "dimension element"
        }

        else if (system != 0) {
          // something is wrong
          rt = RIGHT_NONE;
        }

        // loop over all dimension elements 
        // (special handling for system cubes)    
        vector<Element*> elements = dimension->getElements(0);
        for (vector<Element*>::iterator elementIter = elements.begin();  elementIter != elements.end();  elementIter++) {      
          Element* element = *elementIter;
          IdentifierType identifierElement = element->getIdentifier();
          
          // resize elementRights (num elements)
          if (elementRightsDbDim.size() <= identifierElement) {
            elementRightsDbDim.resize(identifierElement + 1, RIGHT_NONE);
          }

          elementRightsDbDim[identifierElement] = rt;
          //cout << "dimension ('" << dimIter->getName() << "') data right for element '" << element->getName() << "' = " << rt << endl;
        }
        
        // set minimum dimension right
        minimumDimensionRightsDb[identifierDimension] = rt; 
      }

      // user info dimension
      else if (dimension->getType() == USER_INFO) {
        // set all rights to the userInfoRight
        
        RightsType rt = userInfoRight;

        // loop over all dimension elements 
        vector<Element*> elements = dimension->getElements(0);
        for (vector<Element*>::iterator elementIter = elements.begin();  elementIter != elements.end();  elementIter++) {      
          Element* element = *elementIter;
          IdentifierType identifierElement = element->getIdentifier();
          
          // resize elementRights (num elements)
          if (elementRightsDbDim.size() <= identifierElement) {
            elementRightsDbDim.resize(identifierElement + 1, RIGHT_NONE);
          }

          elementRightsDbDim[identifierElement] = rt;
          //cout << "dimension ('" << dimIter->getName() << "') data right for element '" << element->getName() << "' = " << rt << endl;
        }
        
        // set minimum dimension right
        minimumDimensionRightsDb[identifierDimension] = rt; 
      }
      
      // normal dimension
      else {
        //SystemDimension * system = dimension->getType() == SYSTEM ? dynamic_cast<SystemDimension*>(dimension) : 0;

        // get group -> dimenension_xyz cube 
        Cube* groupDimensionDataCube = db->findCubeByName(SystemCube::PREFIX_GROUP_DIMENSION_DATA + dimension->getName(), (User*) 0);
        
        RightsType minimum = RIGHT_WRITE;
        
        // loop over all dimension elements and get right for the element from 
        // "#_GROUP_DIMENSION_DATA_<dimension_name> cube
        vector<Element*> elements = dimension->getElements(0);
        for (vector<Element*>::iterator elementIter = elements.begin();  elementIter != elements.end();  elementIter++) {      
          Element* element = *elementIter;
          IdentifierType identifierElement = element->getIdentifier();
        
          // resize elementRights (num elements)
          if (elementRightsDbDim.size() <= identifierElement) {
            elementRightsDbDim.resize(identifierElement + 1, RIGHT_NONE);
          }
  
          // normal dimension, check rights cube
          RightsType rt = RIGHT_NONE;
        
          // loop over all groups
          for (vector<Element*>::iterator group = userGroups.begin();  group != userGroups.end();  group++) {
            RightsType elementRT = computeDimensionDataRight(groupDimensionDataCube, *group, dimension, element);
             
            if (rt < elementRT) {
              rt = elementRT;
            }          
          }
            
          if (rt == RIGHT_SPLASH) {
            rt = RIGHT_WRITE;
          }
  
          if (rt < minimum) {
            minimum = rt;
          }

          elementRightsDbDim[identifierElement] = rt;
          //cout << "dimension ('" << dimIter->getName() << "') data right for element '" << element->getName() << "' = " << rt << endl;
        }

        // set minimum dimension right
        minimumDimensionRightsDb[identifierDimension] = minimum; 
      }
    }
  }



  void User::computeCubeDataRights (Database* db) {
    if (!isExternal && userElement == 0) {
      return;
    }

    //cout << "compute cube data rights for user '" << userElement->getName() << "'" << endl;

    IdentifierType idDb = db->getIdentifier();
     
    // get all groups of the user    
    vector<Element*> userGroups = getUserGroups();

    // resize cubeRights (num databases)
    if (cubeRights.size() <= idDb) {
      cubeRights.resize(idDb + 1);
    }

    vector<RightsType>& cubeRightsDb = cubeRights[idDb];

    // loop over all cubes
    vector<Cube*> cubes = db->getCubes(0);
    for (vector<Cube*>::iterator cubeIter = cubes.begin(); cubeIter != cubes.end(); cubeIter++) {
      Cube* cube = *cubeIter;
      IdentifierType identifierCube = cube->getIdentifier();

      // resize cubeRightsDb (num cubes)
      if (cubeRightsDb.size() <= identifierCube) {
        cubeRightsDb.resize(identifierCube + 1);
      }


      RightsType rt;
      SystemCube * system = cube->getType() == SYSTEM ? dynamic_cast<SystemCube*>(cube) : 0;

      // system cube
      if (system != 0) {
        // use dimension data right
        rt = RIGHT_WRITE;
      }
      
      // normal dimension, check rights cube
      else {
        Dimension* cubeDimension = db->findDimensionByName(SystemDatabase::NAME_CUBE_DIMENSION, (User*) 0);
        Cube* groupCubeDataCube = db->findCubeByName(SystemCube::GROUP_CUBE_DATA, (User*) 0);
        Element* cubeElement = cubeDimension->lookupElementByName(cube->getName());

        if (cubeElement) {
          rt = computeCubeDataRight(groupCubeDataCube, &userGroups, cubeElement);            
        }
        else {
          rt = RIGHT_NONE;
        }
      }

      cubeRightsDb[identifierCube] = rt;
      //cout << "cube ('" << cubeIter->getName() << "') data right = " << rt << endl;
    }
  }


  string User::rightsTypeToString (RightsType rt) {
    switch (rt) {
      case RIGHT_SPLASH:
        return "S";

      case RIGHT_DELETE:
        return "D";

      case RIGHT_WRITE:
        return "W";

      case RIGHT_READ:
        return "R";

      default:
        return "N";
    }
  }
    
    
    
    
  RightsType User::stringToRightsType (const string& str) {
    if (str == "S") {
      return RIGHT_SPLASH;
    }
    else if (str == "D") {
      return RIGHT_DELETE;
    }
    else if (str == "W") {
      return RIGHT_WRITE;
    }
    else if (str == "R") {
      return RIGHT_READ;
    }
    else {
      return RIGHT_NONE;
    }
  }
  
  
  
  void User::checkDatabaseToken (Database* db) {
    IdentifierType idDb = db->getIdentifier();
    
    checkDatabaseTokenSize(idDb+1);
    
    // check global token
    if (globalDatabaseToken[idDb] != databaseToken[idDb]) {
      computeCubeDataRights(db);
      computeDimensionDataRights(db);
      databaseToken[idDb] = globalDatabaseToken[idDb];
    }
  }
  
  

  void User::checkDatabaseTokenSize (size_t size) {
    // resize databaseToken and globalDatabaseToken
    if (databaseToken.size() < size) {
      databaseToken.resize(size, 0);
    }    
    if (globalDatabaseToken.size() < size) {
      globalDatabaseToken.resize(size, 1);
    }
  }
  
  
  
  
  void User::updateGlobalDatabaseToken (Database* db) {
    IdentifierType id = db->getIdentifier();
  
    if (globalDatabaseToken.size() <= id) {
      globalDatabaseToken.resize(id+1, 1);
    }
    
    globalDatabaseToken[id]++;
  } 
}
