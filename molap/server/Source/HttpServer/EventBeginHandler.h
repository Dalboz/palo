////////////////////////////////////////////////////////////////////////////////
/// @brief push rule begin
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

#ifndef HTTP_SERVER_EVENT_BEGIN_HANDLER_H
#define HTTP_SERVER_EVENT_BEGIN_HANDLER_H 1

#include "palo.h"

#include <string>

#include "Olap/Cube.h"
#include "Olap/PaloSession.h"

#include "HttpServer/PaloHttpHandler.h"
#include "HttpServer/PaloSemaphoreHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief database info
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS EventBeginHandler : public PaloHttpHandler, public PaloSemaphoreHandler {
  public:
    EventBeginHandler (Server * server, Scheduler * scheduler) 
      : PaloHttpHandler(server) {

      // create a singleton
      this->scheduler = scheduler;
    }



  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      checkToken(server, request);
      return handlePaloSemaphoreRaised(request, session->getIdentifier(), 0);
    }



    HttpResponse * handlePaloSemaphoreRaised (const HttpRequest * request, IdentifierType clientData, const string* msg) {
      PaloSession * session = PaloSession::findSession(clientData);

      if (server->isBlocking()) {
        if (session->getIdentifier() == server->getActiveSession()) {
          throw ParameterException(ErrorException::ERROR_WITHIN_EVENT,
                                   "session already within event block",
                                   SESSION, request->getValue(SESSION));
        }

        Logger::debug << "waiting for current event to finish" << endl;

        return new HttpResponse(server->getSemaphore(), clientData);
      }
      else {
        const string& source = request->getValue(SOURCE);
        PaloSession * sourceSession = findSession(source);
        User * sourceUser = sourceSession->getUser();

        server->setUsername(sourceUser == 0 ? "" : sourceUser->getName());
        server->setEvent(request->getValue(EVENT));
        server->setBlocking(true);
        server->setActiveSession(session->getIdentifier());

        Logger::info << "starting transaction for user " << server->getRealUsername() << endl;

        return generateOkResponse(server);
      }
    }



    HttpResponse * handlePaloSemaphoreDeleted (const HttpRequest * request, IdentifierType clientData) {
      throw ErrorException(ErrorException::ERROR_INTERNAL, "semaphore failed");
    }



    void releaseTransaction (PaloSession * session) {
      if (! server->isBlocking()) {
        throw ParameterException(ErrorException::ERROR_NOT_WITHIN_EVENT,
                                 "missing event/begin",
                                 SESSION, (int) session->getIdentifier());
      }

      if (session->getIdentifier() != server->getActiveSession()) {
        throw ErrorException(ErrorException::ERROR_INTERNAL, "session mismatch in event");
      }

      Logger::info << "finished transaction for user " << server->getRealUsername() << endl;

      server->setUsername("");
      server->setEvent("");
      server->setBlocking(false);
      server->setActiveSession(0);

      scheduler->raiseAllSemaphore(server->getSemaphore(), 0);
    }



  private:
    static Scheduler * scheduler;
  };

}

#endif
