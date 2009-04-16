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

#include "InputOutput/HttpServerTask.h"

#include <iostream>

#include "InputOutput/HttpRequest.h"
#include "InputOutput/HttpResponse.h"
#include "InputOutput/HttpServer.h"
#include "InputOutput/Logger.h"
#include "InputOutput/Scheduler.h"

namespace palo {
  using namespace std;


  
  HttpServerTask::HttpServerTask (socket_t fd, HttpServer* server, Scheduler* scheduler)
    : IoTask(fd, fd),
      ReadTask(fd),
      WriteTask(fd),
      ReadWriteTask(fd, fd), 
      scheduler(scheduler),
      server(server),
      readPosition(0) {
    httpRequest = 0;
    readRequestBody = false;
  }



  HttpServerTask::~HttpServerTask () {
    for (deque<StringBuffer*>::iterator i = writeBuffers.begin();  i != writeBuffers.end();  i++) {
      StringBuffer * buffer = *i;

      buffer->free();

      delete buffer;
    }
    
    if (httpRequest != 0) {
      delete httpRequest;
    }
    
    writeBuffers.clear();
  }



  bool HttpServerTask::handleRead () {
    bool res = fillReadBuffer();

    if (! res) {
      return false;
    }

    return processRead();
  }



  bool HttpServerTask::processRead () {
    bool handleRequest = false;

    if (! readRequestBody) {
      const char * ptr = readBuffer.c_str() + readPosition;
      const char * end = readBuffer.end() - 3;

      for (;  ptr < end;  ptr++) {
        if (ptr[0] == '\r' && ptr[1] == '\n' && ptr[2] == '\r' && ptr[3] == '\n') {
          break;
        }
      }

      if (ptr < end) {
        readPosition = ptr - readBuffer.c_str() + 4;
        
        if (httpRequest != 0) {
          delete httpRequest;
        }

        httpRequest = extractRequest();

        switch (httpRequest->getRequestType()) {
          case HttpRequest::HTTP_REQUEST_GET:
            handleRequest = true;
            break;

          case HttpRequest::HTTP_REQUEST_POST:
            bodyLength = httpRequest->getContentLegth();

            if (bodyLength > 0) {
              readRequestBody = true;
            }
            else {
              handleRequest = true;
            }
            break;

          default:
            Logger::warning << "got corrupted HTTP request" << endl;
            server->handleIllegalRequest(this);
            return true;
        }

      }
      else {
        if (readBuffer.c_str() < end) {
          readPosition = end - readBuffer.c_str();
        }
      }
    }
    
    // readRequestBody might have changed, so cannot use else
    if (readRequestBody) {
      if (readBuffer.length() < bodyLength) {
        return true;
      }      

      // read "bodyLength" from read buffer and add this body to "httpRequest"      
      string body(readBuffer.c_str(), bodyLength);
      httpRequest->setBody(body);

      // remove body from read buffer and reset read position
      readBuffer.erase_front(bodyLength);
      readPosition = 0;
      readRequestBody = false;
      handleRequest = true;
    }

    if (handleRequest) {
      HttpResponse * response = server->handleRequest(this, httpRequest);

      if (response->isDeferred()) {
        scheduler->waitOnSemaphore(response->getSemaphore(), this, response->getClientData());
      }
      else {
        addResponse(response);
      }

      delete response;
    }

    return true;
  }



  HttpRequest * HttpServerTask::extractRequest () {
    string request(readBuffer.c_str(), readPosition);

    readBuffer.erase_front(readPosition);
    readPosition = 0;

    return new HttpRequest(request);
  }



  void HttpServerTask::handleShutdown () {
    Logger::trace << "beginning shutdown of connection on " << readSocket << endl;
    server->handleClientHangup(this);
  }



  void HttpServerTask::handleHangup (socket_t fd) {
    Logger::trace << "got hangup on socket " << fd << endl;
    server->handleClientHangup(this);
  }



  void HttpServerTask::handleSemaphoreRaised (semaphore_t semaphore, IdentifierType data, const string* msg) {
    HttpResponse * response = server->handleSemaphoreRaised(semaphore, this, httpRequest, data, msg);

    if (response->isDeferred()) {
      scheduler->waitOnSemaphore(response->getSemaphore(), this, response->getClientData());
    }
    else {
      addResponse(response);
    }

    delete response;
  }



  void HttpServerTask::handleSemaphoreDeleted (semaphore_t semaphore, IdentifierType data) {
    HttpResponse * response = server->handleSemaphoreDeleted(semaphore, this, httpRequest, data);

    if (response->isDeferred()) {
      scheduler->waitOnSemaphore(response->getSemaphore(), this, response->getClientData());
    }
    else {
      addResponse(response);
    }

    delete response;
  }



  void HttpServerTask::addResponse (HttpResponse* response) {
    StringBuffer * buffer;

    // save header
    buffer = new StringBuffer();
    buffer->initialize();
    buffer->replaceText(response->getHeader());
    buffer->appendText(response->getBody());

    writeBuffers.push_back(buffer);

    // clear body
    response->getBody().clear();

    // start output
    fillWriteBuffer();
  }



  void HttpServerTask::completedWriteBuffer () {
    fillWriteBuffer();
  }



  void HttpServerTask::fillWriteBuffer () {
    if (! hasWriteBuffer() && ! writeBuffers.empty()) {
      StringBuffer * buffer = writeBuffers.front();
      writeBuffers.pop_front();

      setWriteBuffer(buffer);
    }
  }
}
