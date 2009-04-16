////////////////////////////////////////////////////////////////////////////////
/// @brief legacy server
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

#ifndef LEGACY_SERVER_LEGACY_SERVER_H
#define LEGACY_SERVER_LEGACY_SERVER_H 1

#include "palo.h"

#include <string>
#include <map>
#include <set>

#include "Collections/AssociativeArray.h"
#include "Collections/StringUtils.h"

#include "InputOutput/ListenTask.h"

namespace palo {
  using namespace std;

  class LegacyRequestHandler;
  class LegacyServer;
  class LegacyServerTask;
  class Scheduler;
  class Server;

  /////////////////////
  // static constructor
  /////////////////////

  SERVER_FUNC LegacyServer * ConstructPaloLegacyServer (Scheduler * scheduler,
                                                          Server * server,
                                                          const string& address,
                                                          int port);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief legacy server
  ///
  /// The legacy server is implemented as a listener task. It listens on a port
  /// for a client connection. As soon as a client requests a connection the
  /// function handleConnected is called. It will then create a new read/write
  /// task used to communicate with the client.
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS LegacyServer : public ListenTask {
  private:
    struct NameHandlerPair {
      NameHandlerPair (const string& name, LegacyRequestHandler* handler) 
        : name(name), handler(handler) {
      }

      const string name;
      LegacyRequestHandler * handler;
    };

    struct Name2HandlerDesc {
      uint32_t hash (const string& name) {
        return StringUtils::hashValue(name.c_str(), name.size());
      }

      bool isEmptyElement (NameHandlerPair * const & handler) {
        return handler == 0;
      }

      uint32_t hashElement (NameHandlerPair * const & handler) {
        return hash(handler->name);
      }

      uint32_t hashKey (const string& key) {
        return hash(key);
      }

      bool isEqualElementElement (NameHandlerPair * const & left, NameHandlerPair * const & right) {
        return left->name == right->name;
      }

      bool isEqualKeyElement (const string& key, NameHandlerPair * const & handler) {
        const string& name = handler->name;

        return key.size() == name.size() && memcmp(key.c_str(), name.c_str(), key.size()) == 0;
      }

      void clearElement (NameHandlerPair * & handler) {
        handler = 0;
      }
    };

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief constructs a new legacy server
    ///
    /// Constructs a new legacy server. The port to which the legacy server listens
    /// must be specified when adding the task to the scheduler.
    ////////////////////////////////////////////////////////////////////////////////

    LegacyServer (Server*, Scheduler*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes a legacy server
    ////////////////////////////////////////////////////////////////////////////////

    ~LegacyServer ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets server
    ////////////////////////////////////////////////////////////////////////////////

    Server * getServer () const {
      return server;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets scheduler
    ////////////////////////////////////////////////////////////////////////////////

    Scheduler * getScheduler () const {
      return scheduler;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief adds a new request handler
    ////////////////////////////////////////////////////////////////////////////////

    void addHandler (const string& name, LegacyRequestHandler*);


    ////////////////////////////////////////////////////////////////////////////////
    /// @brief lookups a request handler by name
    ////////////////////////////////////////////////////////////////////////////////

    LegacyRequestHandler * lookupHandler (const string& name) {
      NameHandlerPair * nhp = handlers.findKey(name);

      return nhp == 0 ? notFoundHandler : nhp->handler;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns getdata handler
    ////////////////////////////////////////////////////////////////////////////////

    LegacyRequestHandler * lookupGetdataHandler () {
      return getdataHandler;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns setdata handler
    ////////////////////////////////////////////////////////////////////////////////

    LegacyRequestHandler * lookupSetdataHandler () {
      return setdataHandler;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief handles client hangup
    ///
    /// The callback functions is called by the LegacyServerTask if the client has
    /// closed the connection. The LegacyServerTask will immediately return after
    /// calling handleClientHangup, so it is possible to delete the LegacyServerTask
    /// inside this callback.
    ////////////////////////////////////////////////////////////////////////////////

    void handleClientHangup (LegacyServerTask*);

  public:
    void handleConnected (socket_t fd);
    void handleShutdown ();

  private:
    Server * server;
    Scheduler * scheduler;

    AssociativeArray<string, NameHandlerPair*, Name2HandlerDesc> handlers;

    LegacyRequestHandler * notFoundHandler;
    LegacyRequestHandler * getdataHandler;
    LegacyRequestHandler * setdataHandler;

    set<LegacyServerTask*> tasks;

    semaphore_t semaphore; // DUMMY for logout
  };

}

#endif

