////////////////////////////////////////////////////////////////////////////////
/// @brief palo server logout handler
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

#ifndef HTTP_SERVER_SERVER_LOGOUT_HANDLER_H
#define HTTP_SERVER_SERVER_LOGOUT_HANDLER_H 1

#include "palo.h"

#include <string>
#include <iostream>

#include "Exceptions/ParameterException.h"

#include "Olap/Server.h"

#include "HttpServer/PaloHttpHandler.h"
#include "HttpServer/EventBeginHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief logout to a palo server
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS ServerLogoutHandler : public PaloHttpHandler, public PaloSemaphoreHandler {
  public:
    ServerLogoutHandler (Server * server, Scheduler * scheduler, EventBeginHandler * handler) 
      : PaloHttpHandler(server), handler(handler), scheduler(scheduler) {
      semaphore = scheduler->createSemaphore();
    }

  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      checkToken(server, request);

      if (Server::isBlocking()) {
        if (session->getIdentifier() == Server::getActiveSession()) {
          handler->releaseTransaction(session);
        }
      }

      PaloSession::deleteSession(session);

      session = 0;

      LoginWorker* loginWorker = server->getLoginWorker();

      if (loginWorker) {
        if (user) {
          loginWorker->executeLogout(semaphore, user->getName());
        }
        else {
          loginWorker->executeLogout(semaphore, "<unknown>");
        }

        return new HttpResponse(semaphore, 0);
      }            

      return generateOkResponse(server);
    }


    HttpResponse * handlePaloSemaphoreDeleted (const HttpRequest * request, IdentifierType clientData) {
      throw ErrorException(ErrorException::ERROR_INTERNAL, "semaphore failed");
    }



    HttpResponse * handlePaloSemaphoreRaised (const HttpRequest * request, IdentifierType clientData, const string* msg) {
      return generateOkResponse(server);
    }


  private:
    EventBeginHandler * handler;
    Scheduler * scheduler;
    semaphore_t semaphore;
  };
}

#endif
