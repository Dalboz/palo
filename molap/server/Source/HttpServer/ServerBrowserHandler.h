////////////////////////////////////////////////////////////////////////////////
/// @brief server browser
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

#ifndef HTTP_SERVER_SERVER_BROWSER_HANDLER_H
#define HTTP_SERVER_SERVER_BROWSER_HANDLER_H 1

#include "palo.h"

#include <string>
#include <iostream>

#include "HttpServer/PaloHttpBrowserHandler.h"
#include "HttpServer/ServerBrowserDocumentation.h"
#include "Exceptions/ParameterException.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief server browser
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS ServerBrowserHandler : public PaloHttpBrowserHandler {
  public:
    ServerBrowserHandler (Server * server, const string &path, Scheduler* scheduler) 
      : PaloHttpBrowserHandler(server, path), scheduler(scheduler) {
    }

  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      string message;
      
      string action = request->getValue("action");

      if (! action.empty()) {
        try {
          if (action == "load") {
            Database* database = findDatabase(request, 0, false);
            server->loadDatabase(database, user);
            message = "database loaded";
          }
          else if (action == "save") {
            Database* database = findDatabase(request, 0);
            server->saveServer(user);
            server->saveDatabase(database, user);
            message = "database saved";
          }
          else if (action == "delete") {
            Database* database = findDatabase(request, 0);
            server->deleteDatabase(database, user);
            message = "database deleted";
          }
          else if (action == "unload") {
            Database* database = findDatabase(request, 0);
            server->unloadDatabase(database, user);
            message = "database unloaded";
          }
          else if (action == "load_server") {
            server->loadServer(user);
            message = "server loaded";
          }
          else if (action == "save_server") {
            server->saveServer(user);
            message = "server saved";
          }
          else if (action == "shutdown_server") {
            server->beginShutdown(0);
            message = "shutdown server";
          }

          if (message != "") {
            message = createHtmlMessage("Info", message);
          }
        }
        catch (ErrorException e) {
          message = createHtmlMessage("Error", e.getMessage());
        }
      }
      
      HttpResponse * response = new HttpResponse(HttpResponse::OK);

      ServerBrowserDocumentation sbd(server, message);
      HtmlFormatter hf(templatePath + "/browser_server.tmpl");

      response->getBody().copy(hf.getDocumentation(&sbd));
      response->setContentType("text/html;charset=utf-8");
    
      return response;
    }

  private:
    Scheduler* scheduler;

  };
}

#endif
