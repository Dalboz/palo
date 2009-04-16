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

#include "Olap/RightsCube.h"

#include "Exceptions/FileFormatException.h"

#include "InputOutput/FileReader.h"

#include "Olap/CubeStorage.h"
#include "Olap/Server.h"

namespace palo {
  void RightsCube::clearCells (User* user) {
    Cube::clearCells(user);
    updateUserRights();
  }


  void RightsCube::clearCells (vector<IdentifiersType> * baseElements,
                               User * user) {
    Cube::clearCells(baseElements, user);
    updateUserRights();
  }



  semaphore_t RightsCube::setCellValue (CellPath* cellPath,
                                        double value,
                                        User* user,
                                        PaloSession * session,
                                        bool checkArea,
                                        bool sepRight,
                                        bool addValue,
                                        SplashMode splashMode,
                                        Lock * lock) {
    semaphore_t semaphore = Cube::setCellValue(cellPath, value, user, session, false, false, false, splashMode, 0);
    updateUserRights();
    return semaphore;
  }
    
    
    
  semaphore_t RightsCube::setCellValue (CellPath* cellPath, const string& value, User* user, PaloSession * session, bool checkArea, bool sepRight, Lock * lock) {
    size_t l = value.length();
    string newValue = value;
    
    if (l > 0) {
      SystemDatabase * system = dynamic_cast<SystemDatabase*>(database);
      if (system == 0 || this == system->getRoleRightObjectCube()) {

        if (l > 1) {
          throw ParameterException(ErrorException::ERROR_INVALID_PERMISSION,
                                   "value not allowed here",
                                   "value", value);
        }
  
        string okStrings = "NRWD";

        if (system) {
          const vector<Element*>* elements = cellPath->getPathElements();
          
          if (SystemDatabase::ROLE[7] == elements->at(1)->getName()) {
            okStrings = "NRWDS";
          } 
        }
        
        char valueChar = ::toupper(value[0]);
        newValue = string(1, valueChar);

        if (newValue.find_first_not_of(okStrings) != string::npos) {
          throw ParameterException(ErrorException::ERROR_INVALID_PERMISSION,
                                   "value not allowed here",
                                   "value", value);
        }
      }       
    }

    semaphore_t semaphore = Cube::setCellValue(cellPath, newValue, user, session, false, false, 0);
    updateUserRights();
    return semaphore;
  }



  semaphore_t RightsCube::clearCellValue (CellPath* cellPath, 
                                          User* user, 
                                          PaloSession * session, 
                                          bool checkArea, 
                                          bool sepRight,
                                          Lock * lock) {
    semaphore_t semaphore = Cube::clearCellValue(cellPath, user, session, false, false, lock);
    updateUserRights();
    return semaphore;
  }
  
  
  
  void RightsCube::deleteElement (const string& username,
                                  const string& event,
                                  Dimension* dimension,
                                  IdentifierType element,
                                  bool processStorageDouble,
                                  bool processStorageString,
                                  bool deleteRules) {
    Cube::deleteElement(username, event, dimension, element, processStorageDouble, processStorageString, deleteRules);
    updateUserRights();
  }
  
  
  
  void RightsCube::updateUserRights () {
    Server* server = database->getServer();
    SystemDatabase* rightsDb = server->getSystemDatabase();
    
    // database is the rights database, refresh all users
    if (rightsDb == database) {
      rightsDb->refreshUsers();
    }

    // not a rights database
    else {
      User::updateGlobalDatabaseToken(database);
    }
  }
  
  
  ////////////////////////////////////////////////////////////////////////////////
  // functions to load and save a dimension
  ////////////////////////////////////////////////////////////////////////////////

  void RightsCube::saveCubeType (FileWriter* file) {
    file->appendIdentifier(identifier);
    file->appendEscapeString(name);

    IdentifiersType identifiers;

    for (vector<Dimension*>::const_iterator i = dimensions.begin(); i != dimensions.end(); i++) {
      identifiers.push_back((*i)->getIdentifier());
    }

    file->appendIdentifiers(&identifiers); 
    file->appendInteger(CUBE_TYPE);
    file->appendBool(isDeletable());
    file->appendBool(isRenamable());
      
    file->nextLine();
  }



  void RightsCube::saveGroupCubeCells (FileWriter* file) {
    IdentifiersType path(dimensions.size());
    size_t size; // will be set in getIndexTable
    
    file->appendComment("Description of data: ");
    file->appendComment("ID;NAME ");
    file->appendSection("GROUP");

    Dimension * groupDimension = database->lookupDimensionByName(SystemDatabase::NAME_GROUP_DIMENSION);
    vector<Element*> elements = groupDimension->getElements(0);

    for (vector<Element*>::iterator iter = elements.begin();  iter != elements.end();  iter++) {
      Element * element = *iter;

      if (element != 0) {
        IdentifierType id = element->getIdentifier();

        file->appendInteger(id);
        file->appendEscapeString(element->getName());
        file->nextLine();
      }
    }

    // there are no numeric values in the rights cube
    file->appendComment("Description of data: ");
    file->appendComment("PATH;VALUE ");
    file->appendSection("STRING");

    for (uint8_t * const * table = storageString->getArray(size);  0 < size;  table++) {
      if (*table != 0) {
        char ** c = (char**) *table;

        storageString->fillPath(*table, &path);

        file->appendIdentifiers(&path);
        file->appendEscapeString(*c);
        file->nextLine();

        size--;
      }
    }
  }


  void RightsCube::saveCube () {
    if (fileName == 0) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "cube file name not set");
    }
     
    database->saveDatabase();

    if (status == LOADED) {
      return;
    }

    // open a new temp-cube file
    FileWriter fw(FileName(*fileName, "tmp"), false);
    fw.openFile();
    
    // save overview
    saveCubeOverview(&fw);

    // and dimensions
    saveCubeDimensions(&fw);

    // check for group dimension
    Dimension * groupDimension = database->lookupDimensionByName(SystemDatabase::NAME_GROUP_DIMENSION);

    // and save cells values
    if (! dimensions.empty() && dimensions[0] == groupDimension) {
      saveGroupCubeCells(&fw);
    }
    else {
      saveCubeCells(&fw);
    }

    // that's it
    fw.appendComment("");
    fw.appendComment("PALO CUBE DATA END");
    fw.closeFile();

    // delete journal files
    bool journalOpen = false;

    if (journal != 0) {
      closeJournal();
      journalOpen = false;
    }

    // archive journal
    JournalFileWriter::archiveJournalFiles(FileName(*fileName, "log"));
    // remove old cube file
    FileUtils::remove(*fileName);
    // delete journal
    JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"), false);
    // rename temp-cube file
    FileUtils::rename(FileName(*fileName, "tmp"), *fileName);

    // reopen journal
    if (journalOpen) {
      openJournal();
    }

    // the cube is now loaded
    status = LOADED;
  }


    
  void RightsCube::loadGroupCubeCells (FileReader* file) {

    // remove old storage objects
    delete storageDouble;
    delete storageString;

    // create new storage objects
    storageDouble = new CubeStorage(this, &sizeDimensions, sizeof(double), false);
    storageString = new CubeStorage(this, &sizeDimensions, sizeof(char*), true);

    // load mapping table for group
    map<IdentifierType, IdentifierType> mapping;

    Dimension * groupDimension = database->lookupDimensionByName(SystemDatabase::NAME_GROUP_DIMENSION);

    if (file->isSectionLine() && file->getSection() == "GROUP") {
      file->nextLine();

      while (file->isDataLine()) {
        IdentifierType id = file->getDataInteger(0);
        string name = file->getDataString(1);
        Element * groupElement = groupDimension->lookupElementByName(name);
        
        if (groupElement != 0) {
          mapping[id] = groupElement->getIdentifier();
        }
      
        file->nextLine();
      }
    }
    else {
      vector<Element*> elements = groupDimension->getElements(0);

      for (vector<Element*>::iterator iter = elements.begin();  iter != elements.end();  iter++) {
        Element * element = *iter;
      
        if (element != 0) {
          IdentifierType id = element->getIdentifier();
      
          mapping[id] = id;
        }
      }
    }

    // there are no numeric entries in a rights cube
    if (file->isSectionLine() && file->getSection() == "NUMERIC") {
      file->nextLine();

      while (file->isDataLine()) {
        file->nextLine();
      }
    }
    
    
    if (file->isSectionLine() && file->getSection() == "STRING") {
      file->nextLine();

      while (file->isDataLine()) {
        IdentifiersType ids = file->getDataIdentifiers(0);

        // convert group id to system group id
        IdentifierType groupId = ids[0];
      
        map<IdentifierType, IdentifierType>::iterator iter = mapping.find(groupId);
      
        // set cell value
        if (iter != mapping.end()) {
          ids[0] = groupId;
      
          setBaseCellValue(&ids, file->getDataString(1));
        }

        file->nextLine();
      }
    }
    else {
      throw FileFormatException("section 'STRING' not found", file);
    }
  }



  void RightsCube::loadCube (bool processJournal) {
    if (status == LOADED) {
      return;
    }

    if (fileName == 0) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "cube file name not set");
    }
     
    updateToken();
    
    FileReader fr(*fileName);
    fr.openFile();
        
    // load overview
    loadCubeOverview(&fr);

    // check for group dimension
    Dimension * groupDimension = database->lookupDimensionByName(SystemDatabase::NAME_GROUP_DIMENSION);

    // and cell values
    if (! dimensions.empty() && dimensions[0] == groupDimension) {
      loadGroupCubeCells(&fr);
    }
    else {
      loadCubeCells(&fr);
    }

    // process journal entries
    if (processJournal) {
      processCubeJournal(status);
    }

    // the cube is now loaded
    status = LOADED;
  }
  
  


  void RightsCube::checkPathAccessRight (User* user, const CellPath* cellPath, RightsType minimumRight) {
    if (user == 0) {
      return;
    }
    
    
    // check role "rights" right
    if (user->getRoleRightsRight() < minimumRight) {
      throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                             "insufficient access rights for rights cubes",
                             "user", (int) user->getIdentifier());
    }

    SystemDatabase * system = dynamic_cast<SystemDatabase*>(database);
    if (system != 0) {
      
      // special check for user and group "admin"
      if (this == system->getUserGroupCube() ||
          this == system->getGroupRoleCube()) {       

        if ((cellPath->getPathElements())->at(0)->getName() == "admin" && 
            (cellPath->getPathElements())->at(1)->getName() == "admin" && 
            RIGHT_READ < minimumRight) {
          throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                             "insufficient access rights for path",
                             "user", (int) user->getIdentifier());
        }
      }
      // special check for password
      else if (this == system->getRoleRightObjectCube()) {
        if ((cellPath->getPathElements())->at(0)->getName() == "admin" && 
            RIGHT_READ < minimumRight) {
          throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                             "insufficient access rights for path",
                             "user", (int) user->getIdentifier());
        }
      }
      // special check for password
      else if (this == system->getUserUserPropertiesCube() ) {
        vector<Dimension*>::iterator dim = dimensions.begin();
        vector<Element*>::const_iterator element = (cellPath->getPathElements())->begin();    
        for (; dim != dimensions.end(); dim++, element++) {      
  
          if ((*element)->getName() == "password" && user->getRolePasswordRight() < minimumRight) {
            throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                               "insufficient access rights for path",
                               "user", (int) user->getIdentifier());
          }
        }        
      }
    }
  }
  
}
