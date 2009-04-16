////////////////////////////////////////////////////////////////////////////////
/// @brief OLAP server
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

#ifndef OLAP_SERVER_H
#define OLAP_SERVER_H 1

#include "palo.h"

#include <string>
#include <vector>
#include <set>

#include "Collections/AssociativeArray.h"
#include "Collections/StringUtils.h"

#include "InputOutput/FileUtils.h"
#include "InputOutput/LoginWorker.h"

#include "Olap/Cube.h"
#include "Olap/Database.h"
#include "Olap/SystemDatabase.h"


namespace palo {
  class Cube;
  class User;
  class Scheduler;
  class ProgressCallback;
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief OLAP server
  ///
  /// An OALP server consists of OLAP databases.
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS Server {
  public:
    static const string NAME_SYSTEM_DATABASE;
    static const string VALID_DATABASE_CHARACTERS;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the revision of the server
    ////////////////////////////////////////////////////////////////////////////////

    static const char * getRevision ();
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the version of the server
    ////////////////////////////////////////////////////////////////////////////////

    static const char * getVersion ();
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if server is currently locked
    ////////////////////////////////////////////////////////////////////////////////
    
    static bool isBlocking () {
      return blocking;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets or releases a global lock for the server
    ////////////////////////////////////////////////////////////////////////////////
    
    static void setBlocking (bool blocking) {
      Server::blocking = blocking;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the active session
    ////////////////////////////////////////////////////////////////////////////////
    
    static IdentifierType getActiveSession () {
      return activeSession;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets the active session
    ////////////////////////////////////////////////////////////////////////////////

    static void setActiveSession (IdentifierType identifier) {
      Server::activeSession = identifier;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the active semaphore
    ////////////////////////////////////////////////////////////////////////////////
    
    static semaphore_t getSemaphore () {
      return semaphore;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets the active semaphore
    ////////////////////////////////////////////////////////////////////////////////

    static void setSemaphore (semaphore_t semaphore) {
      Server::semaphore = semaphore;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the active event
    ////////////////////////////////////////////////////////////////////////////////
    
    static const string& getEvent () {
      return event;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the active event
    ////////////////////////////////////////////////////////////////////////////////
    
    static void setEvent (const string& event) {
      Server::event = event;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the active user
    ///
    /// If no user is holding a lock, returns the supplied user. Otherwise the
    /// user holding the lock is returned.
    ////////////////////////////////////////////////////////////////////////////////
    
    static const string& getUsername (User* user) {
      static const string empty = "";
      static const string system = "#SYSTEM#";

      if (blocking) {
        return username;
      }
      else if (user == 0) {
        return system;
      }
      else {
        return user->getName();
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the active user (for debugging)
    ////////////////////////////////////////////////////////////////////////////////

    static const string& getRealUsername () {
      return username;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets the active user
    ////////////////////////////////////////////////////////////////////////////////

    static void setUsername (const string& username) {
      Server::username = username;
    }

    static void addChangedMarkerCube (Cube*);

    static void removeChangedMarkerCube (Cube*);

    static bool triggerMarkerCalculation (Cube*);

  private:
    struct Name2DatabaseDesc {
      uint32_t hash (const string& name) {
        return StringUtils::hashValueLower(name.c_str(), name.size());
      }

      bool isEmptyElement (Database * const & database) {
        return database == 0;
      }

      uint32_t hashElement (Database * const & database) {
        return hash(database->getName());
      }

      uint32_t hashKey (const string& key) {
        return hash(key);
      }

      bool isEqualElementElement (Database * const & left, Database * const & right) {
        return left->getName() == right->getName();
      }

      bool isEqualKeyElement (const string& key, Database * const & database) {
        const string& name = database->getName();

        return key.size() == name.size() && strncasecmp(key.c_str(), name.c_str(), key.size()) == 0;
      }

      void clearElement (Database * & database) {
        database = 0;
      }
    };

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates an empty server
    ////////////////////////////////////////////////////////////////////////////////

    Server (Scheduler*, const FileName& fileName);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes an server
    ////////////////////////////////////////////////////////////////////////////////

    ~Server ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name functions to save and load the server
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns true if the server is loadable
    ////////////////////////////////////////////////////////////////////////////////

    bool isLoadable () {
      return FileUtils::isReadable(fileName);
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief reads data from file
    ////////////////////////////////////////////////////////////////////////////////

    void loadServer (User* user);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief saves data to file  
    ////////////////////////////////////////////////////////////////////////////////

    void saveServer (User* user);
      
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief registers a progress callback
    ////////////////////////////////////////////////////////////////////////////////

    void registerProgressCallback (ProgressCallback* callback, double periodicProgressIntervalInSeconds) {
	  this->lastProgress = clock();
	  if (periodicProgressIntervalInSeconds>0)
		this->makeProgressInterval = (clock_t)(periodicProgressIntervalInSeconds * CLOCKS_PER_SEC);
      this->callback = callback;
    }
      
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief unregisters a progress callback
    ////////////////////////////////////////////////////////////////////////////////

    void unregisterProgressCallback () {
      this->callback = 0;
      this->makeProgressInterval = 0;
    }
      
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief calls the progress callback
    ////////////////////////////////////////////////////////////////////////////////

    void makeProgress ();

	void timeBasedProgress();
      
    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name getter and setter
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets the token
    ////////////////////////////////////////////////////////////////////////////////

    uint32_t getToken () const {
      return token;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets the system database
    ////////////////////////////////////////////////////////////////////////////////

    SystemDatabase* getSystemDatabase () {
      return systemDatabase;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets number of databases
    ////////////////////////////////////////////////////////////////////////////////

    size_t sizeDatabases () {
      return numDatabases;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets list of database
    ////////////////////////////////////////////////////////////////////////////////

    vector<Database*> getDatabases (User* user);


    void setLoginWorker (LoginWorker* worker) {
      loginWorker = worker;
    }
    
    LoginWorker* getLoginWorker () const {
      return loginWorker;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

 public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name functions to update internal structures
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief increments the token
    ////////////////////////////////////////////////////////////////////////////////

    void updateToken () {
      token++;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name functions to administrate the databases
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief shuts down the server
    ////////////////////////////////////////////////////////////////////////////////

    void beginShutdown (User *);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns a new database identifier
    ////////////////////////////////////////////////////////////////////////////////

    IdentifierType fetchDatabaseIdentifier ();
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief adds new database to server
    ////////////////////////////////////////////////////////////////////////////////

    void addDatabase (Database*, bool notify);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief removes a database to server
    ////////////////////////////////////////////////////////////////////////////////

    void removeDatabase (Database*, bool notify);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief renames a database
    ////////////////////////////////////////////////////////////////////////////////

    void renameDatabase (Database*, const string& name, bool notify);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief adds new database to server
    ////////////////////////////////////////////////////////////////////////////////

    Database* addDatabase (const string& name, User* user);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes database
    ////////////////////////////////////////////////////////////////////////////////

    void deleteDatabase (Database* database, User* user);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief renames a database
    ////////////////////////////////////////////////////////////////////////////////

    void renameDatabase (Database* database, const string& name, User* user);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief loads database into memory
    ////////////////////////////////////////////////////////////////////////////////

    void loadDatabase (Database* database, User* user);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief saves database
    ////////////////////////////////////////////////////////////////////////////////

    void saveDatabase (Database* database, User* user);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief unloads database from memory
    ////////////////////////////////////////////////////////////////////////////////

    void unloadDatabase (Database* database, User* user);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets database by identifier
    ///
    /// This function is defined here to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    Database * lookupDatabase (IdentifierType identifier) {
      return identifier < databases.size() ? databases[identifier] : 0;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets database by name
    ///
    /// This function is defined here to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    Database * lookupDatabaseByName (const string& name) {
      return nameToDatabase.findKey(name);
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets database by identifier
    ///
    /// This function is defined here to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    Database * findDatabase (IdentifierType identifier, User* user, bool requireLoad = true) {
      checkDatabaseAccessRight(user, RIGHT_READ);
      
      Database* database = lookupDatabase(identifier);
      
      if (database == 0) {
        throw ParameterException(ErrorException::ERROR_DATABASE_NOT_FOUND,
                                 "database not found",
                                 "database identifier", (int) identifier);
      }

      if (requireLoad && database->getStatus() == Database::UNLOADED) {
        throw ParameterException(ErrorException::ERROR_DATABASE_NOT_LOADED,
                                 "database not loaded",
                                 "database identifier", (int) identifier);
      }

      if (database->getType() == SYSTEM && user != 0 && user->getRoleRightsRight() < RIGHT_READ) {
        throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                 "insufficient access rights for database",
                                 "user", (int) user->getIdentifier());        
      }    

      return database;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets database by name
    ///
    /// This function is defined here to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    Database * findDatabaseByName (const string& name, User* user, bool requireLoad = true) {
      checkDatabaseAccessRight(user, RIGHT_READ);

      Database* database = lookupDatabaseByName(name);
      
      if (database == 0) {
        throw ParameterException(ErrorException::ERROR_DATABASE_NOT_FOUND,
                                 "database not found",
                                 "name", name);
      }

      if (requireLoad && database->getStatus() == Database::UNLOADED) {
        throw ParameterException(ErrorException::ERROR_DATABASE_NOT_LOADED,
                                 "database not loaded",
                                 "name", name);
      }
      
      if (database->getType() == SYSTEM && user != 0 && user->getRoleRightsRight() < RIGHT_READ) {
        throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                 "insufficient access rights for database",
                                 "user", (int) user->getIdentifier());        
      }    
      
      return database;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name functions to administrate the system databases
    ////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates or loads the system database #_SYSTEM_
    ////////////////////////////////////////////////////////////////////////////////

    void addSystemDatabase (); 

    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:
    void rebuildAllMarkers ();

  private:
    static FileName computeDatabaseFileName (const FileName& fileName, IdentifierType identifier, const string& name);

  private:
    static bool blocking;
    static IdentifierType activeSession;
    static semaphore_t semaphore;
    static string username;
    static string event;

    static set<Cube*> changedMarkerCubes;

  private:
    size_t loadServerOverview (FileReader* file);
    void loadServerDatabase (FileReader* file, size_t sizeDatabases);
    void loadServerDatabases (FileReader* file, size_t sizeDatabases);

    void saveServerOverview (FileWriter* file);
    void saveServerDatabases (FileWriter* file);

    void checkDatabaseAccessRight (User* user, RightsType minimumRight) {
      if (user != 0 && user->getRoleDatabaseRight() < minimumRight) {
        throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                 "insufficient access rights for database",
                                 "user", (int) user->getIdentifier());
      }
    }    

    void checkSystemOperationRight (User* user, RightsType minimumRight) {
      if (user != 0 && user->getRoleSysOpRight() < minimumRight) {
        throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                 "insufficient access rights for system operation",
                                 "user", (int) user->getIdentifier());
      }
    }    
    
  private:
    uint32_t token;                    // token for changes
    size_t numDatabases;               // number of databases
    vector<Database*> databases;       // list of databases
    set<IdentifierType> freeDatabases; // list of unused datbases in list of databases

    AssociativeArray<string, Database*, Name2DatabaseDesc> nameToDatabase;
    
    FileName fileName; // file name of the server
    
    SystemDatabase* systemDatabase; // this database contains users, passwords and other system data 
    
    LoginWorker* loginWorker;

    Scheduler * scheduler;

    ProgressCallback * callback;
	clock_t lastProgress;
	clock_t makeProgressInterval;
  };

}

#endif 
