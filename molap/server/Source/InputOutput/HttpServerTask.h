////////////////////////////////////////////////////////////////////////////////
/// @brief read/write task of a http server
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

#ifndef INPUT_OUTPUT_HTTP_SERVER_TASK_H
#define INPUT_OUTPUT_HTTP_SERVER_TASK_H 1

#include "palo.h"

#include <deque>

#include "InputOutput/ReadWriteTask.h"
#include "InputOutput/SemaphoreTask.h"

namespace palo {
  using namespace std;

  class HttpRequest;
  class HttpResponse;
  class HttpServer;
  class Scheduler;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief read/write task of a http server
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS HttpServerTask : public ReadWriteTask, public SemaphoreTask {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief constructs a new read/write task of a http server
    ///
    /// The http server constructs a HttpServerTask for each client
    /// connection. This task is responsible for dealing for the input/output
    /// to/from the client.
    ////////////////////////////////////////////////////////////////////////////////

    HttpServerTask (socket_t fd, HttpServer*, Scheduler*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes a read/write task of a http server
    ////////////////////////////////////////////////////////////////////////////////

    ~HttpServerTask ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief adds a response to the client
    ///
    /// This function is called by the HttpServer to add a new HttpResponse
    /// which must be delivered to the client.
    ////////////////////////////////////////////////////////////////////////////////

    void addResponse (HttpResponse*);

  public:
    bool handleRead ();
    void handleHangup (socket_t fd);
    void handleShutdown ();
    void handleSemaphoreRaised (semaphore_t, IdentifierType, const string*);
    void handleSemaphoreDeleted (semaphore_t, IdentifierType);
    
  protected:
    void completedWriteBuffer ();
    bool processRead ();

  private:
    HttpRequest * extractRequest ();
    void fillWriteBuffer ();

  private:
    Scheduler * scheduler;
    HttpServer * server;
    deque<StringBuffer*> writeBuffers;
    size_t readPosition;
    
    HttpRequest * httpRequest;
    bool readRequestBody;
    size_t bodyLength;
  };

}

#endif
