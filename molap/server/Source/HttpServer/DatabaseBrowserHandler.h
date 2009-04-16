////////////////////////////////////////////////////////////////////////////////
/// @brief database browser
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

#ifndef HTTP_SERVER_DATABASE_BROWSER_HANDLER_H
#define HTTP_SERVER_DATABASE_BROWSER_HANDLER_H 1

#include "palo.h"

#include <string>
#include <iostream>

#include "InputOutput/HtmlFormatter.h"

#include "Exceptions/ParameterException.h"

#include "HttpServer/PaloHttpBrowserHandler.h"
#include "HttpServer/DatabaseBrowserDocumentation.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief database browser
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS DatabaseBrowserHandler : public PaloHttpBrowserHandler {
    
  public:
    DatabaseBrowserHandler (Server * server, const string& path) 
      : PaloHttpBrowserHandler(server, path) {
    }

  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      string message;      
      Database* database = findDatabase(request, 0, false);

      string action = request->getValue("action");

      if (! action.empty()) {
        try {
          if (action == "load") {
            string idString = request->getValue(ID_CUBE);
            IdentifierType id = StringUtils::stringToInteger(idString);
            database->loadCube(database->findCube(id, user, false), user);
            message = "cube loaded";
          }
          else if (action == "save") {
            Cube* cube = findCube(database, request, 0);
            server->saveServer(0);
            server->saveDatabase(database, 0);
            database->saveCube(cube, 0);
            message = "cube saved";
          }
          else if (action == "delete") {
            Cube* cube = findCube(database, request, 0);
            database->deleteCube(cube, 0);
            message = "cube deleted";
          }
          else if (action == "unload") {
            Cube* cube = findCube(database, request, 0);
            database->unloadCube(cube, 0);
            message = "cube unloaded";
          }
          else if (action == "load_database") {
            server->loadDatabase(database, 0);
            message = "database loaded";
          }
          else if (action == "save_database") {
            server->saveServer(0);
            server->saveDatabase(database, 0);
            message = "database saved";
          }
          else if (action == "delete_dimension") {
            Dimension* dimension = findDimension(database, request, 0);
            database->deleteDimension(dimension, 0);
            message = "dimension deleted";
          }
          
          if (! message.empty()) {
            message = createHtmlMessage( "Info", message);
          }
        }
        catch (ErrorException e) {
          message = createHtmlMessage( "Error", e.getMessage());
        }
      }

      HttpResponse * response = new HttpResponse(HttpResponse::OK);

      DatabaseBrowserDocumentation sbd(database, message);
      HtmlFormatter hf(templatePath + "/browser_database.tmpl");

      response->getBody().copy(hf.getDocumentation(&sbd));
      response->setContentType("text/html;charset=utf-8");
    
      return response;
    }
  };

}

#endif
