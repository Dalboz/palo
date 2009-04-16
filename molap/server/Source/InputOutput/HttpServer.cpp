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

#include "InputOutput/HttpServer.h"

#include <iostream>

#include "Collections/StringBuffer.h"

#include "InputOutput/HttpRequest.h"
#include "InputOutput/HttpRequestHandler.h"
#include "InputOutput/HttpResponse.h"
#include "InputOutput/HttpServerTask.h"
#include "InputOutput/Logger.h"
#include "InputOutput/Scheduler.h"

namespace palo {
  using namespace std;



  HttpServer::HttpServer (Scheduler* scheduler)
    : IoTask(INVALID_SOCKET, INVALID_SOCKET), scheduler(scheduler), trace(0) {
      notFoundHandler = 0;
  }



  HttpServer::~HttpServer () {
    for (set<HttpServerTask*>::iterator i = tasks.begin();  i != tasks.end();  i++) {
      HttpServerTask * task = *i;

      scheduler->removeIoTask(task);

      closesocket(task->getReadSocket());
    }
    
    for (map<string, HttpRequestHandler*>::iterator i = handlers.begin();  i != handlers.end();  i++) {
      HttpRequestHandler* requestHandler  = i->second;
      delete requestHandler;
    }

    if (trace != 0) {
      trace->close();
      delete trace;
    }
  }



  void HttpServer::addHandler (const string& path, HttpRequestHandler* handler) {
    handlers[path] = handler;
  }



  void HttpServer::addNotFoundHandler (HttpRequestHandler* handler) {
    notFoundHandler = handler;
  }



  void HttpServer::handleConnected (socket_t fd) {
    HttpServerTask * task = new HttpServerTask(fd, this, scheduler);

    bool success = scheduler->addIoTask(task);

    if (success) {
      tasks.insert(task);
      Logger::info << "got new client connection on " << fd << endl;
    }
    else {
      delete task;
      closesocket(fd);
    }
  }



  void HttpServer::handleClientHangup (HttpServerTask * task) {
    Logger::info << "lost client connection on " << task->getReadSocket() << endl;

    scheduler->removeIoTask(task);
    
    closesocket(task->getReadSocket());
    tasks.erase(task);
  }



  void HttpServer::handleShutdown () {
    Logger::trace << "beginning shutdown of listener on " << readSocket << endl;

    closesocket(readSocket);

    scheduler->removeListenTask(this);
  }



  HttpResponse * HttpServer::handleRequest (HttpServerTask* task, HttpRequest* request) {
    const string& path = request->getRequestPath();
    // map<string, HttpRequestHandler*>::iterator iter = handlers.find(path);
    map<string, HttpRequestHandler*>::iterator iter;
    
    // delete last '/' from request
    if (!path.empty() && *(path.rbegin()) == '/') {
      string tmpPath = path.substr(0, path.find_last_not_of("/")+1);
      if (tmpPath.empty()) {
        tmpPath = "/";
      }
      iter = handlers.find(tmpPath);
    }
    else {
      iter = handlers.find(path);
    }
    
    HttpResponse * response = 0;

    if (iter != handlers.end()) {
      response = iter->second->handleHttpRequest(request);
    }
    else {
      if (notFoundHandler != 0) {
        response = notFoundHandler->handleHttpRequest(request);
      }
      else {
        response = new HttpResponse(HttpResponse::NOT_IMPLEMENTED);
      }
      Logger::warning << "no http request handler for '" << path << "' found" << endl;
    }


    if (trace) {
      string method = "UNKNOWN";
      
      switch (request->getRequestType()) {
        case HttpRequest::HTTP_REQUEST_GET:
          method = "GET";
          break;

        case HttpRequest::HTTP_REQUEST_POST:
          method = "PUT";
          break;
      }

      string keyString = "";
      const vector<string>& keys = request->getKeys();

      for (vector<string>::const_iterator iter = keys.begin();  iter != keys.end();  iter++) {
        if (! keyString.empty()) {
          keyString += "&";
        }

        keyString += *iter + "=" + request->getValue(*iter);
      }

      *trace << method << " " << path << (keyString.empty() ? "" : "?") << keyString << "\n"
             << response->getBody().c_str() << endl;
    }

    return response;
  }




  HttpResponse * HttpServer::handleSemaphoreRaised (semaphore_t semaphore, HttpServerTask* task, HttpRequest* request, IdentifierType data, const string* msg) {
    const string& path = request->getRequestPath();
    map<string, HttpRequestHandler*>::iterator iter = handlers.find(path);

    HttpResponse * response = 0;

    if (iter != handlers.end()) {
      response = iter->second->handleSemaphoreRaised(semaphore, request, data, msg);
    }
    else {
      if (notFoundHandler != 0) {
        response = notFoundHandler->handleHttpRequest(request);
      }
      else {
        response = new HttpResponse(HttpResponse::NOT_IMPLEMENTED);
      }

      Logger::warning << "no http request handler for '" << path << "' found" << endl;
    }

    return response;
  }




  HttpResponse * HttpServer::handleSemaphoreDeleted (semaphore_t semaphore, HttpServerTask* task, HttpRequest* request, IdentifierType data) {
    const string& path = request->getRequestPath();
    map<string, HttpRequestHandler*>::iterator iter = handlers.find(path);

    HttpResponse * response = 0;

    if (iter != handlers.end()) {
      response = iter->second->handleSemaphoreDeleted(semaphore, request, data);
    }
    else {
      if (notFoundHandler != 0) {
        response = notFoundHandler->handleHttpRequest(request);
      }
      else {
        response = new HttpResponse(HttpResponse::NOT_IMPLEMENTED);
      }

      Logger::warning << "no http request handler for '" << path << "' found" << endl;
    }

    return response;
  }




  void HttpServer::handleIllegalRequest (HttpServerTask* task) {
    scheduler->removeIoTask(task);

    closesocket(task->getReadSocket());
    tasks.erase(task);
  }
}
