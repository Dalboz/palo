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

#include "Olap/NormalDatabase.h"

#include <iostream>

#include "Exceptions/FileFormatException.h"

#include "Olap/CubeDimension.h"
#include "Olap/ConfigurationDimension.h"
#include "Olap/DimensionDimension.h"
#include "Olap/RightsCube.h"
#include "Olap/RightsDimension.h"
#include "Olap/Server.h"
#include "Olap/SubsetViewCube.h"
#include "Olap/SubsetViewDimension.h"
#include "Olap/User.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  // constructors and destructors
  ////////////////////////////////////////////////////////////////////////////////

  NormalDatabase::NormalDatabase (IdentifierType databaseIdentifier, Server* server, const string& name)
    : Database(databaseIdentifier, server, name) {
    groupDimension = 0;
    cubeDimension = 0;
    configurationDimension = 0;
    groupCube = 0;
    configurationCube = 0;
    dimensionDimension = 0;
    subsetDimension = 0;
    viewDimension = 0;
    subsetLocalCube = 0;
    viewLocalCube = 0;
    subsetGlobalCube = 0;
    viewGlobalCube = 0;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // notification callbacks
  ////////////////////////////////////////////////////////////////////////////////

  void NormalDatabase::notifyAddDatabase () {
    if (status != UNLOADED) {
      addSystemDimension();
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // functions to load and save a dimension
  ////////////////////////////////////////////////////////////////////////////////

  void NormalDatabase::saveDatabaseType (FileWriter* file) {
    file->appendInteger(identifier);
    file->appendEscapeString(name);
    file->appendInteger(DATABASE_TYPE);
    file->appendInteger(deletable ? 1 : 0);
    file->appendInteger(renamable ? 1 : 0);
    file->appendInteger(extensible ? 1 : 0);
    file->nextLine();        
  }

  void NormalDatabase::loadDatabase () {
    if (status == LOADED) {
      return;
    }

    Database::loadDatabase();

    try {
      findDimensionByName(SystemDatabase::NAME_GROUP_DIMENSION, (User*) 0);
      findDimensionByName(SystemDatabase::NAME_CUBE_DIMENSION, (User*) 0);
      findCubeByName(SystemCube::GROUP_CUBE_DATA, (User*) 0);
      
      // call cubeDimension->notifyAddDimension() for adding 
      // attributes dimension and cube for "#_CUBE_"-cube
      if (!cubeDimension) {
        addSystemDimension();
      }
      if (cubeDimension) {
        addSystemDimension();
        CubeDimension* cd = dynamic_cast<CubeDimension*>(cubeDimension);
        if (cd) {
          AttributesDimension* ad = cd->getAttributesDimension();
          if (!ad) {
            Logger::info << "Attributes dimension for dimension '" << cd->getName() << "' not found. Create it now." << endl;
            // attribues dimension is missing 
            cubeDimension->notifyAddDimension();
          }
          cd->checkElements();
        }
        
        DimensionDimension* dd = dynamic_cast<DimensionDimension*>(dimensionDimension);
        if (dd) {
          dd->checkElements();
        }        
      }
      else {
        Logger::info << "Cubes dimension for database '" << getName() << "' not found." << endl;
      }
      
      
      if (configurationCube) {
        ConfigurationCube* cc = dynamic_cast<ConfigurationCube*>(configurationCube);
        if (cc) {
          cc->updateDatabaseClientCacheType();
		  cc->updateDatabaseHideElements();
        }        
      }      
            
    }
    catch (...) {
      unloadDatabase();
      throw;
    }
      
  }

  ////////////////////////////////////////////////////////////////////////////////
  // other stuff
  ////////////////////////////////////////////////////////////////////////////////

  void NormalDatabase::addSystemDimension () {
    Database* systemDB = server->findDatabaseByName(Server::NAME_SYSTEM_DATABASE, 0);

    // create alias dimension for #_USER_
    Dimension* sourceDimension = systemDB->findDimensionByName(SystemDatabase::NAME_USER_DIMENSION, 0);
    userDimension = lookupDimensionByName(SystemDatabase::NAME_USER_DIMENSION);

    if (userDimension == 0) {
      userDimension = addAliasDimension(SystemDatabase::NAME_USER_DIMENSION, sourceDimension);
      userDimension->setDeletable(false);
      userDimension->setRenamable(false);
      userDimension->setChangable(false);
    }

    // check system dimension
    else if (userDimension->getType() == SYSTEM) {
      // TODO check alias dimension
    }

    // upps, something is wrong
    else {
      throw FileFormatException("alias dimension '" + SystemDatabase::NAME_USER_DIMENSION + "' corrupted", 0);
    }


    // create alias dimension for #_GROUP_
    sourceDimension = systemDB->findDimensionByName(SystemDatabase::NAME_GROUP_DIMENSION, 0);
    groupDimension = lookupDimensionByName(SystemDatabase::NAME_GROUP_DIMENSION);

    if (groupDimension == 0) {
      groupDimension = addAliasDimension(SystemDatabase::NAME_GROUP_DIMENSION, sourceDimension);
      groupDimension->setDeletable(false);
      groupDimension->setRenamable(false);
      groupDimension->setChangable(false);
    }

    // check system dimension
    else if (groupDimension->getType() == SYSTEM) {
      // TODO check alias dimension
    }

    // upps, something is wrong
    else {
      throw FileFormatException("alias dimension '" + SystemDatabase::NAME_GROUP_DIMENSION + "' corrupted", 0);
    }


    // create cube dimension for #_CUBE_
    cubeDimension = lookupDimensionByName(SystemDatabase::NAME_CUBE_DIMENSION);

    if (cubeDimension == 0) {
      cubeDimension = new CubeDimension(fetchDimensionIdentifier(), SystemDatabase::NAME_CUBE_DIMENSION, this);
      cubeDimension->setDeletable(false);
      cubeDimension->setRenamable(false);
      cubeDimension->setChangable(false);
      addDimension(cubeDimension, true);
    }

    else if (cubeDimension->getType() == SYSTEM) {
      // TODO check cube dimension
    }

    // upps, something is wrong
    else {
      throw FileFormatException("cube dimension '" + SystemDatabase::NAME_CUBE_DIMENSION + "' corrupted", 0);
    }

    // create cube #_GROUP_CUBE_DATA
    groupCube = lookupCubeByName(SystemCube::GROUP_CUBE_DATA);

    if (groupCube == 0) {
      vector<Dimension*> dimensions;
      dimensions.push_back(groupDimension);
      dimensions.push_back(cubeDimension);

      IdentifierType cubeIdentifier = fetchCubeIdentifier();
      groupCube = new RightsCube(cubeIdentifier, SystemCube::GROUP_CUBE_DATA, this, &dimensions);
      groupCube->setDeletable(false);
      groupCube->setRenamable(false);

      addCube(groupCube, false);
    }

    else if (groupCube->getType() == SYSTEM) {
      // TODO check cube
    }

    // upps, something is wrong
    else {
      throw FileFormatException("group cube '" + SystemCube::GROUP_CUBE_DATA + "' corrupted", 0);
    }
    
    // create configuration dimension #_CONFIGURATION_ with client cache element
    configurationDimension = lookupDimensionByName(SystemDatabase::NAME_CONFIGURATION_DIMENSION);

    if (configurationDimension == 0) {
      configurationDimension = new ConfigurationDimension(fetchDimensionIdentifier(), SystemDatabase::NAME_CONFIGURATION_DIMENSION, this);
      configurationDimension->setDeletable(false);
      configurationDimension->setRenamable(false);
      configurationDimension->setChangable(true);
      addDimension(configurationDimension, true);
      
      configurationDimension->addElement(SystemDatabase::NAME_CLIENT_CACHE_ELEMENT, Element::STRING, 0, true);
	  configurationDimension->addElement(SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT, Element::STRING, 0, true);
    }

    // check system dimension
    else if (configurationDimension->getType() == SYSTEM) {
      // TODO check configuration dimension
    }

    // upps, something is wrong
    else {
      throw FileFormatException("configuration dimension '" + SystemDatabase::NAME_CONFIGURATION_DIMENSION + "' corrupted", 0);
    }


    // create cube #_CONFIGURATION
    configurationCube = lookupCubeByName(SystemCube::CONFIGURATION_DATA);

    if (configurationCube == 0) {
      vector<Dimension*> dimensions;
      dimensions.push_back(configurationDimension);

      configurationCube = new ConfigurationCube(fetchCubeIdentifier(), SystemCube::CONFIGURATION_DATA, this, &dimensions);
      configurationCube->setDeletable(false);
      configurationCube->setRenamable(false);

      addCube(configurationCube, false);

    }

    else if (configurationCube->getType() == SYSTEM) {
      // TODO check cube
    }

    // upps, something is wrong
    else {
      throw FileFormatException("configuration cube '" + SystemCube::CONFIGURATION_DATA + "' corrupted", 0);
    }
    

    // create dimension dimension #_DIMENSION_ 
    dimensionDimension = lookupDimensionByName(SystemDatabase::NAME_DIMENSION_DIMENSION);

    if (dimensionDimension == 0) {
      dimensionDimension = new DimensionDimension(fetchDimensionIdentifier(), SystemDatabase::NAME_DIMENSION_DIMENSION, this);
      dimensionDimension->setDeletable(false);
      dimensionDimension->setRenamable(false);
      dimensionDimension->setChangable(true);
      addDimension(dimensionDimension, true);
    }

    // check system dimension
    else if (dimensionDimension->getType() == SYSTEM) {
      // TODO check dimension dimension
    }

    // upps, something is wrong
    else {
      throw FileFormatException("dimension dimension '" + SystemDatabase::NAME_DIMENSION_DIMENSION + "' corrupted", 0);
    }

    // create subset dimension #_SUBSET_ 
    subsetDimension = lookupDimensionByName(SystemDatabase::NAME_SUBSET_DIMENSION);

    if (subsetDimension == 0) {
      subsetDimension = new SubsetViewDimension(fetchDimensionIdentifier(), SystemDatabase::NAME_SUBSET_DIMENSION, this);
      subsetDimension->setDeletable(false);
      subsetDimension->setRenamable(false);
      subsetDimension->setChangable(true);
      addDimension(subsetDimension, true);
    }

    // check system dimension
    else if (subsetDimension->getType() != SYSTEM) {
      throw FileFormatException("subset dimension '" + SystemDatabase::NAME_SUBSET_DIMENSION + "' corrupted", 0);
    }


    // create view dimension #_VIEW_ 
    viewDimension = lookupDimensionByName(SystemDatabase::NAME_VIEW_DIMENSION);

    if (viewDimension == 0) {
      viewDimension = new SubsetViewDimension(fetchDimensionIdentifier(), SystemDatabase::NAME_VIEW_DIMENSION, this);
      viewDimension->setDeletable(false);
      viewDimension->setRenamable(false);
      viewDimension->setChangable(true);
      addDimension(viewDimension, true);
    }

    // check system dimension
    else if (viewDimension->getType() != SYSTEM) {
      throw FileFormatException("view dimension '" + SystemDatabase::NAME_VIEW_DIMENSION + "' corrupted", 0);
    }


    // create cube #_SUBSET_LOCAL
    subsetLocalCube = lookupCubeByName(SystemCube::NAME_SUBSET_LOCAL_CUBE);

    if (subsetLocalCube == 0) {
      vector<Dimension*> dimensions;
      dimensions.push_back(dimensionDimension);
      dimensions.push_back(userDimension);
      dimensions.push_back(subsetDimension);

      subsetLocalCube = new SubsetViewCube(fetchCubeIdentifier(), SystemCube::NAME_SUBSET_LOCAL_CUBE, this, &dimensions);
      subsetLocalCube->setDeletable(false);
      subsetLocalCube->setRenamable(false);

      addCube(subsetLocalCube, false);

    }

    else if (subsetLocalCube->getType() != SYSTEM) {
      throw FileFormatException("subset local cube '" + SystemCube::NAME_SUBSET_LOCAL_CUBE + "' corrupted", 0);
    }
    
    

    // create cube #_SUBSET_GLOBAL
    subsetGlobalCube = lookupCubeByName(SystemCube::NAME_SUBSET_GLOBAL_CUBE);

    if (subsetGlobalCube == 0) {
      vector<Dimension*> dimensions;
      dimensions.push_back(dimensionDimension);
      dimensions.push_back(subsetDimension);

      subsetGlobalCube = new SubsetViewCube(fetchCubeIdentifier(), SystemCube::NAME_SUBSET_GLOBAL_CUBE, this, &dimensions);
      subsetGlobalCube->setDeletable(false);
      subsetGlobalCube->setRenamable(false);

      addCube(subsetGlobalCube, false);

    }

    else if (subsetLocalCube->getType() != SYSTEM) {
      throw FileFormatException("subset global cube '" + SystemCube::NAME_SUBSET_GLOBAL_CUBE + "' corrupted", 0);
    }
    
    // create cube #_VIEW_LOCAL
    viewLocalCube = lookupCubeByName(SystemCube::NAME_VIEW_LOCAL_CUBE);

    if (viewLocalCube == 0) {
      vector<Dimension*> dimensions;
      dimensions.push_back(cubeDimension);
      dimensions.push_back(userDimension);
      dimensions.push_back(viewDimension);

      viewLocalCube = new SubsetViewCube(fetchCubeIdentifier(), SystemCube::NAME_VIEW_LOCAL_CUBE, this, &dimensions);
      viewLocalCube->setDeletable(false);
      viewLocalCube->setRenamable(false);

      addCube(viewLocalCube, false);

    }

    else if (viewLocalCube->getType() != SYSTEM) {
      throw FileFormatException("view local cube '" + SystemCube::NAME_VIEW_LOCAL_CUBE + "' corrupted", 0);
    }
    
    

    // create cube #_VIEW_GLOBAL
    viewGlobalCube = lookupCubeByName(SystemCube::NAME_VIEW_GLOBAL_CUBE);

    if (viewGlobalCube == 0) {
      vector<Dimension*> dimensions;
      dimensions.push_back(cubeDimension);
      dimensions.push_back(viewDimension);

      viewGlobalCube = new SubsetViewCube(fetchCubeIdentifier(), SystemCube::NAME_VIEW_GLOBAL_CUBE, this, &dimensions);
      viewGlobalCube->setDeletable(false);
      viewGlobalCube->setRenamable(false);

      addCube(viewGlobalCube, false);

    }

    else if (viewGlobalCube->getType() != SYSTEM) {
      throw FileFormatException("view global cube '" + SystemCube::NAME_VIEW_GLOBAL_CUBE + "' corrupted", 0);
    }
    
    
  }
    
}
