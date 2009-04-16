////////////////////////////////////////////////////////////////////////////////
/// @brief palo database
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

#include "Olap/Database.h"

#include <algorithm>
#include <iostream>

#include "Exceptions/FileFormatException.h"
#include "Exceptions/FileOpenException.h"
#include "Exceptions/ParameterException.h"

#include "Collections/DeleteObject.h"
#include "Collections/StringBuffer.h"

#include "InputOutput/FileReader.h"
#include "InputOutput/FileWriter.h"
#include "InputOutput/FileUtils.h"
#include "InputOutput/JournalFileReader.h"
#include "InputOutput/Logger.h"
#include "InputOutput/Statistics.h"

#include "Olap/AliasDimension.h"
#include "Olap/ConfigurationCube.h"
#include "Olap/NormalCube.h"
#include "Olap/NormalDatabase.h"
#include "Olap/NormalDimension.h"
#include "Olap/Server.h"
#include "Olap/SystemCube.h"
#include "Olap/SystemDimension.h"
#include "Olap/SystemDatabase.h"
#include "Olap/UserInfoCube.h"
#include "Olap/UserInfoDimension.h"

namespace palo {

  const string Database::INVALID_CHARACTERS = "\\/?*:|<>"; 

  ////////////////////////////////////////////////////////////////////////////////
  // constructors and destructors
  ////////////////////////////////////////////////////////////////////////////////

  Database::Database (IdentifierType identifier, Server* server, const string& name)
    : token(rand()),
      identifier(identifier),
      name(name),
      fileName(0),
      server(server),
      journal(0),
      deletable(true),
      renamable(true),
      extensible(true),
      numDimensions(0),
      nameToDimension(1000, Name2DimensionDesc()),
      numCubes(0),
      nameToCube(1000, Name2CubeDesc()),
      cacheType(ConfigurationCube::NO_CACHE),
	  hideElements(false) {

    status=CHANGED; // fileName is always 0! Check for UNLOADED later
    
    // empty database has no filled cache
    hasCachedRules = false;
  }



  Database::~Database () {
    for_each(dimensions.begin(), dimensions.end(), DeleteObject());
    for_each(cubes.begin(), cubes.end(), DeleteObject());

    if (journal != 0) {
      delete journal;
    }

    if (fileName != 0) {
      delete fileName;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // functions to load and save a dimension
  ////////////////////////////////////////////////////////////////////////////////

  bool Database::isLoadable () {
    return fileName == 0 ? false : (FileUtils::isReadable(*fileName) || FileUtils::isReadable(FileName(*fileName, "tmp")));
  }



  Database* Database::loadDatabaseFromType (FileReader* file,
                                            Server* server,
                                            IdentifierType identifier,
                                            const string& name,
                                            int type) {
    int isDeletable  = file->getDataInteger(3,1);
    int isRenamable  = file->getDataInteger(4,1);
    int isExtensible = file->getDataInteger(5,1); 
    
    Database * database;

    switch (type) {
      case NormalDatabase::DATABASE_TYPE:
        Logger::info << "registered database '" << name << "'" << endl;
        database = new NormalDatabase(identifier, server, name);
        break;

      case SystemDatabase::DATABASE_TYPE:
        Logger::info << "registered system database '" << name << "'" << endl;
        database = new SystemDatabase(identifier, server, name);
        break;

      default:
        Logger::error << "unknown database type '" << identifier << "' for database '" << name << "' found" << endl;
        throw FileFormatException("unknown database type", file);
    }

    database->setDeletable(isDeletable != 0);
    database->setRenamable(isRenamable != 0);
    database->setExtensible(isExtensible != 0);

    return database;
  }



  void Database::loadDatabaseOverview (FileReader* file, size_t& sizeDimensions, size_t& sizeCubes) {
    if (file->isSectionLine() && file->getSection() == "DATABASE" ) {
      file->nextLine();

      if (file->isDataLine()) {
        sizeDimensions = file->getDataInteger(0);
        sizeCubes = file->getDataInteger(1);
        file->nextLine();
      }
    }
    else {
      throw FileFormatException("section 'DATABASE' not found", file);
    }
  }



  void Database::loadDatabaseDimension (FileReader* file, size_t sizeDimensions) {
    uint32_t identifier = file->getDataInteger(0);
    string name          = file->getDataString(1);

    if (identifier >= sizeDimensions) {
      Logger::error << "dimension identifier '" << identifier << "' of dimension '" << name << "' is greater or equal than maximum (" <<  sizeDimensions << ")" << endl;
      throw FileFormatException("wrong identifier for dimension", file);
    }

    uint32_t type        = file->getDataInteger(2);
    Dimension* dimension = Dimension::loadDimensionFromType(file, this, identifier, name, type);

    addDimension(dimension, false);

    file->nextLine();
  }



  void Database::loadDatabaseDimensions (FileReader* file, size_t sizeDimensions) {
    for_each(dimensions.begin(), dimensions.end(), DeleteObject());

    numDimensions = 0;
    dimensions.clear();
    freeDimensions.clear();
    nameToDimension.clear();

    // load dimension section
    if (file->isSectionLine() && file->getSection() == "DIMENSIONS" ) {
      file->nextLine();

      while (file->isDataLine()) {
        loadDatabaseDimension(file, sizeDimensions);
      }          
    }
    else {
      throw FileFormatException("section 'DIMENSIONS' not found", file);
    }

    // load dimension data into memory
    for (vector<Dimension*>::iterator i = dimensions.begin();  i < dimensions.end();  i++) {
      Dimension * dimension = *i;

      if (dimension != 0) {
        dimension->loadDimension(file);
      }
    }
  }



  Cube * Database::loadDatabaseCube (FileReader* file, size_t sizeCubes) {
    uint32_t identifier = file->getDataInteger(0);
    string name            = file->getDataString(1);

    if (identifier >= sizeCubes) {
      Logger::error << "cube identifier '" << identifier << "' of cube '" << name << "' is greater or equal than maximum (" <<  sizeCubes << ")" << endl;
      throw FileFormatException("wrong identifier for cube", file);
    }


    const vector<int> dids = file->getDataIntegers(2);
    int type               = file->getDataInteger(3);
    bool deleteable        = file->getDataBool(4);
    bool renamable         = file->getDataBool(5);
            
    vector<Dimension*> dims;

    for (vector<int>::const_iterator i = dids.begin(); i != dids.end(); i++) {
      dims.push_back(findDimension(*i, 0));
    }

    Cube* cube = Cube::loadCubeFromType(file, this, identifier, name, &dims, type);
    cube->setDeletable(deleteable);
    cube->setRenamable(renamable);

    addCube(cube, false);

    file->nextLine();

    return cube;
  }



  void Database::loadDatabaseCubes (FileReader* file, size_t sizeCubes) {
    for_each(cubes.begin(), cubes.end(), DeleteObject());

    numCubes = 0;
    cubes.clear();
    freeCubes.clear();
    nameToCube.clear();

    // load cubes section
    set<IdentifierType> knownIds;

    if (file->isSectionLine() && file->getSection() == "CUBES" ) {
      file->nextLine();

      while ( file->isDataLine() ) {                        
        Cube * cube = loadDatabaseCube(file, sizeCubes);

        knownIds.insert(cube->getIdentifier());
      }          
    }
    else {
      throw FileFormatException("section 'CUBES' not found", file);
    }

    // check for old cube files
    vector<string> files = FileUtils::listFiles(fileName->path);
    string prefix = fileName->name + "_cube_";
    string postfix = "." + fileName->extension;
    string rules = "_rules";

    for (vector<string>::iterator iter = files.begin();  iter != files.end();  iter++) {
      string name = StringUtils::tolower(*iter);

      if (name.size() >= max(prefix.size(), postfix.size())
          && name.compare(0, prefix.size(), prefix) == 0
          && name.compare(name.size() - postfix.size(), postfix.size(), postfix) == 0
          && name.find(rules) == string::npos) {
        string num = name.substr(prefix.size(), name.size() - prefix.size() - postfix.size());

        try {
          IdentifierType id = StringUtils::stringToInteger(num);

          if (knownIds.find(id) == knownIds.end()) {
            FileName cubeStrange(fileName->path, fileName->name + "_CUBE_" + StringUtils::convertToString(id), fileName->extension);
            FileName cubeDeleted(fileName->path, fileName->name + "_CUBE_" + StringUtils::convertToString(id), "corrupt");
            
            FileUtils::remove(cubeDeleted);
            FileUtils::rename(cubeStrange, cubeDeleted);

            Logger::info << "found obsolete cube file " << cubeStrange.fullPath() << ", removing" << endl;
          }
        }
        catch (const ParameterException&) {
        }
      }
    }

    // load cube data into memory
    for (vector<Cube*>::iterator i = cubes.begin();  i < cubes.end();  i++) {
      Cube * cube = *i;

      if (cube != 0) {
        cube->loadCube(false);
      }
    }

  }



  bool Database::loadDatabaseJournal (FileReader* file) {
    bool changed = false;

    {
      JournalFileReader history(FileName(*fileName, "log"));
      try {
        history.openFile();
      }
      catch (FileOpenException fe) {
        return changed;
      }

      Logger::trace << "scaning log file for database '" << name << "'" << endl;

      while (history.isDataLine()) {
        string username = history.getDataString(1);
        string event = history.getDataString(2);
        string command = history.getDataString(3);

        if (command == "DELETE_DIMENSION") {
          IdentifierType idDimension = history.getDataInteger(4);
          Dimension * dimension      = findDimension(idDimension, 0);

          dimension->setDeletable(true);
          deleteDimension(dimension, (User*) 0);

          changed = true;
        }

        else if (command == "RENAME_DIMENSION") {
          IdentifierType idDimension = history.getDataInteger(4);
          const string name          = history.getDataString(5);

          renameDimension(findDimension(idDimension, 0), name, (User*) 0);

          changed = true;
        }

        else if (command == "DELETE_CUBE") {
          IdentifierType idCube = history.getDataInteger(4);
          Cube* cube = findCube(idCube, 0);
          
          cube->setDeletable(true); 
          deleteCube(cube, 0);

          changed = true;
        }

        else if (command == "RENAME_CUBE") {
          IdentifierType idCube = history.getDataInteger(4);
          const string name     = history.getDataString(5);

          renameCube(findCube(idCube, 0), name, (User*) 0);

          changed = true;
        }

        else if (command == "CLEAR_ELEMENTS") {
          IdentifierType idDimension = history.getDataInteger(4);

          findDimension(idDimension, 0)->clearElements(0);

          changed = true;
        }

        else if (command == "MOVE_ELEMENT") {
          IdentifierType idDimension = history.getDataInteger(4);
          IdentifierType idElement   = history.getDataInteger(5);
          IdentifierType position    = history.getDataInteger(6);
          Dimension * dimension      = findDimension(idDimension, 0);

          dimension->moveElement(dimension->findElement(idElement, 0), position, 0);

          changed = true;
        }

        else if (command == "ADD_ELEMENT") {
          IdentifierType idDimension = history.getDataInteger(4);
          IdentifierType idElement   = history.getDataInteger(5);
          const string name          = history.getDataString(6);
          Element::ElementType type  = (Element::ElementType) history.getDataInteger(7);
          Dimension * dimension      = findDimension(idDimension, 0);
          Element * newElement       = dimension->addElement(name, type, 0);

          if (newElement->getIdentifier() != idElement) {
            Logger::error << "element identifier '" << newElement->getIdentifier() << "' of element '" << name << "' is not equal to '" <<  idElement << "'" << endl;
            throw FileFormatException("element identifier corrupted", file);
          }

          changed = true;
        }

        else if (command == "RENAME_ELEMENT") {
          IdentifierType idDimension = history.getDataInteger(4);
          IdentifierType idElement   = history.getDataInteger(5);
          const string name          = history.getDataString(6);
          Dimension * dimension      = findDimension(idDimension, 0);
          Element * element          = dimension->findElement(idElement, 0);

          dimension->changeElementName(element, name, 0);

          changed = true;
        }

        else if (command == "ADD_CHILDREN") {
          IdentifierType idDimension = history.getDataInteger(4);
          IdentifierType idElement   = history.getDataInteger(5);
          IdentifiersType children   = history.getDataIdentifiers(6);
          vector<double> weights     = history.getDataDoubles(7);
          Dimension * dimension      = findDimension(idDimension, 0);

          if (children.size() != weights.size()) {
            throw FileFormatException("children weights corrupted", file);
          }

          size_t len = children.size();
          ElementsWeightType ew(len);

          for (size_t i = 0;  i < len;  i++) {
            ew[i].first  = dimension->findElement(children[i], 0);
            ew[i].second = weights[i];
          }

          dimension->addChildren(dimension->findElement(idElement, 0), &ew, 0);

          changed = true;
        }

        else if (command == "CHANGE_ELEMENT") {
          IdentifierType idDimension = history.getDataInteger(4);
          IdentifierType idElement   = history.getDataInteger(5);
          Element::ElementType type  = (Element::ElementType) history.getDataInteger(6);
          Dimension * dimension      = findDimension(idDimension, 0);

          dimension->changeElementType(dimension->findElement(idElement, 0), type, 0, false);

          changed = true;
        }

        else if (command == "DELETE_ELEMENT") {
          IdentifierType idDimension = history.getDataInteger(4);
          IdentifierType idElement   = history.getDataInteger(5);
          Dimension * dimension      = findDimension(idDimension, 0);

          dimension->deleteElement(dimension->findElement(idElement, 0), 0);

          changed = true;
        } else if (command == "DELETE_ELEMENTS")
		{
			IdentifierType idDimension = history.getDataInteger(4);
			Dimension * dimension      = findDimension(idDimension, 0);
			std::vector<Element*> elements;
			IdentifiersType ids = history.getDataIdentifiers(5);
			for (size_t i = 0; i<ids.size(); i++) {
				Element* element = dimension->findElement(ids[i], 0);
				if (element!=0)
					elements.push_back(element);
			}
			dimension->deleteElements(elements, 0);
			changed = true;

		} else if (command == "REMOVE_CHILDREN") {
          IdentifierType idDimension = history.getDataInteger(4);
          IdentifierType idElement   = history.getDataInteger(5);
          Dimension * dimension      = findDimension(idDimension, 0);

          dimension->removeChildren(0, dimension->findElement(idElement, 0));

          changed = true;
        }

        else if (command == "REMOVE_CHILDREN_NOT_IN") {
          IdentifierType idDimension = history.getDataInteger(4);
          IdentifierType idElement   = history.getDataInteger(5);
          IdentifiersType children   = history.getDataIdentifiers(6);
          Dimension * dimension      = findDimension(idDimension, 0);

		  set<Element*> keep;

		  for (size_t i = 0;  i < children.size();  i++) {
            keep.insert(dimension->findElement(children[i], 0));
          }

          dimension->removeChildrenNotIn(0, dimension->findElement(idElement, 0), &keep);

          changed = true;
        }

        history.nextLine();
      }
    }

    // this is done when saving the database:
    // JournalFileWriter::archiveJournalFiles(FileName(*fileName, "log"));
    // JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"), false);

    return changed;
  }



  void Database::loadDatabase () {
    if (status == LOADED) {
      return;
    }

    bool useLog = status == UNLOADED;

    if (fileName == 0) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "database file name not set");
    }
     
    status = LOADING;

    updateToken();

    // close the journal
    bool journalOpen = false;

    if (journal != 0) {
      closeJournal();
      journalOpen = true;
    }

    bool tmpExtensible = extensible;
    extensible = true;

    // load database from file
    try {
      if (!FileUtils::isReadable(*fileName) && FileUtils::isReadable(FileName(*fileName, "tmp"))) {
        Logger::warning << "using temp file for database '" << name << "'" << endl;

        // delete journal
        JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"), false);

        // rename temp file
        if (! FileUtils::rename(FileName(*fileName, "tmp"), *fileName)) {
          Logger::error << "cannot rename database file: '" << strerror(errno) << "'" << endl;
          Logger::error << "please check the underlying file system for errors" << endl;
          exit(1);
        }
      } 

      bool changed = false;

      // we need to close the file again in order to save it
      {
        FileReader fr(*fileName);
        fr.openFile();

        // get overview
        size_t sizeDimensions = 0;
        size_t sizeCubes = 0;

        loadDatabaseOverview(&fr, sizeDimensions, sizeCubes);
    
        // load dimensions from file
        loadDatabaseDimensions(&fr, sizeDimensions);
      
        // load cubes from file
        loadDatabaseCubes(&fr, sizeCubes);

        // and read the journal
        if (useLog) {
          changed = loadDatabaseJournal(&fr);
        }

        // delete the journal as we have replaced the database with the saved data
        else {
          JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"), false);
        }
      }

      // force a write back to disk, this will archive the journal
      if (changed) {
        status = CHANGED;
        saveDatabase();
      }

      // process cube logfiles
      for (vector<Cube*>::iterator i = cubes.begin();  i < cubes.end();  i++) {
        Cube * cube = *i;

        if (cube != 0) {
          cube->processCubeJournal(Cube::UNLOADED);
        }
      }            

    }

    catch (...) {
      if (journalOpen) {
        openJournal();
      }
      extensible = tmpExtensible;
      Logger::error << "cannot load database '" << name << "'" << endl;
      status = UNLOADED;
      throw;
    }

    if (journalOpen) {
      openJournal();
    }

    extensible = tmpExtensible;

    // database is now loaded
    status = LOADED;
  }



  void Database::saveDatabaseOverview (FileWriter* file) {
    file->appendComment("PALO DATABASE DATA");
    file->appendComment("");
    
    file->appendComment("Description of data:");
    file->appendComment("SIZE_DIMENSIONS;SIZE_CUBES;");

    // TODO write a file version

    file->appendSection("DATABASE");
    file->appendInteger( (int32_t) this->dimensions.size() );
    file->appendInteger( (int32_t) this->cubes.size() );

    file->nextLine();
  }



  void Database::saveDatabaseDimensions (FileWriter* file) {
    file->appendComment("Description of data:");
    file->appendComment("ID;NAME;TYPE;...");
    file->appendSection("DIMENSIONS");

    vector<Dimension*> dimensions = getDimensions(0);

    for (vector<Dimension*>::iterator i = dimensions.begin(); i != dimensions.end(); i++) {
      Dimension * dimension = *i;
      dimension->saveDimensionType(file);
    }
    
    for (vector<Dimension*>::iterator i = dimensions.begin(); i != dimensions.end(); i++) {
      Dimension * dimension = *i;
      dimension->saveDimension(file);
    }
  }



  void Database::saveDatabaseCubes (FileWriter* file) {
    file->appendComment("Description of data:");
    file->appendComment("ID;NAME;DIMENSIONS;TYPE;DELETABLE;RENAMABLE;...");
    file->appendSection("CUBES" );

    vector<Cube*> cubes = getCubes(0);

    for (vector<Cube*>::iterator i = cubes.begin(); i != cubes.end(); i++) {
      Cube * cube = *i;

      cube->saveCubeType(file);
    }
  }



  void Database::saveDatabase () {
    if (status == LOADED || status == LOADING) {
      return;
    }

    server->saveServer(0);

    if (fileName == 0) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "database file name not set");
    }
    
    // open a new temp-database file
    FileWriter fw(FileName(*fileName, "tmp"), false);
    fw.openFile();

    // save database, dimension and cubes to disk
    saveDatabaseOverview(&fw);
    saveDatabaseDimensions(&fw);
    saveDatabaseCubes(&fw);

    fw.appendComment("");
    fw.appendComment("");
    fw.appendComment("PALO DATABASE DATA END");
    fw.closeFile();

    // archive journal files
    bool journalOpen = false;

    if (journal != 0) {
      closeJournal();
      journalOpen = true;
    }

    // archive journal
    JournalFileWriter::archiveJournalFiles(FileName(*fileName, "log"));

    // remove old database file
    if (FileUtils::isReadable(*fileName) && ! FileUtils::remove(*fileName)) {
      Logger::error << "cannot remove database file: '" << strerror(errno) << "'" << endl;
      Logger::error << "please check the underlying file system for errors" << endl;
      exit(1);
    }

    // delete journal
    JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"), false);

    // rename temp-database file
    if (! FileUtils::rename(FileName(*fileName, "tmp"), *fileName)) {
      Logger::error << "cannot rename database file: '" << strerror(errno) << "'" << endl;
      Logger::error << "please check the underlying file system for errors" << endl;
      exit(1);
    }

    // reopen journal
    if (journalOpen) {
      openJournal();
    }

    // database is now loaded
    setStatus(LOADED);
  }



  void Database::setDatabaseFile (const FileName& newName) {

    // new association with a database file
    if (fileName == 0) {
      fileName = new FileName(newName);

      if (FileUtils::isReadable(FileName(fileName->path, "database", "csv"))) {
        status = UNLOADED;
      }
      else {
        if (! FileUtils::createDirectory(fileName->path)) {
          Logger::error << "cannot create database directory: '" << strerror(errno) << "'" << endl;
          Logger::error << "please check the underlying file system for errors" << endl;
          exit(1);
        }

        saveDatabase();
      }

      openJournal();

      return;
    }

    // no change in database file
    if (newName.path == fileName->path) {
      return;
    }

    // close journal
    bool journalOpen = false;

    if (journal != 0) {
      closeJournal();
      journalOpen = true;
    }

    // change database file
    if (status == UNLOADED) {
      if (! FileUtils::renameDirectory(*fileName, newName)) {
        throw ParameterException(ErrorException::ERROR_RENAME_FAILED,
                                 "cannot rename database files",
                                 "name", name);
      }

      
      delete fileName;
      fileName = new FileName(newName);

      if (journalOpen) {
        openJournal();
      }
    }
    else {
      saveDatabase();

      unloadDatabase();
      
      if (! FileUtils::renameDirectory(*fileName, newName)) {
        throw ParameterException(ErrorException::ERROR_RENAME_FAILED,
                                 "cannot rename database files",
                                 "name", name);
      }

      delete fileName;
      fileName = new FileName(newName);

      if (journalOpen) {
        openJournal();
      }

      loadDatabase();
    }
  };



  void Database::deleteDatabaseFiles () {
    if (fileName == 0) {
      return;
    }

    // delete database file from disk
    if (FileUtils::isReadable(*fileName)) {
      if (status == UNLOADED) {
        loadDatabase();
      }

      FileWriter::deleteFile(*fileName);
    }

    // delete cube files of database from disk
    vector<Cube*> cubes = getCubes(0);

    for (vector<Cube*>::iterator i = cubes.begin(); i != cubes.end(); i++) {
      Cube* cube = *i;

      removeCube(cube, false);
      delete cube;
    }

    // delete journals    
    if (journal != 0) {
      delete journal;
      journal = 0;
    }

    JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"));

    // delete path
    if (! FileUtils::removeDirectory(*fileName)) {
      Logger::error << "cannot remove database directory: '" << strerror(errno) << "'" << endl;
      Logger::error << "please check the underlying file system for errors" << endl;
      exit(1);
    }
  }



  void Database::unloadDatabase () {
    if (! isLoadable()) {
      throw ParameterException(ErrorException::ERROR_DATABASE_UNSAVED,
                               "cannot unload an unsaved database, you must use delete",
                               "", "");
    }
      
    if (fileName == 0) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "database file name not set");
    }
     
    if (! FileUtils::isReadable(*fileName)) { 
      throw ParameterException(ErrorException::ERROR_CORRUPT_FILE,
                               "cannot load file",
                               "file name", fileName->fullPath());
    }
    
    updateToken();

    // delete all dimensions
    for_each(dimensions.begin(), dimensions.end(), DeleteObject());

    dimensions.clear();
    freeDimensions.clear();
    nameToDimension.clear();

    // delete all cubes
    for_each(cubes.begin(), cubes.end(), DeleteObject());

    cubes.clear();
    freeCubes.clear();
    nameToCube.clear();

    // database is now unloaded
    status = UNLOADED;
  }



  void Database::calculateMarkers () {
    for (vector<Cube*>::iterator i = cubes.begin();  i != cubes.end();  i++) {
      Cube* cube = *i;

      if (cube != 0) {
        cube->checkNewMarkerRules();
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // getter and setter
  ////////////////////////////////////////////////////////////////////////////////

  vector<Dimension*> Database::getDimensions (User* user) {
    checkDimensionAccessRight(user, RIGHT_READ);
    
    vector<Dimension*> result;
    
    for (vector<Dimension*>::iterator i = dimensions.begin();  i < dimensions.end();  i++) {
      Dimension * dimension = *i;

      if (dimension == 0) {
        continue;
      }

      if (user == 0) {
        result.push_back(dimension);
      }
      else {
        SystemDimension * system = dimension->getType() == SYSTEM ? dynamic_cast<SystemDimension*>(dimension) : 0;

        if (system) {
          switch (system->getSubType()) {
            case SystemDimension::ATTRIBUTE_DIMENSION:
                result.push_back(dimension);
                break;
                
            case SystemDimension::RIGHTS_DIMENSION:
                if (user->getRoleRightsRight() > RIGHT_NONE) {
                  result.push_back(dimension);
                }
                break;
                
            case SystemDimension::CONFIGURATION_DIMENSION:
                if (user->getRoleSysOpRight() > RIGHT_NONE) {
                  result.push_back(dimension);
                }
                break;
                
            case SystemDimension::ALIAS_DIMENSION:            
                if (user->getRoleRightsRight() > RIGHT_NONE || user->getRoleSubSetViewRight() > RIGHT_NONE) {
                  result.push_back(dimension);
                }
                break;

            default:
                if (user->getRoleRightsRight() > RIGHT_NONE || user->getRoleSubSetViewRight() > RIGHT_NONE) {
                  result.push_back(dimension);
                }
            
          }
          
        }
        else {
          if (dimension->getType() == USER_INFO) {
            // user info dimension
            if (user->getRoleUserInfoRight() > RIGHT_NONE) {
              result.push_back(dimension);
            }            
          }
          else {
            // normal dimension 
            result.push_back(dimension);
          }
        }        
      }
    }
    
    return result;
  }
  
  
  
  vector<Cube*> Database::getCubes (User* user) {
    checkCubeAccessRight(user, RIGHT_READ);

    vector<Cube*> result;
      
    for (vector<Cube*>::iterator i = cubes.begin();  i < cubes.end();  i++) {
      Cube * cube = *i;

      if (cube == 0) {
        continue;
      }

      if (user == 0) {
        result.push_back(cube);
      }
      else {
        SystemCube * system = cube->getType() == SYSTEM ? dynamic_cast<SystemCube*>(cube) : 0;
        if (system) {
          // system cube
          
          switch (system->getSubType()) {
            case SystemCube::ATTRIBUTES_CUBE:
                result.push_back(cube);
                break;
            case SystemCube::CONFIGURATION_CUBE:
                if (user->getRoleSysOpRight() > RIGHT_NONE) {
                  result.push_back(cube);
                }
            case SystemCube::SUBSET_VIEW_CUBE:
                if (user->getRoleSubSetViewRight() > RIGHT_NONE) {
                  result.push_back(cube);
                }
            default:
              if (user->getRoleRightsRight() > RIGHT_NONE) {
                result.push_back(cube);
              }
          }
        }
        else if (cube->getType() == USER_INFO) {
          // user info cube
          if (user->getRoleUserInfoRight() > RIGHT_NONE) {
            result.push_back(cube);
          }            
        }
        else {
          // normal cube
          result.push_back(cube);
        }
      }
    }
    
    return result;
  }
  
  ////////////////////////////////////////////////////////////////////////////////
  // functions to administrate dimensions
  ////////////////////////////////////////////////////////////////////////////////

  IdentifierType Database::fetchDimensionIdentifier () {
    return (IdentifierType) dimensions.size();
  }



  // this method is used internally, so no rights checking is done
  void Database::addDimension (Dimension* dimension, bool notify) {
    const string& name = dimension->getName();

    // check extensible flag
    if (! extensible) {
      throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                               "dimension cannot be added",
                               "dimension", dimension->getName());
    }

    if (lookupDimensionByName(name) != 0) {
      throw ParameterException(ErrorException::ERROR_DIMENSION_NAME_IN_USE,
                               "dimension name is already in use",
                               "name", name);
    }

    IdentifierType identifier = dimension->getIdentifier();

    if (lookupDimension(identifier) != 0) {
      throw ParameterException(ErrorException::ERROR_INTERNAL,
                               "dimension identifier is already in use",
                               "identifier", (int) identifier);
    }

    // tell dimension that it will be added
    if (notify) {
      dimension->beforeAddDimension();
    }

    // check if we have to change the free list
    if (identifier >= dimensions.size()) {
      dimensions.resize(identifier + 1, 0);

      for (size_t i = dimensions.size() - 1;  i < identifier;  i++) {
        freeDimensions.insert(i);
      }
    }

    // add dimension to mapping
    dimensions[identifier] = dimension;
    nameToDimension.addElement(name, dimension);

    // we have one more dimension
    numDimensions++;
    
    // database has been changed, update token and status
    setStatus(CHANGED);
    updateToken();

    // tell dimension that it has been added
    if (notify) {
      try {
        dimension->notifyAddDimension();
      }
      catch (...) {
        dimensions[identifier] = 0;
        nameToDimension.removeElement(dimension);
        numDimensions--;
        freeDimensions.insert(identifier);
        throw;
      }
    }

    clearRuleCaches();
  }



  // this method is used internally, so no rights checking is done
  void Database::removeDimension (Dimension* dimension, bool notify) {
    IdentifierType identifier = dimension->getIdentifier();

    if (dimension != lookupDimension(identifier)) {
      throw ParameterException(ErrorException::ERROR_DIMENSION_NOT_FOUND,
                               "dimension not found", 
                               "dimension", (int) identifier);
    }
    
    // is dimension used in a normal cube?
    for (vector<Cube*>::iterator i = cubes.begin();  i != cubes.end();  i++) {
      Cube* cube = *i;

      if (cube == 0) {
        continue;
      }

      if (cube->getType() == NORMAL) {      
        const vector<Dimension*>* dims = cube->getDimensions();

        // dimension is used by cube
        if (find(dims->begin(), dims->end(), dimension) != dims->end()) {
          throw ParameterException(ErrorException::ERROR_DIMENSION_IN_USE,
                                 "dimension is used by a cube", 
                                 "dimension", (int) identifier);
        }
      }
    }

    // tell dimension that it will be removed
    if (notify) {
      // we have to delete system cubes using the dimension here
      dimension->beforeRemoveDimension();
    }

    // remove element from mapping
    nameToDimension.removeElement(dimension);
    
    // we have one dimension less
    numDimensions--;
    
    // add identifier of the dimension to freeDimensions
    freeDimensions.insert(identifier);
    
    // clear entry in dimensions
    dimensions[identifier] = 0;

    // database has been changed, update token and status
    setStatus(CHANGED);
    updateToken();

    // tell dimension that it has been removed
    if (notify) {
      dimension->notifyRemoveDimension();
    }
    
    clearRuleCaches();
  }



  // this method is used internally, so no rights checking is done
  void Database::renameDimension (Dimension* dimension, const string& name, bool notify) {

    // check for double used name
    Dimension* dimByName = lookupDimensionByName(name);

    if (dimByName != 0) {
      if (dimByName != dimension) {
        throw ParameterException(ErrorException::ERROR_DIMENSION_NAME_IN_USE,
                               "dimension name is already in use", 
                               "name", name);
      }
      
      if (dimension->getName() == name) {
        // new name = old name     
        return;
      }

    }
    
    // tell dimension that it will be renamed
    if (notify) {
      dimension->beforeRenameDimension();
    }

    // keep old name
    string oldName = dimension->getName();

    // delete old name
    nameToDimension.removeElement(dimension);
    
    // change name 
    dimension->setName(name);
    
    // add new name to dimensionNameToDimensionId
    nameToDimension.addElement(name, dimension);

    // database has been changed
    setStatus(CHANGED);
    updateToken();

    // tell dimension that it has been renamed
    if (notify) {
      dimension->notifyRenameDimension(oldName);
    }
    
    clearRuleCaches();
  }



  Dimension* Database::addDimension (const string& name, User* user, bool isUserInfo) {
    if (isUserInfo) {
      checkUserInfoAccessRight(user, RIGHT_DELETE);
    }
    else {
      checkDimensionAccessRight(user, RIGHT_WRITE);
    }

    checkDimensionName(name, isUserInfo);
    
    // create new dimension
    IdentifierType identifier = fetchDimensionIdentifier();
    Dimension* dimension;
    if (isUserInfo) {
      dimension = new UserInfoDimension(identifier, name, this);
    }
    else {
      dimension = new NormalDimension(identifier, name, this);
    }

    try {
      // and update dimension structure
      addDimension(dimension, true);
    }
    catch (...) {
      delete dimension;
      dimension = 0;
      throw;
    }

    // save dimension change
    saveDatabase();

    // log changes to journal is not necessary (see above, we already saved the database)
    // this is kept for tracking the changes to the database
    if (journal != 0) {
      journal->appendCommand(Server::getUsername(user),
                             Server::getEvent(),
                             "ADD_DIMENSION");
      journal->appendInteger(dimension->getIdentifier());
      journal->appendEscapeString(name);
      journal->nextLine();
    }

    // return dimension
    return dimension;
  }



  // this method is used internally, so no rights checking is done
  Dimension* Database::addAliasDimension (const string& name, Dimension* alias) {
    if (name.empty()) {
      throw ParameterException(ErrorException::ERROR_INVALID_DIMENSION_NAME,
                               "dimension name is empty",
                               "name", name);
    }
  
    // create new dimension
    IdentifierType identifier = fetchDimensionIdentifier();
    Dimension * dimension = new AliasDimension(identifier, name, this, alias);

    // and update dimension structure
    addDimension(dimension, true);

    // return dimension
    return dimension;
  }
  


  void Database::deleteDimension (Dimension* dimension, User* user) {
    
    if (dimension->getType() == USER_INFO) {
      checkUserInfoAccessRight(user, RIGHT_DELETE);
    }
    else {
      checkDimensionAccessRight(user, RIGHT_DELETE);
    }

    // check deletable flag
    if (! dimension->isDeletable()) {
      throw ParameterException(ErrorException::ERROR_DIMENSION_UNDELETABLE,
                               "dimension cannot be deleted",
                               "dimension", dimension->getName());
    }
    
    IdentifierType id = dimension->getIdentifier();
    
    // remove dimension from database
    removeDimension(dimension, true);

    // and finally delete dimension
    delete dimension;

    // delete dimension from name mapping
    if (journal != 0) {
      journal->appendCommand(Server::getUsername(user),
                             Server::getEvent(),
                             "DELETE_DIMENSION");
      journal->appendInteger(id);
      journal->nextLine();
    }
    
    // save database to delete dimension from database file 
    saveDatabase();    
  }



  void Database::renameDimension (Dimension* dimension, const string& name, User* user) {
    
    if (dimension->getType() == USER_INFO) {
      checkUserInfoAccessRight(user, RIGHT_WRITE);
    }
    else {
      checkDimensionAccessRight(user, RIGHT_WRITE);
    }

    checkName(name);

    // check renamable flag
    if (! dimension->isRenamable()) {
      throw ParameterException(ErrorException::ERROR_DIMENSION_UNRENAMABLE,
                               "dimension cannot be renamed",
                               "dimension", dimension->getName());
    }
    
    if (name.empty()) {
      throw ParameterException(ErrorException::ERROR_INVALID_DIMENSION_NAME,
                               "dimension name is empty",
                               "name", name);
    }
    
    if (dimension->getType() == USER_INFO) {
      checkDimensionName(name, true);
    }
    else {
      checkDimensionName(name, false);
    }
    
    // rename dimension in database
    renameDimension(dimension, name, true);

    if (journal != 0) {
      journal->appendCommand(Server::getUsername(user),
                             Server::getEvent(),
                             "RENAME_DIMENSION");
      journal->appendInteger(dimension->getIdentifier());
      journal->appendEscapeString(name);
      journal->nextLine();
    }
  }
  
  ////////////////////////////////////////////////////////////////////////////////
  // functions to administrate dimensions
  ////////////////////////////////////////////////////////////////////////////////

  IdentifierType Database::fetchCubeIdentifier () {
    return (IdentifierType) cubes.size();
  }



  FileName Database::computeCubeFileName (const FileName& fileName, IdentifierType cubeIdentifier) {
    return FileName(fileName.path, fileName.name + "_CUBE_" + StringUtils::convertToString(cubeIdentifier), fileName.extension);
  }



  // this method is used internally, so no rights checking is done
  void Database::addCube (Cube* cube, bool notify) {
    const string& name = cube->getName();

    // check extensible flag
    if (! extensible) {
      throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                               "cube cannot be added",
                               "cube", cube->getName());
    }

    if (lookupCubeByName(name) != 0) {
      throw ParameterException(ErrorException::ERROR_CUBE_NAME_IN_USE,
                               "cube name is already in use",
                               "name", name);
    }      

    IdentifierType identifier = cube->getIdentifier();

    if (lookupCube(identifier) != 0) {
      throw ParameterException(ErrorException::ERROR_INTERNAL,
                               "cube identifier is already in use",
                               "identifier", (int) identifier);
    }      

    // check if we have to change the free list
    if (identifier >= cubes.size()) {
      cubes.resize(identifier + 1, 0);

      for (size_t i = cubes.size() - 1;  i < identifier;  i++) {
        freeCubes.insert(i);
      }
    }

    // add cube to mapping
    cubes[identifier] = cube;
    nameToCube.addElement(name, cube);

    // we have one more dimension
    numCubes++;

    // update filename
    cube->setCubeFile(computeCubeFileName(*fileName, identifier));

    // database has been changed, update token and status
    setStatus(CHANGED);
    updateToken();

    // tell cube that it has been added
    if (notify) {
      try {
        cube->notifyAddCube();
      }
      catch (...) {
        cubes[identifier] = 0;
        nameToCube.removeElement(cube);
        numCubes--;
        throw;
      }
    }
  }



  // this method is used internally, so no rights checking is done
  void Database::removeCube (Cube* cube, bool notify) {
    IdentifierType id = cube->getIdentifier();
    
    // check that cube is cube of database
    if (cube != lookupCube(id)) {
      throw ParameterException(ErrorException::ERROR_CUBE_NOT_FOUND,
                               "cube not found",
                               "cube", (int) id);
    }

    // delete cube file from disk
    cube->deleteCubeFiles();
        
    // add identifier of the cube to freeCubes
    freeCubes.insert(cube->getIdentifier());
    
    // delete cube from hash_map nameToCube
    nameToCube.removeElement(cube);
    
    // we have one cube less
    numCubes--;
    
    // remove cube from list of cubes
    cubes[id] = 0;

    // database has been changed, update token and status
    setStatus(CHANGED);
    updateToken();

    // tell cube that it has been removed
    if (notify) {
      cube->notifyRemoveCube();
    }
    
    clearRuleCaches();
  }



  // this method is used internally, so no rights checking is done
  void Database::renameCube (Cube* cube, const string& name, bool notify) {
    checkName(name);

    // check for double used name
    Cube* cubeByName = lookupCubeByName(name);

    if (cubeByName != 0) {
      if (cubeByName != cube) {
        throw ParameterException(ErrorException::ERROR_CUBE_NAME_IN_USE,
                               "cube name is already in use",
                               "name", name);
      }        

      if (cube->getName() == name) {
        // new name == old name
        return;
      }

    }
    
    string oldName = cube->getName();

    // delete old name
    nameToCube.removeElement(cube);
    
    // change name 
    cube->setName(name);
    
    // add new name to nameToCubeId
    nameToCube.addElement(name, cube);

    // database has been changed
    setStatus(CHANGED);
    updateToken();

    // tell cube that it has been renamed
    if (notify) {
      cube->notifyRenameCube(oldName);
    }
    
    clearRuleCaches();
  }



  Cube* Database::addCube (const string& name, vector<Dimension*>* dimensions, User* user, bool isUserInfo) {

    if (isUserInfo) {
      checkUserInfoAccessRight(user, RIGHT_DELETE);
    }
    else {
      checkCubeAccessRight(user, RIGHT_WRITE);
    }
    
    checkCubeName(name, isUserInfo);
        
    // use new identifier
    IdentifierType identifier = fetchCubeIdentifier();
    
    // create new cube and add cube to cube vector
    Cube* cube;
    if (isUserInfo) {
      cube = new UserInfoCube(identifier, name, this, dimensions);
    }
    else {
      cube = new NormalCube(identifier, name, this, dimensions);
    }

    try {
      // and add cube to structure
      addCube(cube, true);
    }
    catch (...) {
      delete cube;
      cube = 0;
      throw;
    }

    // log changes to journal
    saveDatabase();

    if (journal != 0) {
      journal->appendCommand(Server::getUsername(user),
                             Server::getEvent(),
                             "ADD_CUBE");
      journal->appendInteger(cube->getIdentifier());
      journal->appendEscapeString(name);

      IdentifiersType identifiers;

      for (vector<Dimension*>::iterator i = dimensions->begin();  i != dimensions->end();  i++) {
        identifiers.push_back((*i)->getIdentifier());
      }

      journal->appendIdentifiers(&identifiers);

      journal->nextLine();
    }

    // and return the cube
    return cube;
  }



  void Database::deleteCube (Cube* cube, User* user) {
    
    if (cube->getType() == USER_INFO) {
      checkUserInfoAccessRight(user, RIGHT_DELETE);
    }
    else {
      checkCubeAccessRight(user, RIGHT_DELETE);
    }    
    
    // check deletable flag
    if (! cube->isDeletable()) {
      throw ParameterException(ErrorException::ERROR_CUBE_UNDELETABLE,
                               "cube cannot be deleted",
                               "cube", cube->getName());
    }
    
    if (status == UNLOADED) {
      throw ParameterException(ErrorException::ERROR_DATABASE_NOT_LOADED,
                               "cannot delete cube of unloaded database",
                               "database", name);
    }
      
    // remove cube
    removeCube(cube, true);

    // delete it completely
    IdentifierType id = cube->getIdentifier();
    delete cube;      
    
    // log changes to journal
    if (journal != 0) {
      journal->appendCommand(Server::getUsername(user),
                             Server::getEvent(),
                             "DELETE_CUBE");
      journal->appendInteger(id);
      journal->nextLine();
    }

    // save database to delete cube from database file 
    saveDatabase();
  }

    
    
  void Database::renameCube (Cube* cube, const string& name, User* user) {
    
    if (cube->getType() == USER_INFO) {
      checkUserInfoAccessRight(user, RIGHT_WRITE);
    }
    else {
      checkCubeAccessRight(user, RIGHT_WRITE);
    }    

    // check renamable flag
    if (! cube->isRenamable()) {
      throw ParameterException(ErrorException::ERROR_CUBE_UNRENAMABLE,
                               "cube cannot be renamed",
                               "cube", cube->getName());
    }
    
    if (name.empty()) {
      throw ParameterException(ErrorException::ERROR_INVALID_CUBE_NAME,
                               "cube name is empty", 
                               "name", name);
    }
    
    if (cube->getStatus() == Cube::UNLOADED) {
      throw ParameterException(ErrorException::ERROR_CUBE_NOT_LOADED,
                               "cube not loaded",
                               "cube", cube->getName());
    }
    
    if (cube->getType() == USER_INFO) {
      checkCubeName(name, true);
    }
    else {
      checkCubeName(name, false);
    }
    
    // rename cube
    renameCube(cube, name, true);

    if (journal != 0) {
      journal->appendCommand(Server::getUsername(user),
                             Server::getEvent(),
                             "RENAME_CUBE");
      journal->appendInteger(cube->getIdentifier());
      journal->appendEscapeString(name);
      journal->nextLine();
    }
  }

  
  
  void Database::loadCube (Cube * cube, User* user) {
    checkSystemOperationRight(user, RIGHT_DELETE);    

    if (fileName == 0) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "database file name not set");
    }
     
    if (status == UNLOADED) {
      throw ParameterException(ErrorException::ERROR_DATABASE_NOT_LOADED,
                               "cannot load cube of unloaded database",
                               "fileName", fileName->fullPath());
    }
      
    updateToken();
    
    cube->loadCube(true);
  }
  
  
  
  void Database::saveCube (Cube* cube, User* user) {
    checkSystemOperationRight(user, RIGHT_WRITE);    

    if (! isLoadable()) {
      throw ParameterException(ErrorException::ERROR_DATABASE_UNSAVED,
                               "cannot save cube of unsaved database",
                               "database", name);
    }
    
    cube->saveCube();
  }

  

  void Database::unloadCube (Cube* cube, User* user) {
    checkSystemOperationRight(user, RIGHT_DELETE);    

    if (status == UNLOADED) {
      throw ParameterException(ErrorException::ERROR_DATABASE_NOT_LOADED,
                               "cannot unload cube of an unloaded database",
                               "database", name);
    }

    if (getType() == NORMAL && (cube->getType() == NORMAL || cube->getType() == USER_INFO)) {
      cube->unloadCube();
    }
    else {
      throw ParameterException(ErrorException::ERROR_CUBE_IS_SYSTEM_CUBE,
			       "cannot unload system or attribute cubes",
			       "cube", cube->getName());    }
  }

  

  ////////////////////////////////////////////////////////////////////////////////
  // other stuff
  ////////////////////////////////////////////////////////////////////////////////

  void Database::updateToken () {
    token++;
    server->updateToken();
    User::updateGlobalDatabaseToken(this);
  }

  void Database::checkName (const string& name) {
    if (name.empty()) {
      throw ParameterException(ErrorException::ERROR_INVALID_DATABASE_NAME,
                               "name is empty",
                               "name", name);
    }
    
    if (name[0] == ' ' || name[name.length()-1] == ' ') {
      throw ParameterException(ErrorException::ERROR_INVALID_DATABASE_NAME,
                               "name begins or ends with a space character",
                               "name", name);
    }
    
    if (name[0] == '.') {
      throw ParameterException(ErrorException::ERROR_INVALID_DATABASE_NAME,
                               "name begins with a dot character",
                               "name", name);
    }
    
    for (size_t i = 0; i < name.length(); i++) {
      if (0 <= name[i] && name[i] < 32) {
        throw ParameterException(ErrorException::ERROR_INVALID_DATABASE_NAME,
                               "name contains an illegal character",
                               "name", name);
      }
    }

    if (name.find_first_of(INVALID_CHARACTERS) != string::npos) {
      throw ParameterException(ErrorException::ERROR_INVALID_DATABASE_NAME,
                               "name contains an illegal character",
                               "name", name);
    }

  }  

  void Database::checkDimensionName (const string& name, bool isInfo) {
    try {
      checkName(name);
    }
    catch (ParameterException e) {
      throw ParameterException(ErrorException::ERROR_INVALID_DIMENSION_NAME,
                               "invalid dimension name",
                               "name", name);
    }
    
    if (isInfo) {
      if (name.length() < 3 || name.substr(0,2) != "##") {
        throw ParameterException(ErrorException::ERROR_INVALID_DIMENSION_NAME,
                           "invalid name for user info object",
                           "name", name);      
      } 
    }
  }  

  void Database::checkCubeName (const string& name, bool isInfo) {
    try {
      checkName(name);
    }
    catch (ParameterException e) {
      throw ParameterException(ErrorException::ERROR_INVALID_CUBE_NAME,
                               "invalid cube name",
                               "name", name);
    }
    
    if (isInfo) {
      if (name.length() < 3 || name.substr(0,2) != "##") {
        throw ParameterException(ErrorException::ERROR_INVALID_CUBE_NAME,
                           "invalid name for user info object",
                           "name", name);      
      } 
    }
  }  

  void Database::clearRuleCaches () {
    if (hasCachedRules) {
      Logger::debug << "clearing rule caches of database '" << name << "'" << endl;
      
      // get all cubes and clear the ruels cache    
      vector<Cube*> cubes = getCubes(0);
      for (vector<Cube*>::iterator i = cubes.begin(); i != cubes.end(); i++) {
        Cube * cube = *i;
        cube->clearRulesCache();
      }
      
      hasCachedRules = false;
    }
  }
}
