////////////////////////////////////////////////////////////////////////////////
/// @brief cell replace
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

#ifndef HTTP_SERVER_CELL_REPLACE_HANDLER_H
#define HTTP_SERVER_CELL_REPLACE_HANDLER_H 1

#include "palo.h"

#include <string>
#include <iostream>

#include "Exceptions/ParameterException.h"

#include "Olap/CellPath.h"
#include "Olap/Dimension.h"
#include "Olap/Lock.h"
#include "Olap/PaloSession.h"
#include "Olap/Server.h"

#include "HttpServer/PaloHttpHandler.h"
#include "HttpServer/PaloSemaphoreHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief cell replace
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CellReplaceHandler : public PaloHttpHandler, public PaloSemaphoreHandler {
    
  public:
    CellReplaceHandler (Server * server) 
      : PaloHttpHandler(server) {
    }

  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      Database* database = findDatabase(request, 0);
      Cube* cube = findCube(database, request, 0);

      checkToken(cube, request);
      
      // get splash mode
      Cube::SplashMode mode = getSplashMode(request);

      // get path
      IdentifiersType path = getPath(request, cube->getDimensions());
      
      // get value
      const string& valueString = request->getValue(VALUE);

      // get "add" flag, true means add value instead of set
      bool addFlag = getBoolean(request, ADD, false);

      if (addFlag) {
        if (mode != Cube::DISABLED && mode != Cube::ADD_BASE && mode != Cube::DEFAULT) {
          throw ParameterException(ErrorException::ERROR_INVALID_SPLASH_MODE,
                                   "add=1 requires splash mode DRFAULT, DISABLED, or ADD",
                                   "splash mode", (int) mode);
        }
      }

      // get event-process flag, false means to circumvent the processor (right will be checked by cube)
      bool eventProcessor = getBoolean(request, EVENT_PROCESSOR, true);

      // construct cell path
      CellPath cp(cube, &path);

      // try to change the cell value
      semaphore_t semaphore = 0;
      bool withinEvent = Server::isBlocking();

      if (! withinEvent && session->isWorker()) {
        throw ParameterException(ErrorException::ERROR_NOT_WITHIN_EVENT,
                                 "worker cell/replace requires an event/begin",
                                 "session", session->getEncodedIdentifier());
      }
      
      // do not use the supervision event processor if within event or requested by user
      bool checkArea = eventProcessor && ! withinEvent;

      if (cp.getPathType() == Element::STRING) {
        semaphore = cube->setCellValue(&cp, valueString, user, session, checkArea, ! withinEvent, Lock::checkLock);
      }
      else {
        double value = valueString.empty() ? 0 : StringUtils::stringToDouble(valueString);

        semaphore = cube->setCellValue(&cp, value, user, session, checkArea, ! withinEvent, addFlag, mode, Lock::checkLock);
      }

      if (semaphore != 0) {
        return new HttpResponse(semaphore, 0);
      }
      else {
        return generateOkResponse(cube);
      }
    }



    HttpResponse * handlePaloSemaphoreRaised (const HttpRequest * request, IdentifierType data, const string* msg) {
      
      if (msg) {
        HttpResponse * response = new HttpResponse(HttpResponse::BAD);        
        StringBuffer& body = response->getBody();
        appendCsvInteger(&body, (int32_t) ErrorException::ERROR_WORKER_MESSAGE);
        body.appendText(*msg); 
        body.appendEol();
        Logger::warning << "error code: " << ErrorException::ERROR_WORKER_MESSAGE << " description: " << *msg << endl;          
        return response;
      }
      
      Database* database = findDatabase(request, 0);
      Cube* cube = findCube(database, request, 0);

      return generateOkResponse(cube);
    }



    HttpResponse * handlePaloSemaphoreDeleted (const HttpRequest * request, IdentifierType data) {
      Database* database = findDatabase(request, 0);
      Cube* cube = findCube(database, request, 0);

      throw ParameterException(ErrorException::ERROR_INTERNAL,
                               "event semaphore deleted",
                               "cube", cube->getName());
    }
  };

}

#endif
