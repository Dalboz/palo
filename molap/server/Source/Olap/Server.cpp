////////////////////////////////////////////////////////////////////////////////
/// @brief palo server
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

#include "Olap/Server.h"

#include <algorithm>
#include <iostream>

#include "Exceptions/FileFormatException.h"
#include "Exceptions/ParameterException.h"

#include "Collections/DeleteObject.h"
#include "Collections/StringBuffer.h"

#include "InputOutput/FileReader.h"
#include "InputOutput/FileWriter.h"
#include "InputOutput/Logger.h"
#include "InputOutput/ProgressCallback.h"
#include "InputOutput/Scheduler.h"
#include "InputOutput/Statistics.h"

#include "Olap/NormalDatabase.h"
#include "Olap/RuleMarker.h"
#include "Olap/SystemDatabase.h"
#include "Olap/User.h"

#include "build.h"

namespace palo {
  const string Server::NAME_SYSTEM_DATABASE = "System";
  const string Server::VALID_DATABASE_CHARACTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-.";
  bool Server::blocking = false;
  semaphore_t Server::semaphore = 0;
  IdentifierType Server::activeSession = 0;
  string Server::username = "";
  string Server::event = "";
  set<Cube*> Server::changedMarkerCubes;


  void Server::addChangedMarkerCube (Cube* cube) {
    Statistics::Timer timer("server::addChangedMarkerCube");

    // check if already know this cube, do nothing in this case
    if (changedMarkerCubes.find(cube) != changedMarkerCubes.end()) {
      return;
    }

    // add cube
    changedMarkerCubes.insert(cube);

    Logger::trace << "adding cube '" << cube->getName() << "' to the list of changed marker cubes" << endl;

    // and all its destinations
    const set<RuleMarker*>& fromMarkers = cube->getFromMarkers();

    for (set<RuleMarker*>::const_iterator i = fromMarkers.begin();  i != fromMarkers.end();  i++) {
      RuleMarker* marker = *i;

      addChangedMarkerCube(marker->getToCube());
    }
  }



  bool Server::triggerMarkerCalculation (Cube* c) {
    if (changedMarkerCubes.empty()) {
      return false;
    }

    bool seen = (changedMarkerCubes.find(c) != changedMarkerCubes.end());

    // first clear all markers
    for (set<Cube*>::iterator i = changedMarkerCubes.begin();  i != changedMarkerCubes.end();  i++) {
      Cube* cube = *i;

      cube->clearAllMarkers();
    }

    // then rebuild
    set<Cube*> cubes;
    changedMarkerCubes.swap(cubes);

    for (set<Cube*>::iterator i = cubes.begin();  i != cubes.end();  i++) {
      Cube* cube = *i;

      cube->rebuildAllMarkers();
    }

    return seen;
  }



  void Server::removeChangedMarkerCube (Cube* cube) {
    changedMarkerCubes.erase(cube);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // constructors and destructors
  ////////////////////////////////////////////////////////////////////////////////

  Server::Server (Scheduler* scheduler, const FileName& fileName)
    : token(rand()),
      numDatabases(0),
      nameToDatabase(1000, Name2DatabaseDesc()),
      fileName(fileName),
      scheduler(scheduler),
      callback(0) {

    if (fileName.name.empty()) {
      throw ParameterException(ErrorException::ERROR_INVALID_SERVER_PATH,
                               "empty file name",
                               "file name", "");
    }

    systemDatabase = 0;
    loginWorker = 0;

	makeProgressInterval = 0; 

    // create a singleton
    semaphore = scheduler->createSemaphore();
  }



  Server::~Server () {
    for_each(databases.begin(), databases.end(), DeleteObject());
  }

  ////////////////////////////////////////////////////////////////////////////////
  // functions to load and save a server
  ////////////////////////////////////////////////////////////////////////////////

  void Server::makeProgress () {
    lastProgress = clock();
    if (callback != 0) {
      callback->callback();
    }
  }

  void Server::timeBasedProgress() {	  	
		if (makeProgressInterval > 0 && clock() - lastProgress > makeProgressInterval) 
			makeProgress();			
  }



  FileName Server::computeDatabaseFileName (const FileName& fileName, IdentifierType identifier, const string& name) {
    return FileName(fileName.path + "/" + name, "database", "csv");
  }



  size_t Server::loadServerOverview (FileReader* file) {
    size_t sizeDatabases = 0;

    if (file->isSectionLine() && file->getSection() == "SERVER" ) {
      file->nextLine();

      if (file->isDataLine()) {
        sizeDatabases = file->getDataInteger(0);
        file->nextLine();
      }
    }
    else {
      throw FileFormatException("section 'SERVER' not found", file);
    }

    return sizeDatabases;
  }



  void Server::loadServerDatabase (FileReader* file, size_t sizeDatabases) {
    IdentifierType identifier = (IdentifierType) file->getDataInteger(0);
    string name               = file->getDataString(1);
    int type                  = file->getDataInteger(2);

    if (identifier >= sizeDatabases) {
      Logger::error << "database identifier '" << identifier << "' of database '" << name << "' is greater or equal than maximum (" <<  sizeDatabases << ")" << endl;
      throw FileFormatException("wrong identifier for database", file);
    }

    FileName dbFile = computeDatabaseFileName(fileName, identifier, name);
    
    if (FileUtils::isReadable(dbFile)) {
      Database* database = Database::loadDatabaseFromType(file, this, identifier, name, type);
            
      addDatabase(database, false);
    }
    else {
      Logger::warning << "database file for '" << name << "' is missing, removing database" << endl;
    }

    file->nextLine();
  }



  void Server::loadServerDatabases (FileReader* file, size_t sizeDatabases) {
    for_each(databases.begin(), databases.end(), DeleteObject());

    numDatabases = 0;
    freeDatabases.clear();
    databases.clear();

    if (file->isSectionLine() && file->getSection() == "DATABASES" ) {
      file->nextLine();

      while ( file->isDataLine() ) {                        
        loadServerDatabase(file, sizeDatabases);
      }          
    }
    else {
      throw FileFormatException("section 'DATABASES' not found", file);
    }
    
    for (size_t i = 0; i < databases.size(); i++) {
      if (databases.at(i) == 0) {
        freeDatabases.insert(i);
      }
    }
  }



  void Server::loadServer (User* user) {
    checkSystemOperationRight(user, RIGHT_DELETE);
          
    updateToken();

    if (!FileUtils::isReadable(fileName) && FileUtils::isReadable(FileName(fileName, "tmp"))) {
      Logger::warning << "using temp file for server" << endl;
      // rename temp file
      FileUtils::rename(FileName(fileName, "tmp"), fileName);
    } 

    FileReader fr(fileName);
    fr.openFile();
            
    // load server from file
    size_t sizeDatabases = loadServerOverview(&fr);
    loadServerDatabases(&fr, sizeDatabases);
  }



  void Server::saveServerOverview (FileWriter* file) {
    file->appendComment("PALO SERVER DATA");
    file->appendComment("");

    file->appendComment("Description of data:");
    file->appendComment("NUMBER_DATABASES;");

    file->appendSection("SERVER");
    file->appendInteger( (uint32_t) databases.size() );
    file->nextLine();
  }



  void Server::saveServerDatabases (FileWriter* file) {
      
    // write data for databases
    file->appendComment("Description of data: ");
    file->appendComment("ID;NAME;TYPE;DELETABLE;RENAMABLE;EXENTSIBLE");

    file->appendSection("DATABASES" );

    vector<Database*> databases = getDatabases(0);

    for (vector<Database*>::iterator i = databases.begin();  i != databases.end();  i++) {
      Database * database = *i;
      database->saveDatabaseType(file);
    }
  }



  void Server::saveServer (User* user) {
    checkSystemOperationRight(user, RIGHT_WRITE);

    // open a new temp-server file
    FileWriter fw(FileName(fileName, "tmp"), false);
    fw.openFile();

    // save server and databases to disk
    saveServerOverview(&fw);
    saveServerDatabases(&fw);

    fw.appendComment("");
    fw.appendComment("PALO SERVER DATA END");
    fw.closeFile();
    
    // remove old server file
    if (FileUtils::isReadable(fileName) && ! FileUtils::remove(fileName)) {
      Logger::error << "cannot remove server file: '" << strerror(errno) << endl;
      Logger::error << "please check the underlying file system for errors" << endl;
      exit(1);
    }

    // rename temp-server file
    if (! FileUtils::rename(FileName(fileName, "tmp"), fileName)) {
      Logger::error << "cannot rename server file: '" << strerror(errno) << endl;
      Logger::error << "please check the underlying file system for errors" << endl;
      exit(1);
    }
  }



  void Server::rebuildAllMarkers () {
    Logger::info << "rebuilding all markers" << endl;

    for (vector<Database*>::iterator i = databases.begin();  i!= databases.end();  i++) {
      Database* database = *i;

      if (database != 0) {
        database->calculateMarkers();
      }
    }
  }
    
  ////////////////////////////////////////////////////////////////////////////////
  // getter and setter
  ////////////////////////////////////////////////////////////////////////////////

  const char * Server::getRevision () {
    return ::Revision;
  }



  const char * Server::getVersion () {
    return ::Version;
  }



  vector<Database*> Server::getDatabases (User* user) {
    checkDatabaseAccessRight(user, RIGHT_READ);   
    bool showSystem = user == 0 || user->getRoleRightsRight() > RIGHT_NONE;    
    
    vector<Database*> result;
      
    for (vector<Database*>::iterator i = databases.begin();  i < databases.end();  i++) {        
      Database * database = *i;

      if (database != 0) {
        if (database->getType() != SYSTEM || showSystem) {
          result.push_back(database);
        }
      }        
    }
      
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // functions to administrate the databases
  ////////////////////////////////////////////////////////////////////////////////

  void Server::beginShutdown (User * user) {
    if (user != 0 && user->getRoleSysOpRight() < RIGHT_DELETE) {
      throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                               "insufficient access rights",
                               "user", (int) user->getIdentifier());
    }

    if (scheduler != 0) {
      if (loginWorker) {
        semaphore_t semaphore = scheduler->createSemaphore();
        loginWorker->executeShutdown(semaphore);
      }
      for (vector<Database*>::iterator id = databases.begin(); id != databases.end(); id++) {
        if (*id) {
          vector<Cube*> cubes = (*id)->getCubes(0);
          for (vector<Cube*>::iterator ic = cubes.begin(); ic != cubes.end(); ic++) {
            (*ic)->sendExecuteShutdown();
          }
        }
      }
      

      scheduler->beginShutdown();
    }
  }



  IdentifierType Server::fetchDatabaseIdentifier () {
    return (IdentifierType) databases.size();
  }



  // this method is used internally, so no rights checking is done
  void Server::addDatabase (Database* database, bool notify) {
    const string& name = database->getName();
    
    if (lookupDatabaseByName(name) != 0) {
      throw ParameterException(ErrorException::ERROR_DATABASE_NAME_IN_USE,
                               "database name is already in use",
                               "name", name);
    }      
            
    IdentifierType identifier = database->getIdentifier();

    if (lookupDatabase(identifier) != 0) {
      throw ParameterException(ErrorException::ERROR_INTERNAL,
                               "database identifier is already in use",
                               "identifier", (int) identifier);
    }

    // check if we have to change the free list
    if (identifier >= databases.size()) {
      databases.resize(identifier + 1, 0);

      for (size_t i = databases.size() - 1;  i < identifier;  i++) {
        freeDatabases.insert(i);
      }
    }

    // add dimension to mapping
    databases[identifier] = database;
    nameToDatabase.addElement(name, database);

    // we have one more database
    numDatabases++;
    
    // set database file
    database->setDatabaseFile(computeDatabaseFileName(fileName, identifier, name));
    
    // server has been changed
    updateToken();

    // tell database that it has been added
    if (notify) {
      database->notifyAddDatabase();
    }
  }



  // this method is used internally, so no rights checking is done
  void Server::removeDatabase (Database* database, bool notify) {
    IdentifierType identifier = database->getIdentifier();
      
    if (database != lookupDatabase(identifier)) {
      throw ParameterException(ErrorException::ERROR_DATABASE_NOT_FOUND,
                               "database not found",
                               "database", database->getName());
    }
    
    // delete database from mapping
    nameToDatabase.removeElement(database);
    
    // we habe one database less
    numDatabases--;

    // add identifier of the database to freeDatabases
    freeDatabases.insert(identifier);
    
    // and remove database from database list
    databases[identifier] = 0;

    // update server token
    updateToken();
    
    // tell database that it has been removed
    if (notify) {
      database->notifyRemoveDatabase();
    }
  }



  // this method is used internally, so no rights checking is done
  void Server::renameDatabase (Database* database, const string& name, bool notify) {

    // check for doubly defined name
    Database* databaseByName = lookupDatabaseByName(name);

    if (databaseByName != 0) {
      if (databaseByName != database) {
        throw ParameterException(ErrorException::ERROR_DATABASE_NAME_IN_USE,
                               "database name already in use",
                               "name", name);
      }        

      if (database->getName() == name) {
        // new name == old name
        return;
      }
    }

    // keep old name
    const string oldName = database->getName();

    // try to change name
    database->setDatabaseFile(computeDatabaseFileName(fileName, database->getIdentifier(), name));
    database->setName(name);

    // delete old mapping
    nameToDatabase.removeKey(oldName);

    // add new name to mapping
    nameToDatabase.addElement(name, database);

    // tell database that it has been renamed
    if (notify) {
      database->notifyRenameDatabase();
    }
  }



  Database* Server::addDatabase (const string& realName, User* user) {
    checkDatabaseAccessRight(user, RIGHT_WRITE);

    if (realName.empty()) {
      throw ParameterException(ErrorException::ERROR_INVALID_DATABASE_NAME,
                               "database name is empty",
                               "name", realName);
    }
      
    string name = realName;

    if (name.find_first_not_of(VALID_DATABASE_CHARACTERS) != string::npos) {
      throw ParameterException(ErrorException::ERROR_INVALID_DATABASE_NAME,
                               "database name contains an illegal character",
                               "name", name);
    }

    if (name[0] == '.') {
      throw ParameterException(ErrorException::ERROR_INVALID_DATABASE_NAME,
                               "database name begins with a dot character",
                               "name", name);
    }

    // create new identifier
    IdentifierType identifier = fetchDatabaseIdentifier();

    // create database and add database to database vector
    Database* database = new NormalDatabase(identifier, this, name);

    database->setDeletable(true);
    database->setRenamable(true);
    database->setExtensible(true);
    
    // and update database structure
    try {
      addDatabase(database, true);
    }
    catch (...) {
      delete database;
      throw;
    }
    
    // return database
    return database;     
  }



  void Server::deleteDatabase (Database* database, User* user) {
    checkDatabaseAccessRight(user, RIGHT_DELETE);
    
    // check deletable flag
    if (! database->isDeletable()) {
      throw ParameterException(ErrorException::ERROR_DATABASE_UNDELETABLE,
                               "database cannot be deleted",
                               "database", database->getName());
    }
    
    // remove database
    removeDatabase(database, true);

    // delete database and cube files from disk
    try {
      database->deleteDatabaseFiles();
    }
    catch (...) {
      delete database;
      throw;
    }
      
    // delete database
    delete database;
  }



  void Server::renameDatabase (Database* database, const string& realName, User* user) {
    checkDatabaseAccessRight(user, RIGHT_WRITE);

    // check renamable flag
    if (! database->isRenamable()) {
      throw ParameterException(ErrorException::ERROR_DATABASE_UNRENAMABLE,
                               "database cannot be renamed",
                               "database", database->getName());
    }
    
    if (realName.empty()) {
      throw ParameterException(ErrorException::ERROR_INVALID_DATABASE_NAME,
                               "database name is empty",
                               "name", realName);
    }

    if (database->getType() != NORMAL) {
      throw ParameterException(ErrorException::ERROR_DATABASE_UNRENAMABLE,
                               "system database cannot be renamed",
                               "name", realName);
    }

    string name = realName;

    if (name.find_first_not_of(VALID_DATABASE_CHARACTERS) != string::npos) {
      throw ParameterException(ErrorException::ERROR_INVALID_DATABASE_NAME,
                               "database name contains illegal characters",
                               "name", name);
    }

    // change name 
    renameDatabase(database, name, true);
  }
  

        
  void Server::loadDatabase (Database* database, User* user) {
    checkSystemOperationRight(user, RIGHT_DELETE);

    database->loadDatabase();
  }



  void Server::saveDatabase (Database* database, User* user) {
    checkSystemOperationRight(user, RIGHT_WRITE);
      
    database->saveDatabase();
  }



  void Server::unloadDatabase (Database* database, User* user) {
    checkSystemOperationRight(user, RIGHT_DELETE);
    
    database->unloadDatabase();
  }

  ////////////////////////////////////////////////////////////////////////////////
  // functions to administrate the system databases
  ////////////////////////////////////////////////////////////////////////////////

  void Server::addSystemDatabase () {
    Database* database = lookupDatabaseByName(NAME_SYSTEM_DATABASE);
    
    if (database == 0) {
      Logger::info << "system database not found" << endl;
      
      // create system database      
      systemDatabase = new SystemDatabase(fetchDatabaseIdentifier(), this, NAME_SYSTEM_DATABASE);
      systemDatabase->setDeletable(false);
      systemDatabase->setRenamable(false);

      addDatabase(systemDatabase, true);

      Logger::trace << "system database check" << endl;
      // create system dimensions and cubes
      systemDatabase->createSystemItems(true);

      Logger::info << "created system database " << endl;
     
      return; 
    }
    
    if (database->getStatus() == Database::UNLOADED) {
      Logger::info << "system database not yet loaded, loading now" << endl;
      loadDatabase(database, 0);
    }
     
    if (database->getType() == SYSTEM) {
      Logger::trace << "system database found and loaded" << endl;
      systemDatabase = (SystemDatabase*) database;
        
      Logger::trace << "system database check" << endl;
      // check system dimensions and cubes
      systemDatabase->createSystemItems();
    }
    else {
      // TODO: this is an error
      cout << "system database is not of type SYSTEM" << endl;
    }
  }   
  
}
