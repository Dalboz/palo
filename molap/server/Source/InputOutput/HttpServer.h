////////////////////////////////////////////////////////////////////////////////
/// @brief http server
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

#ifndef INPUT_OUTPUT_HTTP_SERVER_H
#define INPUT_OUTPUT_HTTP_SERVER_H 1

#include "palo.h"

#include <string>
#include <map>
#include <set>
#include <fstream>

#include "InputOutput/ListenTask.h"

namespace palo {
  using namespace std;

  class HttpRequest;
  class HttpRequestHandler;
  class HttpResponse;
  class HttpServerTask;
  class Scheduler;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief http server
  ///
  /// The http server is implemented as a listener task. It listens on a port
  /// for a client connection. As soon as a client requests a connection the
  /// function handleConnected is called. It will then create a new read/write
  /// task used to communicate with the client.
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS HttpServer : public ListenTask {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief constructs a new http server
    ///
    /// Constructs a new http server. The port to which the http server listens
    /// must be specified when adding the task to the scheduler.
    ////////////////////////////////////////////////////////////////////////////////

    HttpServer (Scheduler*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes a http server
    ////////////////////////////////////////////////////////////////////////////////

    ~HttpServer ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets a new trace file
    ////////////////////////////////////////////////////////////////////////////////

    void setTraceFile (const string& name) {
      if (trace != 0) {
        trace->close();
        delete trace;
        trace = 0;
      }

      trace = new ofstream(name.c_str());

      if (! *trace) {
        Logger::error << "cannot open trace file '" << name << "'" << endl;
        delete trace;
        trace = 0;
      }
      else {
        Logger::info << "using trace file '" << name << "'" << endl;
      }
    }

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief adds a new request handler
    ///
    /// The functions add a new request handler for the path given. If the http
    /// server receives an request with this path it calls the
    /// HttpRequestHandler::handleHttpRequest method of the handler. This method
    /// must a return a suitable HttpResponse object.
    ////////////////////////////////////////////////////////////////////////////////

    void addHandler (const string& path, HttpRequestHandler*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief adds a default request handler
    ///
    /// The functions add a new request handler which is called in cases where
    /// no request handler has been defined for the given path. The http
    /// server calls the HttpRequestHandler::handleHttpRequest method of the
    /// handler. This method must a return a suitable HttpResponse object.
    ////////////////////////////////////////////////////////////////////////////////

    void addNotFoundHandler (HttpRequestHandler*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief handles client hangup
    ///
    /// The callback functions is called by the HttpServerTask if the client has
    /// closed the connection. The HttpServerTask will immediately return after
    /// calling handleClientHangup, so it is possible to delete the HttpServerTask
    /// inside this callback.
    ////////////////////////////////////////////////////////////////////////////////

    void handleClientHangup (HttpServerTask*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief handles client request
    ///
    /// The callback functions is called by the HttpServerTask if the client has
    /// received a new, syntactically valid request.
    ////////////////////////////////////////////////////////////////////////////////

    HttpResponse * handleRequest (HttpServerTask*, HttpRequest*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief handles malformed client request
    ///
    /// The callback functions is called by the HttpServerTask if the client has
    /// received a syntactically invalid request.
    ////////////////////////////////////////////////////////////////////////////////

    void handleIllegalRequest (HttpServerTask*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief handles semaphore raised
    ////////////////////////////////////////////////////////////////////////////////

    HttpResponse * handleSemaphoreRaised (semaphore_t, HttpServerTask*, HttpRequest*, IdentifierType, const string*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief handles semaphore deleted
    ////////////////////////////////////////////////////////////////////////////////

    HttpResponse * handleSemaphoreDeleted (semaphore_t, HttpServerTask*, HttpRequest*, IdentifierType);

  public:
    void handleConnected (socket_t);
    void handleShutdown ();

  private:
    Scheduler * scheduler;

    ofstream * trace;

    map<string, HttpRequestHandler*> handlers;
    HttpRequestHandler*              notFoundHandler;

    set<HttpServerTask*> tasks;
  };

}

#endif

