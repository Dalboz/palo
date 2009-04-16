////////////////////////////////////////////////////////////////////////////////
/// @brief session handler for http
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

#ifndef OLAP_PALO_SESSION_H
#define OLAP_PALO_SESSION_H 1

#include "palo.h"

#include <map>
#include <deque>

extern "C" {
#include <time.h>
}

#include "Olap/Cube.h"
#include "Olap/Database.h"
#include "Olap/Server.h"

namespace palo {
  class User;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief session handler for http
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS PaloSession {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates a new session
    ////////////////////////////////////////////////////////////////////////////////

    static PaloSession * createSession (IdentifierType sessionId, Server *, User *, bool worker, time_t ttl);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates a new session
    ////////////////////////////////////////////////////////////////////////////////

    static PaloSession * createSession (Server *, User *, bool worker, time_t ttl);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes a session
    ////////////////////////////////////////////////////////////////////////////////

    static void deleteSession (PaloSession *);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief finds session by identifier
    ////////////////////////////////////////////////////////////////////////////////

    static PaloSession * findSession (IdentifierType);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief finds session by encoded identifier
    ////////////////////////////////////////////////////////////////////////////////

    static PaloSession * findSession (const string& identifier);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief stores cell replace from bulk update
    ////////////////////////////////////////////////////////////////////////////////

    static void storeCellReplace (Database * database,
                                  Cube * cube,
                                  IdentifiersType ids, 
                                  const string& value,
                                  Cube::SplashMode mode) {
      storedDatabases.push_back(database->getIdentifier());
      storedCubes.push_back(cube->getIdentifier());
      storedIds.push_back(ids);
      storedValues.push_back(value);
      storedModes.push_back(mode);
    }


    static bool hasCellReplace () {
      return ! storedDatabases.empty();
    }

    static void popCellReplace (Server * server,
                                Database * * database,
                                Cube * * cube,
                                IdentifiersType * ids,
                                string * value,
                                Cube::SplashMode * mode) {
      if (storedDatabases.empty()) {
        return;
      }

      *database = server->findDatabase(storedDatabases[0], 0);
      *cube = (*database)->findCube(storedCubes[0], 0);
      *ids = storedIds[0];
      *value = storedValues[0];
      *mode = storedModes[0];

      storedDatabases.pop_front();
      storedCubes.pop_front();
      storedIds.pop_front();
      storedValues.pop_front();
      storedModes.pop_front();
    }


    static bool isUserActive (IdentifierType user);

  private:
    PaloSession (IdentifierType, Server *, User*, bool worker, time_t);
    ~PaloSession ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the session identifier
    ////////////////////////////////////////////////////////////////////////////////

    IdentifierType getIdentifier () const {
      return identifier;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the base64 encoded session identifier
    ////////////////////////////////////////////////////////////////////////////////

    string getEncodedIdentifier ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if session is a worker session
    ////////////////////////////////////////////////////////////////////////////////

    bool isWorker () const {
      return worker;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the time to live
    ////////////////////////////////////////////////////////////////////////////////

    time_t getTtl () const {
      return timeToLive;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the time to live interval
    ////////////////////////////////////////////////////////////////////////////////

    time_t getTtlIntervall () const {
      return timeToLiveIntervall;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the user or 0
    ////////////////////////////////////////////////////////////////////////////////

    User * getUser () const;


    size_t getSizeOfHistory(IdentifierType database, IdentifierType cube);
    void addPathToHistory(IdentifierType database, IdentifierType cube, const IdentifiersType* path);
    void printHistory();
    const vector< IdentifiersType >* getHistory () {
      return & pathsHistory;
    }
    void removeHistory () {
      pathsHistory.clear();
    }
    
  private:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief updates TTL
    ////////////////////////////////////////////////////////////////////////////////

    void updateTtl () {
      timeToLive = time(0) + timeToLiveIntervall;
    }
    
    bool historyTimedOut (const timeval& l, const timeval& r);

  private:
    static map< IdentifierType, PaloSession* > sessions;

    static deque<IdentifierType> storedDatabases;
    static deque<IdentifierType> storedCubes;
    static deque<IdentifiersType> storedIds;
    static deque<string> storedValues;
    static deque<Cube::SplashMode> storedModes;

  private:
    IdentifierType identifier;
    Server * server;
    IdentifierType userIdentifier;
    bool worker;
    time_t timeToLive;
    time_t timeToLiveIntervall;

  private:
    vector< IdentifiersType > pathsHistory;
    IdentifierType databaseId;
    IdentifierType cubeId;
    timeval lastTime;
  };

}

#endif
