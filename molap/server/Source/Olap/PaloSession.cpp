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

#include "Olap/PaloSession.h"

#include "Exceptions/ParameterException.h"

#include "Olap/SystemDatabase.h"
#include "Olap/Server.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

namespace palo {
  map< IdentifierType, PaloSession* > PaloSession::sessions;
  deque<IdentifierType> PaloSession::storedDatabases;
  deque<IdentifierType> PaloSession::storedCubes;
  deque<IdentifiersType> PaloSession::storedIds;
  deque<string> PaloSession::storedValues;
  deque<Cube::SplashMode> PaloSession::storedModes;



  bool PaloSession::isUserActive (IdentifierType id) {
    time_t now = time(0);

    for (map<IdentifierType, PaloSession*>::iterator i = sessions.begin();  i != sessions.end();  i++) {
      PaloSession* session = i->second;

      Logger::error << "id = " << id << " session user id = " << session->userIdentifier << " ttl " << session->getTtl() << endl;

      if (session->userIdentifier == id && session->getTtl() >= now) {
        return true;
      }
    }

    return false;
  }



  PaloSession::PaloSession (IdentifierType identifier,
                            Server * server,
                            User * user,
                            bool worker,
                            time_t ttlIntervall) 
    : identifier(identifier), server(server), worker(worker), timeToLiveIntervall(ttlIntervall) {
    userIdentifier = user == 0 ? (IdentifierType) -1 : user->getIdentifier();

    if (timeToLiveIntervall == 0) {
      timeToLiveIntervall = 24L * 3600L * 364L * 10L;
    }

    timeToLive = time(0) + timeToLiveIntervall;

    databaseId = 0;
    cubeId = 0;
    gettimeofday(&lastTime, 0);
  }



  PaloSession::~PaloSession () {
  }



  PaloSession * PaloSession::createSession (IdentifierType sessionId,
                                            Server * server,
                                            User * user,
                                            bool worker,
                                            time_t ttlIntervall) {
    Logger::debug << "session key " << sessionId << endl;

    PaloSession * session = new PaloSession(sessionId, server, user, worker, ttlIntervall);
    sessions[sessionId] = session;

    return session;
  }



  PaloSession * PaloSession::createSession (Server * server,
                                            User * user,
                                            bool worker,
                                            time_t ttlIntervall) {
    IdentifierType nextSession;

    do {
      nextSession = (IdentifierType)
        ((rand() ^ (rand() << 8) ^ (rand() << 16) ^ (rand() << 24)) & 0x00FFFFFF);
    } while (nextSession == 0);

    map< IdentifierType, PaloSession* >::iterator iter = sessions.find(nextSession);

    while (iter != sessions.end()) {
      nextSession = (nextSession + 1) & 0x00FFFFFF;

      if (nextSession == 0) {
        nextSession = 1;
      }

      iter = sessions.find(nextSession);
    }

    return createSession(nextSession, server, user, worker, ttlIntervall);
  }



  void PaloSession::deleteSession (PaloSession * session) {
    sessions.erase(session->getIdentifier());

    delete session;
  }



  PaloSession * PaloSession::findSession (IdentifierType id) {
    map< IdentifierType, PaloSession* >::iterator iter = sessions.find(id);

    if (iter == sessions.end()) {
      throw ParameterException(ErrorException::ERROR_INVALID_SESSION,
                               "wrong session identifier",
                               "session identifier", (int) id);
    }

    PaloSession * session = iter->second;

    if (time(0) > session->getTtl()) {
      throw ParameterException(ErrorException::ERROR_INVALID_SESSION,
                               "old session identifier",
                               "session identifier", (int) id);
    }

    session->updateTtl();

    return session;
  }



  PaloSession * PaloSession::findSession (const string& identifier) {
    uint8_t sid[3];
    size_t length = sizeof(sid);
    StringUtils::decodeBase64(identifier, sid, length);
    
    if (length != 3) { 
      throw ParameterException(ErrorException::ERROR_INVALID_SESSION,
                               "wrong session identifier",
                               "session identifier", identifier);
    }
    
    // get session identifier as integer
    IdentifierType sessionId = (sid[0] << 16) | (sid[1] << 8) | sid[2];

    // find session
    return PaloSession::findSession(sessionId);
  }



  string PaloSession::getEncodedIdentifier () {
    IdentifierType sessionId = identifier;
   
    uint8_t buf[3];
    buf[0] = (sessionId >> 16) & 0xFF;
    buf[1] = (sessionId >>  8) & 0xFF;
    buf[2] = (sessionId      ) & 0xFF;

    return StringUtils::encodeBase64(buf, 3);
  }



  User * PaloSession::getUser () const {
    SystemDatabase* sd = server->getSystemDatabase();
    User * user = 0;

    if (sd) {
      if (userIdentifier != (IdentifierType) -1) {
        user = sd->getUser(userIdentifier);
      }
      else {
        return 0;
      }
    }

    if (user == 0) {
      throw ParameterException(ErrorException::ERROR_INVALID_SESSION,
                               "user in session not found",
                               "user", (int) userIdentifier);
    }

    return user;
  }
  

  bool PaloSession::historyTimedOut (const timeval& l, const timeval& r) {
    time_t      sec = l.tv_sec  - r.tv_sec;
    suseconds_t msc = l.tv_usec - r.tv_usec;

    while (msc < 0) {
      msc += 1000000;
      sec -= 1;
    }

    return ((sec * 1000000LL) + msc) > 100000;    
  }

  
    
  size_t PaloSession::getSizeOfHistory(IdentifierType database, IdentifierType cube) {
    if (database != databaseId ||
        cubeId != cube) {
      return 0;
    }
    else {
      timeval now;
      gettimeofday(&now, 0);
      if (historyTimedOut(now, lastTime)) {
        Logger::debug << "clear path history" << endl; 
        pathsHistory.clear();
      }
      lastTime = now;    
    }
    return pathsHistory.size();
  }

  
  void PaloSession::addPathToHistory(IdentifierType database, IdentifierType cube, const IdentifiersType* path) {
    timeval now;
    gettimeofday(&now, 0);

    if (database != databaseId ||
        cubeId != cube) {
      databaseId = database;
      cubeId = cube;
      Logger::debug << "clear path history" << endl; 
      pathsHistory.clear();
    }
    else {    
      if (historyTimedOut(now, lastTime)) {
        Logger::debug << "clear path history" << endl; 
        pathsHistory.clear();
      }
    }

    lastTime = now;    
    pathsHistory.push_back(*path);    
  }
  

  
  void PaloSession::printHistory() {
    vector< IdentifiersType >::iterator p = pathsHistory.begin();
    
    for (; p != pathsHistory.end(); p++) {
      cout << "path = ";
      size_t last = p->size();
      for (size_t i = 0; i < last; i++) {
        if (i>0) {
          cout << ",";
        }
        cout << p->at(i);
      }
      cout << endl;
    }
  }
}
