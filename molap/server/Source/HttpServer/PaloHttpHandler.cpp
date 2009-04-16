////////////////////////////////////////////////////////////////////////////////
/// @brief http handler for palo objects
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

#include "HttpServer/PaloHttpHandler.h"

extern "C" {
#include <time.h>
}

#include "Exceptions/ParameterException.h"

#include "Collections/StringBuffer.h"

#include "InputOutput/DocumentationHandler.h"
#include "InputOutput/Statistics.h"
#include "InputOutput/StatisticsDocumentation.h"

#include "HttpServer/CellAreaHandler.h"
#include "HttpServer/CellAreaCounterHandler.h"
#include "HttpServer/CellCopyHandler.h"
#include "HttpServer/CellDrillThroughHandler.h"
#include "HttpServer/CellExportHandler.h"
#include "HttpServer/CellGoalSeekHandler.h"
#include "HttpServer/CellReplaceBulkHandler.h"
#include "HttpServer/CellReplaceHandler.h"
#include "HttpServer/CellValueHandler.h"
#include "HttpServer/CellValuesHandler.h"
#include "HttpServer/CubeBrowserHandler.h"
#include "HttpServer/CubeClearCacheHandler.h"
#include "HttpServer/CubeClearHandler.h"
#include "HttpServer/CubeCommitHandler.h"
#include "HttpServer/CubeCreateHandler.h"
#include "HttpServer/CubeDeleteHandler.h"
#include "HttpServer/CubeInfoHandler.h"
#include "HttpServer/CubeLoadHandler.h"
#include "HttpServer/CubeLockHandler.h"
#include "HttpServer/CubeLocksHandler.h"
#include "HttpServer/CubeRenameHandler.h"
#include "HttpServer/CubeRollbackHandler.h"
#include "HttpServer/CubeRulesHandler.h"
#include "HttpServer/CubeSaveHandler.h"
#include "HttpServer/CubeUnloadHandler.h"
#include "HttpServer/DatabaseCubesHandler.h"
#include "HttpServer/DatabaseBrowserHandler.h"
#include "HttpServer/DatabaseCreateHandler.h"
#include "HttpServer/DatabaseDeleteHandler.h"
#include "HttpServer/DatabaseInfoHandler.h"
#include "HttpServer/DatabaseLoadHandler.h"
#include "HttpServer/DatabaseRenameHandler.h"
#include "HttpServer/DatabaseSaveHandler.h"
#include "HttpServer/DatabaseUnloadHandler.h"
#include "HttpServer/ServerDatabasesHandler.h"
#include "HttpServer/DimensionBrowserHandler.h"
#include "HttpServer/DimensionClearHandler.h"
#include "HttpServer/DimensionCreateHandler.h"
#include "HttpServer/DimensionDeleteHandler.h"
#include "HttpServer/DimensionElementHandler.h"
#include "HttpServer/DimensionElementsHandler.h"
#include "HttpServer/DimensionInfoHandler.h"
#include "HttpServer/DimensionCubesHandler.h"
#include "HttpServer/DimensionRenameHandler.h"
#include "HttpServer/DatabaseDimensionsHandler.h"
#include "HttpServer/ElementAppendHandler.h"
#include "HttpServer/ElementBrowserHandler.h"
#include "HttpServer/ElementCreateHandler.h"
#include "HttpServer/ElementCreateBulkHandler.h"
#include "HttpServer/ElementDeleteHandler.h"
#include "HttpServer/ElementDeleteBulkHandler.h"
#include "HttpServer/ElementInfoHandler.h"
#include "HttpServer/ElementMoveHandler.h"
#include "HttpServer/ElementRenameHandler.h"
#include "HttpServer/ErrorCodeBrowserHandler.h"
#include "HttpServer/MemoryStatisticsBrowserHandler.h"
#include "HttpServer/EventBeginHandler.h"
#include "HttpServer/EventEndHandler.h"
#include "HttpServer/RuleBrowserHandler.h"
#include "HttpServer/RuleCreateHandler.h"
#include "HttpServer/RuleDeleteHandler.h"
#include "HttpServer/RuleFunctionsHandler.h"
#include "HttpServer/RuleInfoHandler.h"
#include "HttpServer/RuleModifyHandler.h"
#include "HttpServer/RuleParseHandler.h"
#include "HttpServer/ServerBrowserHandler.h"
#include "HttpServer/ServerInfoHandler.h"
#include "HttpServer/ServerLoadHandler.h"
#include "HttpServer/ServerLoginHandler.h"
#include "HttpServer/ServerLogoutHandler.h"
#include "HttpServer/ServerSaveHandler.h"
#include "HttpServer/ServerShutdownHandler.h"
#include "HttpServer/StatisticsClearHandler.h"
#include "HttpServer/StatisticsHandler.h"
#include "HttpServer/SvsInfoHandler.h"
#include "HttpServer/TimeStatisticsBrowserHandler.h"

#include "Olap/AttributesDimension.h"
#include "Olap/AttributesCube.h"
#include "Olap/CubeDimension.h"
#include "Olap/NormalCube.h"
#include "Olap/NormalDimension.h"
#include "Olap/RightsCube.h"
#include "Olap/PaloSession.h"
#include "Olap/SubsetViewDimension.h"
#include "Olap/SystemCube.h"
#include "Olap/SystemDimension.h"
#include "Olap/UserInfoDimension.h"

#if defined(ENABLE_HTTP_MODULE)
#include "Programs/extension.h"
#endif

namespace palo {
  using namespace std;

  /////////////////
  // local handlers
  /////////////////

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief api overview
  ////////////////////////////////////////////////////////////////////////////////

  class ApiOverviewHandler : public HttpRequestHandler {
  public:
    ApiOverviewHandler (const string& templateDirectory)
      : templateDirectory(templateDirectory) {
    }

  public:
    HttpResponse * handleHttpRequest (const HttpRequest * request) {
      HttpResponse * response = new HttpResponse(HttpResponse::OK);

      FileDocumentation fd(templateDirectory + "/api_overview.api");
      HtmlFormatter hd(templateDirectory + "/api_overview.tmpl");

      response->getBody().copy(hd.getDocumentation(&fd));
      response->setContentType("text/html;charset=utf-8");

      return response;
    }

  private:
    const string templateDirectory;
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief home
  ////////////////////////////////////////////////////////////////////////////////

  class HomeHandler : public HttpRequestHandler {
  public:
    HomeHandler (const string& templateDirectory)
      : templateDirectory(templateDirectory) {
    }

  public:
    HttpResponse * handleHttpRequest (const HttpRequest * request) {
      HttpResponse * response = new HttpResponse(HttpResponse::OK);

      FileDocumentation fd(templateDirectory + "/home.api");
      HtmlFormatter hd(templateDirectory + "/home.tmpl");

      response->getBody().copy(hd.getDocumentation(&fd));
      response->setContentType("text/html;charset=utf-8");

      return response;
    }

  private:
    const string templateDirectory;
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief file not found
  ////////////////////////////////////////////////////////////////////////////////

  class NotFoundHandler : public HttpRequestHandler {
  public:
    NotFoundHandler (const string& templateDirectory)
      : templateDirectory(templateDirectory) {
    }

  public:
    HttpResponse * handleHttpRequest (const HttpRequest * request) {
      const string& path = request->getRequestPath();
      HttpResponse * response;

      Logger::error << "PATH " << path << endl;
      if (path.find("/api") == 0 || path.find("/browser") == 0) {
        response = new HttpResponse(HttpResponse::NOT_FOUND);

        FileDocumentation fd(templateDirectory + "/home.api");
        HtmlFormatter hd(templateDirectory + "/not_found.tmpl");
        response->getBody().copy(hd.getDocumentation(&fd));
        response->setContentType("text/html;charset=utf-8");
      }
      else {
        response = new HttpResponse(HttpResponse::BAD);

        response->getBody().appendInteger( (uint32_t) ErrorException::ERROR_API_CALL_NOT_IMPLEMENTED);
        response->getBody().appendText(";"
          + StringUtils::escapeString(ErrorException::getDescriptionErrorType(ErrorException::ERROR_API_CALL_NOT_IMPLEMENTED)) + ";");
        response->getBody().appendText(StringUtils::escapeString("error in request"));
      }

      return response;
    }

  private:
    const string templateDirectory;
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief http disabled
  ////////////////////////////////////////////////////////////////////////////////

  class DisabledHandler : public HttpRequestHandler {
  public:
    HttpResponse * handleHttpRequest (const HttpRequest * request) {
      HttpResponse * response = new HttpResponse(HttpResponse::BAD);

      response->getBody().appendInteger( (uint32_t) ErrorException::ERROR_HTTP_DISABLED);
      response->getBody().appendText(";"
        + StringUtils::escapeString(ErrorException::getDescriptionErrorType(ErrorException::ERROR_HTTP_DISABLED)) + ";");
      response->getBody().appendText(StringUtils::escapeString("please use secure communication"));

      return response;
    }
  };

  /////////////////////
  // static constructor
  /////////////////////

  HttpServer * ConstructPaloHttpServer (Scheduler * scheduler,
                                        Server * server,
                                        const string& templateDirectory,
                                        const string& address,
                                        int port,
                                        bool admin,
                                        bool requireUserLogin,
                                        Encryption_e encryption,
                                        int https) {

    // need user login (currently only a global option!)
    PaloHttpHandler::setRequireUser(requireUserLogin);

    // construct a http server
    HttpServer * httpServer = new HttpServer(scheduler);

    // add all handlers
    AddPaloHttpHandlers(server, httpServer, scheduler, templateDirectory, admin, encryption, https,
                        (encryption == ENC_REQUIRED ? false : true));

    // open listen port and address
    bool ok;

    if (address.empty()) {
      ok = scheduler->addListenTask(httpServer, port);
    }
    else {
      ok = scheduler->addListenTask(httpServer, address, port);
    }

    if (! ok) {
      Logger::error << "cannot open port '" << port << "' on address '" << address << "'" << endl;
      delete httpServer;
      return 0;
    }
    else {
      Logger::info << "http port '" << port << "' on address '" << address << "' open" << endl;
      return httpServer;
    }
  }



  void AddPaloHttpHandlers (Server* server,
                            HttpServer* httpServer,
                            Scheduler* scheduler,
                            const string& templateDirectory,
                            bool admin,
                            Encryption_e encryption,
                            int https,
                            bool enabled) {

    // and add handlers for pictures and style sheets
    httpServer->addHandler("/", new HomeHandler(templateDirectory));
    httpServer->addHandler("/api", new ApiOverviewHandler(templateDirectory));
    httpServer->addHandler("/api/example.html", new HttpFileRequestHandler(templateDirectory + "/example.html", "text/html"));
    httpServer->addHandler("/favicon.ico", new HttpFileRequestHandler(templateDirectory + "/favicon.ico", "image/x-icon"));
    httpServer->addHandler("/inc/background.gif", new HttpFileRequestHandler(templateDirectory + "/background.gif", "image/gif"));
    httpServer->addHandler("/inc/bg.gif", new HttpFileRequestHandler(templateDirectory + "/bg.gif", "image/gif"));
    httpServer->addHandler("/inc/header5.jpg", new HttpFileRequestHandler(templateDirectory + "/header5.jpg", "image/jpeg"));
    httpServer->addHandler("/inc/style_palo.css", new HttpFileRequestHandler(templateDirectory + "/style_palo.css", "text/css"));
    httpServer->addHandler("/inc/topheader.gif", new HttpFileRequestHandler(templateDirectory + "/topHeader.gif", "image/gif"));

    // add error handler
    httpServer->addNotFoundHandler(new NotFoundHandler(templateDirectory));

    // add OLAP Server
    if (enabled) {
      PaloHttpHandler::defineStandardHandlers(server, httpServer, scheduler, templateDirectory, encryption, https);
    }
    else {
      PaloHttpHandler::defineDisabledHandlers(server, httpServer, encryption, https);
    }

    // add documentation
    PaloHttpHandler::defineDocumentationHandlers(server, templateDirectory, templateDirectory + "/documentation", httpServer);

    // browser and statistics
    if (admin) {
      PaloHttpHandler::defineBrowserHandlers(server, httpServer, templateDirectory, scheduler);
      PaloHttpHandler::defineStatisticsHandlers(server, httpServer, templateDirectory);
    }
  }

  ///////////////////
  // static constants
  ///////////////////

  const string PaloHttpHandler::ACTIVATE            = "activate";
  const string PaloHttpHandler::ADD                 = "add";
  const string PaloHttpHandler::BASE_ONLY           = "base_only";
  const string PaloHttpHandler::BLOCKSIZE           = "blocksize";
  const string PaloHttpHandler::COMMENT             = "comment";
  const string PaloHttpHandler::COMPLETE            = "complete";
  const string PaloHttpHandler::CONDITION           = "condition";
  const string PaloHttpHandler::DEFINITION          = "definition";
  const string PaloHttpHandler::EXTERN_PASSWORD     = "extern_password";
  const string PaloHttpHandler::EXTERNAL_IDENTIFIER = "external_identifier";
  const string PaloHttpHandler::EVENT               = "event";
  const string PaloHttpHandler::EVENT_PROCESSOR     = "event_processor";
  const string PaloHttpHandler::FUNCTIONS           = "functions";
  const string PaloHttpHandler::ID_AREA             = "area";
  const string PaloHttpHandler::ID_CHILDREN         = "children";
  const string PaloHttpHandler::ID_CUBE             = "cube";
  const string PaloHttpHandler::ID_DATABASE         = "database";
  const string PaloHttpHandler::ID_DIMENSION        = "dimension";
  const string PaloHttpHandler::ID_DIMENSIONS       = "dimensions";
  const string PaloHttpHandler::ID_ELEMENT          = "element";
  const string PaloHttpHandler::ID_ELEMENTS         = "elements";
  const string PaloHttpHandler::ID_LOCK             = "lock";
  const string PaloHttpHandler::ID_MODE		        = "mode";
  const string PaloHttpHandler::ID_PATH             = "path";
  const string PaloHttpHandler::ID_PATHS            = "paths";
  const string PaloHttpHandler::ID_PATH_TO          = "path_to";
  const string PaloHttpHandler::ID_RULE             = "rule";
  const string PaloHttpHandler::ID_TYPE             = "type";
  const string PaloHttpHandler::NAME_AREA           = "name_area";
  const string PaloHttpHandler::NAME_CHILDREN       = "name_children";
  const string PaloHttpHandler::NAME_CUBE           = "name_cube";
  const string PaloHttpHandler::NAME_DATABASE       = "name_database";
  const string PaloHttpHandler::NAME_DIMENSION      = "name_dimension";
  const string PaloHttpHandler::NAME_DIMENSIONS     = "name_dimensions";
  const string PaloHttpHandler::NAME_ELEMENT        = "name_element";
  const string PaloHttpHandler::NAME_ELEMENTS       = "name_elements";
  const string PaloHttpHandler::NAME_PATH           = "name_path";
  const string PaloHttpHandler::NAME_PATHS          = "name_paths";
  const string PaloHttpHandler::NAME_PATH_TO        = "name_path_to";
  const string PaloHttpHandler::NAME_USER           = "user";
  const string PaloHttpHandler::NEW_NAME            = "new_name";
  const string PaloHttpHandler::NUM_STEPS           = "steps";
  const string PaloHttpHandler::PASSWORD            = "password";
  const string PaloHttpHandler::POSITION            = "position";
  const string PaloHttpHandler::SESSION             = "sid";
  const string PaloHttpHandler::SHOW_ATTRIBUTE      = "show_attribute";
  const string PaloHttpHandler::SHOW_NORMAL         = "show_normal";
  const string PaloHttpHandler::SHOW_RULE           = "show_rule";
  const string PaloHttpHandler::SHOW_RULES          = "show_rules";
  const string PaloHttpHandler::SHOW_SYSTEM         = "show_system";
  const string PaloHttpHandler::SHOW_INFO           = "show_info";
  const string PaloHttpHandler::SKIP_EMPTY          = "skip_empty";
  const string PaloHttpHandler::SOURCE              = "source";
  const string PaloHttpHandler::SPLASH              = "splash";
  const string PaloHttpHandler::USE_IDENTIFIER      = "use_identifier";
  const string PaloHttpHandler::USE_RULES           = "use_rules";
  const string PaloHttpHandler::VALUE               = "value";
  const string PaloHttpHandler::VALUES              = "values";
  const string PaloHttpHandler::WEIGHTS             = "weights";
	const string PaloHttpHandler::SHOW_LOCK_INFO      = "show_lock_info";



  const string PaloHttpHandler::X_PALO_SERVER            = "X-PALO-SV";
  const string PaloHttpHandler::X_PALO_DATABASE          = "X-PALO-DB";
  const string PaloHttpHandler::X_PALO_DIMENSION         = "X-PALO-DIM";
  const string PaloHttpHandler::X_PALO_CUBE              = "X-PALO-CB";
  const string PaloHttpHandler::X_PALO_CUBE_CLIENT_CACHE = "X-PALO-CC";



  bool PaloHttpHandler::requireUser = false;
  time_t PaloHttpHandler::defaultTtl = 3600L;

  ////////////////////
  // standard handlers
  ////////////////////

  void PaloHttpHandler::defineStandardHandlers (Server * server,
                                                HttpServer * httpServer,
                                                Scheduler * scheduler,
                                                const string& directory,
                                                Encryption_e encryption,
                                                int https) {
    EventBeginHandler * handler = new EventBeginHandler(server, scheduler);

    httpServer->addHandler("/event/begin", handler);
    httpServer->addHandler("/event/end", new EventEndHandler(server, handler));

    httpServer->addHandler("/server/databases", new ServerDatabasesHandler(server));
    httpServer->addHandler("/server/info", new ServerInfoHandler(server, encryption, https));
    httpServer->addHandler("/server/load", new ServerLoadHandler(server));
    httpServer->addHandler("/server/login", new ServerLoginHandler(server, scheduler));
    httpServer->addHandler("/server/logout", new ServerLogoutHandler(server, scheduler, handler));
    httpServer->addHandler("/server/save", new ServerSaveHandler(server));
    httpServer->addHandler("/server/shutdown", new ServerShutdownHandler(server, scheduler));

    httpServer->addHandler("/database/create", new DatabaseCreateHandler(server));
    httpServer->addHandler("/database/cubes", new DatabaseCubesHandler(server));
    httpServer->addHandler("/database/destroy", new DatabaseDeleteHandler(server));
    httpServer->addHandler("/database/dimensions", new DatabaseDimensionsHandler(server));
    httpServer->addHandler("/database/info", new DatabaseInfoHandler(server));
    httpServer->addHandler("/database/load", new DatabaseLoadHandler(server));
    httpServer->addHandler("/database/rename", new DatabaseRenameHandler(server));
    httpServer->addHandler("/database/save", new DatabaseSaveHandler(server, scheduler));
    httpServer->addHandler("/database/unload", new DatabaseUnloadHandler(server));

    httpServer->addHandler("/dimension/clear", new DimensionClearHandler(server));
    httpServer->addHandler("/dimension/create", new DimensionCreateHandler(server));
    httpServer->addHandler("/dimension/cubes", new DimensionCubesHandler(server));
    httpServer->addHandler("/dimension/destroy", new DimensionDeleteHandler(server));
    httpServer->addHandler("/dimension/element", new DimensionElementHandler(server));
    httpServer->addHandler("/dimension/elements", new DimensionElementsHandler(server));
    httpServer->addHandler("/dimension/info", new DimensionInfoHandler(server));
    httpServer->addHandler("/dimension/rename", new DimensionRenameHandler(server));

    httpServer->addHandler("/element/append", new ElementAppendHandler(server));
    httpServer->addHandler("/element/create", new ElementCreateHandler(server, false));
	httpServer->addHandler("/element/create_bulk", new ElementCreateBulkHandler(server));
    httpServer->addHandler("/element/destroy", new ElementDeleteHandler(server));
			httpServer->addHandler("/element/destroy_bulk", new ElementDeleteBulkHandler(server));
    httpServer->addHandler("/element/info", new ElementInfoHandler(server));
    httpServer->addHandler("/element/move", new ElementMoveHandler(server));
    httpServer->addHandler("/element/rename", new ElementRenameHandler(server));
    httpServer->addHandler("/element/replace", new ElementCreateHandler(server, true));	

    httpServer->addHandler("/cube/clear_cache", new CubeClearCacheHandler(server));
    httpServer->addHandler("/cube/clear", new CubeClearHandler(server));
    httpServer->addHandler("/cube/commit", new CubeCommitHandler(server));
    httpServer->addHandler("/cube/create", new CubeCreateHandler(server));
    httpServer->addHandler("/cube/destroy", new CubeDeleteHandler(server));
    httpServer->addHandler("/cube/info", new CubeInfoHandler(server));
    httpServer->addHandler("/cube/load", new CubeLoadHandler(server));
    httpServer->addHandler("/cube/lock", new CubeLockHandler(server));
    httpServer->addHandler("/cube/locks", new CubeLocksHandler(server));
    httpServer->addHandler("/cube/rename", new CubeRenameHandler(server));
    httpServer->addHandler("/cube/rollback", new CubeRollbackHandler(server));
    httpServer->addHandler("/cube/rules", new CubeRulesHandler(server));
    httpServer->addHandler("/cube/save", new CubeSaveHandler(server));
    httpServer->addHandler("/cube/unload", new CubeUnloadHandler(server));

    httpServer->addHandler("/cell/area", new CellAreaHandler(server));
    httpServer->addHandler("/cell/area_counter", new CellAreaCounterHandler(server));
    httpServer->addHandler("/cell/copy", new CellCopyHandler(server));
    httpServer->addHandler("/cell/export", new CellExportHandler(server));	
	httpServer->addHandler("/cell/goalseek", new CellGoalSeekHandler(server));
    httpServer->addHandler("/cell/replace", new CellReplaceHandler(server));
    httpServer->addHandler("/cell/replace_bulk", new CellReplaceBulkHandler(server));
    httpServer->addHandler("/cell/value", new CellValueHandler(server));
    httpServer->addHandler("/cell/values", new CellValuesHandler(server));
	httpServer->addHandler("/cell/drillthrough", new CellDrillThroughHandler(server, scheduler));

    httpServer->addHandler("/rule/create", new RuleCreateHandler(server));
    httpServer->addHandler("/rule/destroy", new RuleDeleteHandler(server));
    httpServer->addHandler("/rule/functions", new RuleFunctionsHandler(server, directory));
    httpServer->addHandler("/rule/info", new RuleInfoHandler(server));
    httpServer->addHandler("/rule/modify", new RuleModifyHandler(server));
    httpServer->addHandler("/rule/parse", new RuleParseHandler(server));

	httpServer->addHandler("/svs/info",  new SvsInfoHandler(server));

    httpServer->addHandler("/statistics/clear",  new StatisticsClearHandler(server));
  }



  void PaloHttpHandler::defineDisabledHandlers (Server * server,
                                                HttpServer * httpServer,
                                                Encryption_e encryption,
                                                int https) {

    httpServer->addHandler("/event/begin", new DisabledHandler());
    httpServer->addHandler("/event/end", new DisabledHandler());

    httpServer->addHandler("/server/databases", new DisabledHandler());
    httpServer->addHandler("/server/info", new ServerInfoHandler(server, encryption, https));
    httpServer->addHandler("/server/load", new DisabledHandler());
    httpServer->addHandler("/server/login", new DisabledHandler());
    httpServer->addHandler("/server/logout", new DisabledHandler());
    httpServer->addHandler("/server/save", new DisabledHandler());
    httpServer->addHandler("/server/shutdown", new DisabledHandler());

    httpServer->addHandler("/database/create", new DisabledHandler());
    httpServer->addHandler("/database/cubes", new DisabledHandler());
    httpServer->addHandler("/database/destroy", new DisabledHandler());
    httpServer->addHandler("/database/dimensions", new DisabledHandler());
    httpServer->addHandler("/database/info", new DisabledHandler());
    httpServer->addHandler("/database/load", new DisabledHandler());
    httpServer->addHandler("/database/rename", new DisabledHandler());
    httpServer->addHandler("/database/save", new DisabledHandler());
    httpServer->addHandler("/database/unload", new DisabledHandler());

    httpServer->addHandler("/dimension/clear", new DisabledHandler());
    httpServer->addHandler("/dimension/create", new DisabledHandler());
    httpServer->addHandler("/dimension/cubes", new DisabledHandler());
    httpServer->addHandler("/dimension/destroy", new DisabledHandler());
    httpServer->addHandler("/dimension/element", new DisabledHandler());
    httpServer->addHandler("/dimension/elements", new DisabledHandler());
    httpServer->addHandler("/dimension/info", new DisabledHandler());
    httpServer->addHandler("/dimension/rename", new DisabledHandler());

    httpServer->addHandler("/element/append", new DisabledHandler());
    httpServer->addHandler("/element/create", new DisabledHandler());
	httpServer->addHandler("/element/create_bulk", new DisabledHandler());
    httpServer->addHandler("/element/destroy", new DisabledHandler());
			httpServer->addHandler("/element/destroy_bulk", new DisabledHandler());
    httpServer->addHandler("/element/info", new DisabledHandler());
    httpServer->addHandler("/element/move", new DisabledHandler());
    httpServer->addHandler("/element/rename", new DisabledHandler());
    httpServer->addHandler("/element/replace", new DisabledHandler());
	httpServer->addHandler("/element/replace_bulk", new DisabledHandler());

    httpServer->addHandler("/cube/clear_cache", new DisabledHandler());
    httpServer->addHandler("/cube/clear", new DisabledHandler());
    httpServer->addHandler("/cube/commit", new DisabledHandler());
    httpServer->addHandler("/cube/create", new DisabledHandler());
    httpServer->addHandler("/cube/destroy", new DisabledHandler());
    httpServer->addHandler("/cube/info", new DisabledHandler());
    httpServer->addHandler("/cube/load", new DisabledHandler());
    httpServer->addHandler("/cube/lock", new DisabledHandler());
    httpServer->addHandler("/cube/locks", new DisabledHandler());
    httpServer->addHandler("/cube/rename", new DisabledHandler());
    httpServer->addHandler("/cube/rollback", new DisabledHandler());
    httpServer->addHandler("/cube/rules", new DisabledHandler());
    httpServer->addHandler("/cube/save", new DisabledHandler());
    httpServer->addHandler("/cube/unload", new DisabledHandler());

    httpServer->addHandler("/cell/area", new DisabledHandler());
    httpServer->addHandler("/cell/area_counter", new DisabledHandler());
    httpServer->addHandler("/cell/copy", new DisabledHandler());
    httpServer->addHandler("/cell/export", new DisabledHandler());	
	httpServer->addHandler("/cell/goalseek", new DisabledHandler());
    httpServer->addHandler("/cell/replace", new DisabledHandler());
    httpServer->addHandler("/cell/replace_bulk", new DisabledHandler());
    httpServer->addHandler("/cell/value", new DisabledHandler());
    httpServer->addHandler("/cell/values", new DisabledHandler());
	httpServer->addHandler("/cell/drillthrough", new DisabledHandler());

    httpServer->addHandler("/rule/create", new DisabledHandler());
    httpServer->addHandler("/rule/destroy", new DisabledHandler());
    httpServer->addHandler("/rule/functions", new DisabledHandler());
    httpServer->addHandler("/rule/info", new DisabledHandler());
    httpServer->addHandler("/rule/modify", new DisabledHandler());
    httpServer->addHandler("/rule/parse", new DisabledHandler());

    httpServer->addHandler("/statistics/clear", new DisabledHandler());

	httpServer->addHandler("/svs/info", new DisabledHandler());
  }



  void PaloHttpHandler::defineDocumentationHandlers (Server* server, const string& path, const string& tmpl, HttpServer * httpServer) {
    httpServer->addHandler("/api/server/databases", new DocumentationHandler(tmpl + ".tmpl", path + "/server_databases.api"));
    httpServer->addHandler("/api/server/info", new DocumentationHandler(tmpl + ".tmpl", path + "/server_info.api"));
    httpServer->addHandler("/api/server/load", new DocumentationHandler(tmpl + ".tmpl", path + "/server_load.api"));
    httpServer->addHandler("/api/server/login", new DocumentationHandler(tmpl + "2.tmpl", path + "/server_login.api"));
    httpServer->addHandler("/api/server/logout", new DocumentationHandler(tmpl + ".tmpl", path + "/server_logout.api"));
    httpServer->addHandler("/api/server/save", new DocumentationHandler(tmpl + ".tmpl", path + "/server_save.api"));
    httpServer->addHandler("/api/server/shutdown", new DocumentationHandler(tmpl + ".tmpl", path + "/server_shutdown.api"));

    httpServer->addHandler("/api/database/create", new DocumentationHandler(tmpl + ".tmpl", path + "/database_create.api"));
    httpServer->addHandler("/api/database/cubes", new DocumentationHandler(tmpl + ".tmpl", path + "/database_cubes.api"));
    httpServer->addHandler("/api/database/destroy", new DocumentationHandler(tmpl + ".tmpl", path + "/database_delete.api"));
    httpServer->addHandler("/api/database/dimensions", new DocumentationHandler(tmpl + ".tmpl", path + "/database_dimensions.api"));
    httpServer->addHandler("/api/database/info", new DocumentationHandler(tmpl + ".tmpl", path + "/database_info.api"));
    httpServer->addHandler("/api/database/load", new DocumentationHandler(tmpl + ".tmpl", path + "/database_load.api"));
    httpServer->addHandler("/api/database/rename", new DocumentationHandler(tmpl + ".tmpl", path + "/database_rename.api"));
    httpServer->addHandler("/api/database/save", new DocumentationHandler(tmpl + ".tmpl", path + "/database_save.api"));
    httpServer->addHandler("/api/database/unload", new DocumentationHandler(tmpl + ".tmpl", path + "/database_unload.api"));

    httpServer->addHandler("/api/dimension/clear", new DocumentationHandler(tmpl + ".tmpl", path + "/dimension_clear.api"));
    httpServer->addHandler("/api/dimension/create", new DocumentationHandler(tmpl + ".tmpl", path + "/dimension_create.api"));
    httpServer->addHandler("/api/dimension/cubes", new DocumentationHandler(tmpl + ".tmpl", path + "/dimension_cubes.api"));
    httpServer->addHandler("/api/dimension/destroy", new DocumentationHandler(tmpl + ".tmpl", path + "/dimension_delete.api"));
    httpServer->addHandler("/api/dimension/element", new DocumentationHandler(tmpl + ".tmpl", path + "/dimension_element.api"));
    httpServer->addHandler("/api/dimension/elements", new DocumentationHandler(tmpl + ".tmpl", path + "/dimension_elements.api"));
    httpServer->addHandler("/api/dimension/info", new DocumentationHandler(tmpl + ".tmpl", path + "/dimension_info.api"));
    httpServer->addHandler("/api/dimension/rename", new DocumentationHandler(tmpl + ".tmpl", path + "/dimension_rename.api"));

    httpServer->addHandler("/api/cube/clear", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_clear.api"));
    httpServer->addHandler("/api/cube/commit", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_commit.api"));
    httpServer->addHandler("/api/cube/create", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_create.api"));
    httpServer->addHandler("/api/cube/destroy", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_delete.api"));
    httpServer->addHandler("/api/cube/info", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_info.api"));
    httpServer->addHandler("/api/cube/load", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_load.api"));
    httpServer->addHandler("/api/cube/lock", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_lock.api"));
    httpServer->addHandler("/api/cube/locks", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_locks.api"));
    httpServer->addHandler("/api/cube/rename", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_rename.api"));
    httpServer->addHandler("/api/cube/rollback", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_rollback.api"));
    httpServer->addHandler("/api/cube/rules", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_rules.api"));
    httpServer->addHandler("/api/cube/save", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_save.api"));
    httpServer->addHandler("/api/cube/unload", new DocumentationHandler(tmpl + ".tmpl", path + "/cube_unload.api"));

    httpServer->addHandler("/api/element/append", new DocumentationHandler(tmpl + ".tmpl", path + "/element_append.api"));
    httpServer->addHandler("/api/element/create", new DocumentationHandler(tmpl + ".tmpl", path + "/element_create.api"));
	httpServer->addHandler("/api/element/create_bulk", new DocumentationHandler(tmpl + ".tmpl", path + "/element_create_bulk.api"));
    httpServer->addHandler("/api/element/destroy", new DocumentationHandler(tmpl + ".tmpl", path + "/element_delete.api"));
	httpServer->addHandler("/api/element/destroy_bulk", new DocumentationHandler(tmpl + ".tmpl", path + "/element_delete_bulk.api"));
    httpServer->addHandler("/api/element/info", new DocumentationHandler(tmpl + ".tmpl", path + "/element_info.api"));
    httpServer->addHandler("/api/element/move", new DocumentationHandler(tmpl + ".tmpl", path + "/element_move.api"));
    httpServer->addHandler("/api/element/rename", new DocumentationHandler(tmpl + ".tmpl", path + "/element_rename.api"));
    httpServer->addHandler("/api/element/replace", new DocumentationHandler(tmpl + ".tmpl", path + "/element_replace.api"));
    
    httpServer->addHandler("/api/cell/area", new DocumentationHandler(tmpl + ".tmpl", path + "/cell_area.api"));
    httpServer->addHandler("/api/cell/copy", new DocumentationHandler(tmpl + ".tmpl", path + "/cell_copy.api"));
	httpServer->addHandler("/api/cell/drillthrough", new DocumentationHandler(tmpl + ".tmpl", path + "/cell_drillthrough.api"));
    httpServer->addHandler("/api/cell/export", new DocumentationHandler(tmpl + ".tmpl", path + "/cell_export.api"));
	httpServer->addHandler("/api/cell/goalseek", new DocumentationHandler(tmpl + ".tmpl", path + "/cell_goalseek.api"));
    httpServer->addHandler("/api/cell/replace", new DocumentationHandler(tmpl + ".tmpl", path + "/cell_replace.api"));
    httpServer->addHandler("/api/cell/replace_bulk", new DocumentationHandler(tmpl + ".tmpl", path + "/cell_replace_bulk.api"));
    httpServer->addHandler("/api/cell/value", new DocumentationHandler(tmpl + ".tmpl", path + "/cell_value.api"));
    httpServer->addHandler("/api/cell/values", new DocumentationHandler(tmpl + ".tmpl", path + "/cell_values.api"));

    httpServer->addHandler("/api/event/begin", new DocumentationHandler(tmpl + ".tmpl", path + "/event_begin.api"));
    httpServer->addHandler("/api/event/end", new DocumentationHandler(tmpl + ".tmpl", path + "/event_end.api"));

    httpServer->addHandler("/api/rule/create", new DocumentationHandler(tmpl + ".tmpl", path + "/rule_create.api"));
    httpServer->addHandler("/api/rule/destroy", new DocumentationHandler(tmpl + ".tmpl", path + "/rule_destroy.api"));
    httpServer->addHandler("/api/rule/functions", new DocumentationHandler(tmpl + ".tmpl", path + "/rule_functions.api"));
    httpServer->addHandler("/api/rule/info", new DocumentationHandler(tmpl + ".tmpl", path + "/rule_info.api"));
    httpServer->addHandler("/api/rule/modify", new DocumentationHandler(tmpl + ".tmpl", path + "/rule_modify.api"));
    httpServer->addHandler("/api/rule/parse", new DocumentationHandler(tmpl + ".tmpl", path + "/rule_parse.api"));

	httpServer->addHandler("/api/svs/info", new DocumentationHandler(tmpl + ".tmpl", path + "/svs_info.api"));

    // browse the error codes
    httpServer->addHandler("/api/error.html", new ErrorCodeBrowserHandler(server, path));

    // no browser handler
    httpServer->addHandler("/browser", new DocumentationHandler(path + "/no_browser.tmpl", path + "/home.api"));
  }



  void PaloHttpHandler::defineBrowserHandlers (Server * server, HttpServer * httpServer, const string& path, Scheduler * scheduler) {
    httpServer->addHandler("/browser", new ServerBrowserHandler(server, path, scheduler));
    httpServer->addHandler("/browser/cube", new CubeBrowserHandler(server, path));
    httpServer->addHandler("/browser/database", new DatabaseBrowserHandler(server, path));
    httpServer->addHandler("/browser/dimension", new DimensionBrowserHandler(server, path));
    httpServer->addHandler("/browser/element", new ElementBrowserHandler(server, path));
    httpServer->addHandler("/browser/rule", new RuleBrowserHandler(server, path));
    httpServer->addHandler("/browser/server", new ServerBrowserHandler(server, path, scheduler));
    httpServer->addHandler("/browser/memory", new MemoryStatisticsBrowserHandler(server, path));
    httpServer->addHandler("/browser/statistics", new TimeStatisticsBrowserHandler(server, path));
  }



  void PaloHttpHandler::defineStatisticsHandlers (Server* server, HttpServer * httpServer, const string& path) {
    httpServer->addHandler("/statistics", new StatisticsHandler(server, path));
  }

  /////////////////////
  // handling a request
  /////////////////////

  HttpResponse * PaloHttpHandler::handleHttpRequest (const HttpRequest * request) {
    Statistics::Timer timer("HttpServer::"+request->getRequestPath());

    try {
      checkLogin(request);

      if (session && Server::isBlocking()) {
        if (session->getIdentifier() != Server::getActiveSession()) {
          return new HttpResponse(Server::getSemaphore(), 0);
        }
      }

      return handlePaloRequest(request);
    }
    catch (ErrorException e) {
      HttpResponse * response = new HttpResponse(HttpResponse::BAD);        

      StringBuffer& body = response->getBody();

      appendCsvInteger(&body, (int32_t) e.getErrorType());
      appendCsvString(&body, StringUtils::escapeString(ErrorException::getDescriptionErrorType(e.getErrorType())));
      appendCsvString(&body, StringUtils::escapeString(e.getMessage()));
      body.appendEol();

      Logger::warning << "error code: " << (int32_t) e.getErrorType() 
          << " description: " << ErrorException::getDescriptionErrorType(e.getErrorType()) 
          << " message: " << e.getMessage() << endl;
          
      return response;
    }
  }



  HttpResponse * PaloHttpHandler::handleSemaphoreRaised (semaphore_t semaphore, const HttpRequest * request, IdentifierType clientData, const string* msg) {

    // we are waiting on a supervision lock
    if (semaphore == Server::getSemaphore()) {
      if (Server::isBlocking()) {
        if (session->getIdentifier() == Server::getActiveSession()) {
          return handleHttpRequest(request);
        }
        else {
          return new HttpResponse(Server::getSemaphore(), 0);
        }
      }
      else {
          return handleHttpRequest(request);
      }
    }

    // we are waiting for a worker to complete
    else {
      PaloSemaphoreHandler * handler = dynamic_cast<PaloSemaphoreHandler*>(this);

      try {
        if (handler == 0) {
          throw ErrorException(ErrorException::ERROR_INTERNAL, "no semaphore callback defined");
        }

        checkLogin(request);

        return handler->handlePaloSemaphoreRaised(request, clientData, msg);
      }
      catch (ErrorException e) {
        HttpResponse * response = new HttpResponse(HttpResponse::BAD);        
        
        StringBuffer& body = response->getBody();
        
        appendCsvInteger(&body, (int32_t) e.getErrorType());
        appendCsvString(&body, StringUtils::escapeString(ErrorException::getDescriptionErrorType(e.getErrorType())));
        appendCsvString(&body, StringUtils::escapeString(e.getMessage()));
        body.appendEol();

        return response;
      }
    }
  }



  HttpResponse * PaloHttpHandler::handleSemaphoreDeleted (semaphore_t semaphore, const HttpRequest * request, IdentifierType clientData) {

    // we are waiting on a supervision lock (should not happen!)
    if (semaphore == Server::getSemaphore()) {
      HttpResponse * response = new HttpResponse(HttpResponse::BAD);        
        
      StringBuffer& body = response->getBody();
        
      appendCsvInteger(&body, (int32_t) ErrorException::ERROR_INTERNAL);
      appendCsvString(&body, StringUtils::escapeString("internal error"));
      appendCsvString(&body, StringUtils::escapeString("semaphore failed"));
      body.appendEol();
        
      return response;
    }

    
    // we are wait for a worker to complete
    else {
      PaloSemaphoreHandler * handler = dynamic_cast<PaloSemaphoreHandler*>(this);

      try {
        if (handler == 0) {
          throw ErrorException(ErrorException::ERROR_INTERNAL, "no semaphore callback defined");
        }
        
        return handler->handlePaloSemaphoreDeleted(request, clientData);
      }
      catch (ErrorException e) {
        HttpResponse * response = new HttpResponse(HttpResponse::BAD);        
        
        StringBuffer& body = response->getBody();
        
        appendCsvInteger(&body, (int32_t) e.getErrorType());
        appendCsvString(&body, StringUtils::escapeString(ErrorException::getDescriptionErrorType(e.getErrorType())));
        appendCsvString(&body, StringUtils::escapeString(e.getMessage()));
        body.appendEol();
        
        return response;
      }
    }
  }

  ///////////////////
  // helper functions
  ///////////////////

  void PaloHttpHandler::appendElement (StringBuffer* sb, Dimension* dimension, Element* element) {
    appendCsvInteger(sb, (int32_t) element->getIdentifier());
    appendCsvString( sb, StringUtils::escapeString(element->getName()));
    appendCsvInteger(sb, (int32_t) element->getPosition());
    appendCsvInteger(sb, (int32_t) element->getLevel(dimension));
    appendCsvInteger(sb, (int32_t) element->getIndent(dimension));
    appendCsvInteger(sb, (int32_t) element->getDepth(dimension));
    appendCsvInteger(sb, (int32_t) element->getElementType());
        
    const Dimension::ParentsType* parents = dimension->getParents(element);

    appendCsvInteger(sb, (int32_t) parents->size());

    // append parent identifier
    bool b = false;

    for (Dimension::ParentsType::const_iterator pi = parents->begin();  pi != parents->end();  pi++) {
      if (b) {
        sb->appendChar(',');
      }

      sb->appendInteger((uint32_t) (*pi)->getIdentifier());
      b = true;
    }

    sb->appendChar(';');
        
    // append children identifier
    const ElementsWeightType * children = dimension->getChildren(element);    

    appendCsvInteger(sb, (int32_t) children->size());

    b = false;

    for (ElementsWeightType::const_iterator ci = children->begin();  ci != children->end();  ci++) {
      if (b) {
        sb->appendChar(',');
      }

      sb->appendInteger((uint32_t) (*ci).first->getIdentifier());
      b = true;
    }

    sb->appendChar(';');
                  
    // append children weight
    b = false;

    for (ElementsWeightType::const_iterator cw = children->begin(); cw != children->end(); cw++) {
      if (b) {
        sb->appendChar(',');
      }

      sb->appendDecimal((*cw).second);
      b = true;
    }

    sb->appendChar(';');
    sb->appendEol();

  }



  HttpResponse * PaloHttpHandler::generateElementResponse (Dimension* dimension, Element* element) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    response->setToken(X_PALO_DIMENSION, dimension->getToken());

    appendElement(&sb, dimension, element);

    return response;
  }
    


  HttpResponse * PaloHttpHandler::generateElementsResponse (Dimension* dimension, vector<Element*>* elements) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    response->setToken(X_PALO_DIMENSION, dimension->getToken());

    for (vector<Element*>::iterator i=elements->begin(); i!=elements->end(); i++) {
      Element* element = (*i);

      appendElement(&sb, dimension, element);
    }

    return response;
  }
    


  void PaloHttpHandler::appendRule (StringBuffer* sb, Rule* rule, bool useIdentifier) {
    appendCsvInteger(sb, (int32_t) rule->getIdentifier());

    StringBuffer sb2;
    sb2.initialize();
    rule->appendRepresentation(&sb2, ! useIdentifier);
    appendCsvString(sb, StringUtils::escapeString(sb2.c_str()));
    sb2.free();

    appendCsvString( sb, StringUtils::escapeString(rule->getExternal()));
    appendCsvString( sb, StringUtils::escapeString(rule->getComment()));
    appendCsvInteger(sb, (int32_t) rule->getTimeStamp());
    
    if (rule->isActive()) {
      appendCsvString(sb, "1");
    }
    else {
      appendCsvString(sb, "0");
    }

    sb->appendEol();
  }



  HttpResponse * PaloHttpHandler::generateRuleResponse (Rule* rule, bool useIdentifier) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    appendRule(&sb, rule, useIdentifier);

    response->setToken(X_PALO_CUBE, rule->getCube()->getToken());

    return response;
  }



  HttpResponse * PaloHttpHandler::generateRulesResponse (Cube* cube,
                                                         const vector<Rule*>* rules,
                                                         bool useIdentifier) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    for (vector<Rule*>::const_iterator iter = rules->begin();  iter != rules->end();  iter++) {
      appendRule(&sb, *iter, useIdentifier);
    }

    response->setToken(X_PALO_CUBE, cube->getToken());

    return response;
  }



  void PaloHttpHandler::appendCube (StringBuffer* sb, Cube* cube) {
    appendCsvInteger(sb, (int32_t) cube->getIdentifier());
    appendCsvString( sb, StringUtils::escapeString(cube->getName()));
        
    const vector<Dimension*>* dimensions = cube->getDimensions();

    appendCsvInteger(sb, (int32_t) dimensions->size());

    bool b = false;
    uint64_t sizeCube = 1;

    for (vector<Dimension*>::const_iterator pi = dimensions->begin();  pi != dimensions->end();  pi++) {
      Dimension* dimension = *pi;

      if (b) {
        sb->appendChar(',');
      }

      sb->appendInteger(dimension->getIdentifier());
      b = true;

      sizeCube *= dimension->sizeElements();
    }

    sb->appendChar(';');

    appendCsvInteger(sb, sizeCube);
    appendCsvInteger(sb, cube->sizeFilledCells());

    appendCsvInteger(sb, (int32_t) cube->getStatus());
    
    ItemType it = cube->getType();
   
    if (it == USER_INFO) {
      appendCsvInteger(sb, (int32_t) 3);
    }
    else if (it != SYSTEM) {      
      appendCsvInteger(sb, (int32_t) 0);
    }
    else {
      SystemCube* systemCube =  dynamic_cast<SystemCube*>(cube);

      switch (systemCube->getSubType()) {
        case SystemCube::ATTRIBUTES_CUBE : 
          appendCsvInteger(sb, (int32_t) 2);
          break;

        default:
          appendCsvInteger(sb, (int32_t) 1);
      }
    }
      
    appendCsvInteger(sb, (int32_t) cube->getToken());
    
    sb->appendEol();
  }



  HttpResponse * PaloHttpHandler::generateCubeResponse (Cube* cube) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    appendCube(&sb, cube);

    response->setToken(X_PALO_CUBE, cube->getToken());
    response->setSecondToken(X_PALO_CUBE_CLIENT_CACHE, cube->getClientCacheToken());

    return response;
  }



  HttpResponse * PaloHttpHandler::generateCubesResponse (Database* database, 
                                                         vector<Cube*>* cubes,
                                                         bool showNormal,
                                                         bool showSystem, 
                                                         bool showAttribute,
                                                         bool showInfo) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();
    
    for (vector<Cube*>::iterator i=cubes->begin(); i!=cubes->end(); i++) {
      Cube* cube = (*i);
      
      ItemType it = cube->getType();
      bool normal = false;
      bool system = false;
      bool attribute = false;
      bool userinfo = false;
      
      if (it == NORMAL) {
        normal = showNormal;
      }
      else if (it == USER_INFO) {
        userinfo = showInfo;
      }
      else if (it == SYSTEM) {
        SystemCube* systemCube =  dynamic_cast<SystemCube*>(cube);
        SystemCube::SystemCubeType sdt = systemCube->getSubType();
                
        system    = (sdt != SystemCube::ATTRIBUTES_CUBE) && showSystem;
        attribute = (sdt == SystemCube::ATTRIBUTES_CUBE) && showAttribute;
      }

      if (normal || system || attribute || userinfo) {
        appendCube(&sb, cube);
      }
    }

    response->setToken(X_PALO_DATABASE, database->getToken());

    return response;
  }



  void PaloHttpHandler::appendDimension (StringBuffer* sb, Dimension* dimension) {
    appendCsvInteger(sb, (int32_t) dimension->getIdentifier());
    appendCsvString (sb, StringUtils::escapeString(dimension->getName()));
    appendCsvInteger(sb, (int32_t) dimension->sizeElements());
    appendCsvInteger(sb, (int32_t) dimension->getLevel());
    appendCsvInteger(sb, (int32_t) dimension->getIndent());
    appendCsvInteger(sb, (int32_t) dimension->getDepth());

    ItemType it = dimension->getType();

    // dimension is a normal dimension
    if (it == NORMAL) {
      NormalDimension* nd = dynamic_cast<NormalDimension*>(dimension);

      appendCsvInteger(sb, (int32_t) 0);

      AttributesDimension* ad = nd->getAttributesDimension();
      if (ad) {
        appendCsvInteger(sb, (int32_t) ad->getIdentifier());
      }
      else {
        sb->appendChar(';');
      }

      AttributesCube* ac = nd->getAttributesCube();
      if (ac) {
        appendCsvInteger(sb, (int32_t) ac->getIdentifier());
      }
      else {
        appendCsvString(sb, "");
      }

      RightsCube* rc = nd->getRightsCube();
      if (rc) {
        appendCsvInteger(sb, (int32_t) rc->getIdentifier());
      }
      else {
        appendCsvString(sb, "");
      }
      
    }

    // dimension is a user dimension
    else if (it == USER_INFO) {
      appendCsvInteger(sb, (int32_t) 3);
      UserInfoDimension* cd = dynamic_cast<UserInfoDimension*>(dimension);

      if (cd) {
        AttributesDimension* ad = cd->getAttributesDimension();

        if (ad) {
          appendCsvInteger(sb, (int32_t) ad->getIdentifier());
        }
        else {
          sb->appendChar(';');
        }
  
        AttributesCube* ac = cd->getAttributesCube();

        if (ac) {
          appendCsvInteger(sb, (int32_t) ac->getIdentifier());
        }
        else {
          appendCsvString(sb, "");
        }

        // ";" for rights cube
        sb->appendChar(';');
      }
      else {
        // three ";" for attributes dimension and attributes and rights cube
        sb->appendChar(';');
        sb->appendChar(';');
        sb->appendChar(';');
      }
    }

    // dimension is a system dimension
    else {
      SystemDimension* systemDimension =  dynamic_cast<SystemDimension*>(dimension);

      // cube dimension has attributes dimension and cube
      if (systemDimension->getSubType() == SystemDimension::CUBE_DIMENSION) {
        appendCsvInteger(sb, (int32_t) 1);
        CubeDimension* cd = dynamic_cast<CubeDimension*>(dimension);

        if (cd) {
          AttributesDimension* ad = cd->getAttributesDimension();

          if (ad) {
            appendCsvInteger(sb, (int32_t) ad->getIdentifier());
          }
          else {
            sb->appendChar(';');
          }
    
          AttributesCube* ac = cd->getAttributesCube();

          if (ac) {
            appendCsvInteger(sb, (int32_t) ac->getIdentifier());
          }
          else {
            appendCsvString(sb, "");
          }

          // ";" for rights cube
          sb->appendChar(';');
        }
        else {
          // three ";" for attributes dimension and attributes and rights cube
          sb->appendChar(';');
          sb->appendChar(';');
          sb->appendChar(';');
        }
      }

      // subset and view dimensions have attributes dimension and cube
      else if (systemDimension->getSubType() == SystemDimension::SUBSET_VIEW_DIMENSION) {
        appendCsvInteger(sb, (int32_t) 1);
        SubsetViewDimension* cd = dynamic_cast<SubsetViewDimension*>(dimension);

        if (cd) {
          AttributesDimension* ad = cd->getAttributesDimension();

          if (ad) {
            appendCsvInteger(sb, (int32_t) ad->getIdentifier());
          }
          else {
            sb->appendChar(';');
          }
    
          AttributesCube* ac = cd->getAttributesCube();

          if (ac) {
            appendCsvInteger(sb, (int32_t) ac->getIdentifier());
          }
          else {
            appendCsvString(sb, "");
          }

          // ";" for rights cube
          sb->appendChar(';');
        }
        else {
          // three ";" for attributes dimension and attributes and rights cube
          sb->appendChar(';');
          sb->appendChar(';');
          sb->appendChar(';');
        }
      }

      // other system dimensions
      else {

        // attributes dimension is treated as a normal dimension 
        if (systemDimension->getSubType() == SystemDimension::ATTRIBUTE_DIMENSION) {
          appendCsvInteger(sb, (int32_t) 2);
  
          AttributesDimension* a = dynamic_cast<AttributesDimension*>(dimension);
          NormalDimension* nd = a->getNormalDimension();

          if (nd) {
            appendCsvInteger(sb, (int32_t) nd->getIdentifier());
          }
          else {
            sb->appendChar(';');
          }
        }
        else {
          // other system dimensions
          appendCsvInteger(sb, (int32_t) 1);
          sb->appendChar(';');
        }

        // two ";" for attributes and rights cube
        sb->appendChar(';');
        sb->appendChar(';');
      }
    }
    
    appendCsvInteger(sb, (int32_t) dimension->getToken());

    sb->appendEol();
  }



  HttpResponse * PaloHttpHandler::generateDimensionResponse (Dimension* dimension) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    appendDimension(&sb, dimension);

    response->setToken(X_PALO_DIMENSION, dimension->getToken());

    return response;
  }



  HttpResponse * PaloHttpHandler::generateDimensionsResponse (Database* database, 
                                                              vector<Dimension*>* dimensions,
                                                              bool showNormal, 
                                                              bool showSystem, 
                                                              bool showAttribute,
                                                              bool showInfo) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();
    
    for (vector<Dimension*>::iterator i=dimensions->begin(); i!=dimensions->end(); i++) {
      Dimension* dimension = (*i);
      
      ItemType it = dimension->getType();
      bool normal = false;
      bool system = false;
      bool attribute = false;
      bool userInfo = false;
            
      if (it == NORMAL) {
        normal = showNormal;
      }
      else if (it == USER_INFO) {
        userInfo = showInfo;        
      }
      else if (it == SYSTEM) {
        SystemDimension* systemDimension =  dynamic_cast<SystemDimension*>(dimension);
        SystemDimension::SystemDimensionType sdt = systemDimension->getSubType();
                
        system    = (sdt != SystemDimension::ATTRIBUTE_DIMENSION) && showSystem;
        attribute = (sdt == SystemDimension::ATTRIBUTE_DIMENSION) && showAttribute;
      }

      if (normal || system || attribute || userInfo) {
        appendDimension(&sb, dimension);
      }
    }

    response->setToken(X_PALO_DATABASE, database->getToken());

    return response;
  }



  void PaloHttpHandler::appendDatabase (StringBuffer* sb, Database* database) {
    appendCsvInteger(sb, (int32_t) database->getIdentifier());
    appendCsvString(sb, StringUtils::escapeString(database->getName()));
    appendCsvInteger(sb, (int32_t) database->sizeDimensions());
    appendCsvInteger(sb, (int32_t) database->sizeCubes());
    appendCsvInteger(sb, (int32_t) database->getStatus());
    appendCsvInteger(sb, (int32_t) database->getType());
    appendCsvInteger(sb, (int32_t) database->getToken());
    sb->appendEol();
  }



  HttpResponse * PaloHttpHandler::generateDatabaseResponse (Database* database) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    appendDatabase(&sb, database);

    response->setToken(X_PALO_DATABASE, database->getToken());

    return response;
  }



  HttpResponse * PaloHttpHandler::generateDatabasesResponse (Server * server, vector<Database*>* databases,
    bool showNormal, bool showSystem) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();
    
    for (vector<Database*>::iterator i=databases->begin(); i!=databases->end(); i++) {
      Database* database = (*i);
      
      bool normal = (database->getType() == NORMAL) && showNormal;
      bool system = (database->getType() != NORMAL) && showSystem;

      if (normal || system) {
        appendDatabase(&sb, database);
      }
    }

    response->setToken(X_PALO_SERVER, server->getToken());

    return response;
  }



	/*void PaloHttpHandler::appendCell (StringBuffer* sb, Cube::CellValueType* value, bool found, bool showRule) {
    string s;

    appendCsvInteger(sb, (int32_t) value->type);

    if (found) {
      appendCsvString(sb, "1");

      switch(value->type) {
        case Element::NUMERIC: 
          appendCsvDouble(sb, (*value).doubleValue);
          break;

        case Element::STRING:
          s = (*value).charValue;
          appendCsvString(sb, StringUtils::escapeString(s));
          break;

        default:
          appendCsvString(sb, "");
          break;
      }
    }
    else {
      appendCsvString(sb, "0;");
    }

    if (showRule) {
      if (value->rule != Rule::NO_RULE) {
        appendCsvInteger(sb, (uint32_t) value->rule);
      }
      else {
        sb->appendChar(';');
      }
    }

    sb->appendEol();
	}*/

	void PaloHttpHandler::appendCell (StringBuffer* sb, Cube::CellValueType* value, bool found, bool showRule, bool showLockInfo, Cube::CellLockInfo lockInfo) 
	{
		string s;

		appendCsvInteger(sb, (int32_t) value->type);

		if (found) {
			appendCsvString(sb, "1");

			switch(value->type) {
	case Element::NUMERIC: 
		appendCsvDouble(sb, (*value).doubleValue);
		break;

	case Element::STRING:
		s = (*value).charValue;
		appendCsvString(sb, StringUtils::escapeString(s));
		break;

	default:
		appendCsvString(sb, "");
		break;
  }
		}
		else {
			appendCsvString(sb, "0;");
		}

		if (showRule) {
			if (value->rule != Rule::NO_RULE) {
				appendCsvInteger(sb, (uint32_t) value->rule);
			}
			else {
				sb->appendChar(';');
			}
		}
		if (showLockInfo) 
			appendCsvInteger(sb, lockInfo);

		sb->appendEol();
	}

	void PaloHttpHandler::appendArea (StringBuffer* sb, const vector< set<IdentifierType> > &area) 
	{
		//1:2:3,4:5:6,...
		for (vector< set<IdentifierType> >::const_iterator dit = area.begin(); dit!=area.end(); dit++) {
			if (dit!=area.begin())
				sb->appendChar(',');
			for (set<IdentifierType>::const_iterator it = dit->begin(); it!=dit->end(); it++)
			{
				if (it!=dit->begin())
					sb->appendChar(':');
				sb->appendInteger(*it);
			}
		}
		sb->appendChar(';');
	}
	void PaloHttpHandler::appendArea (StringBuffer* sb, const vector< IdentifiersType > &area) {
		//finish
		for (vector< IdentifiersType >::const_iterator dit = area.begin(); dit!=area.end(); dit++) {
			if (dit!=area.begin())
				sb->appendChar(',');
			for (IdentifiersType::const_iterator it = dit->begin(); it!=dit->end(); it++)
			{
				if (it!=dit->begin())
					sb->appendChar(':');
				sb->appendInteger(*it);
			}
		}
		sb->appendChar(';');
	}


	HttpResponse * PaloHttpHandler::generateCellResponse (Cube* cube, Cube::CellValueType* value, bool found, bool showRule, bool showLockInfo, Cube::CellLockInfo lockInfo) {
		HttpResponse * response = new HttpResponse(HttpResponse::OK);
		StringBuffer& sb = response->getBody();

			appendCell(&sb, value, found, showRule, showLockInfo, lockInfo);

		response->setToken(X_PALO_CUBE, cube->getToken());

		return response;
	}



  HttpResponse * PaloHttpHandler::generateCellsResponse (Cube* cube,
                                                         vector<Cube::CellValueType> * valueCells,
                                                         vector<bool> * foundCells,
                                                         vector<int> * errorCells,
		bool showRule,
		bool showLockInfo,
		vector<Cube::CellLockInfo>* lockInfo) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();
    
    vector<Cube::CellValueType>::iterator  iv = valueCells->begin();
    vector<bool>::iterator                 ib = foundCells->begin();
    vector<int>::iterator                  ie = errorCells->begin();
			vector<Cube::CellLockInfo>::iterator   lii = lockInfo->begin();

			for (; iv != valueCells->end();  iv++, ib++, ie++, lii++) {
      Cube::CellValueType* value = &(*iv);
      bool found                 = *ib;
      int errorCode              = *ie;
				Cube::CellLockInfo lockInfo= *lii;

      if (errorCode == ErrorException::ERROR_UNKNOWN) {
					appendCell(&sb, value, found, showRule, showLockInfo, lockInfo);
      }
      else {
        appendCsvInteger(&sb, (int32_t) 99);
        appendCsvInteger(&sb, (int32_t) errorCode);
        appendCsvString( &sb, "cannot evaluate path");

        if (showRule) {
          sb.appendChar(';');
        }

        sb.appendEol();
      }
    }

    response->setToken(X_PALO_CUBE, cube->getToken());

    return response;
  }



	HttpResponse * PaloHttpHandler::generateLocksResponse (Server* server, Cube* cube, User* user, bool completeContainsArea) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();
    
    const vector<Lock*>& locks = cube->getCubeLocks(user);
    
    for (vector<Lock*>::const_iterator i = locks.begin(); i != locks.end();  i++) {
			appendLock(server, &sb, *i, completeContainsArea);
    }

    response->setToken(X_PALO_CUBE, cube->getToken());

    return response;
  }



	HttpResponse * PaloHttpHandler::generateLockResponse (Server* server, Cube* cube, Lock* lock, bool completeContainsArea) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();
    
    if (lock) {
			appendLock(server, &sb, lock, completeContainsArea);
    }
    
    response->setToken(X_PALO_CUBE, cube->getToken());

    return response;
  }


	void PaloHttpHandler::appendLock (Server* server, StringBuffer* sb, Lock* lock, bool completeContainsArea) {
    appendCsvInteger(sb, (int32_t) lock->getIdentifier());
		if (completeContainsArea)
			appendArea(sb, lock->getContainsArea());
		else
    appendCsvString(sb, lock->getAreaString());    
    

    // we need a system database
    SystemDatabase* sd = server->getSystemDatabase();
    
    User* user = sd->getUser(lock->getUserIdentifier());
    if (user) {
      appendCsvString(sb, user->getName());    
    }
    else {
      appendCsvString(sb, "");    
    }      
    appendCsvInteger(sb, (int32_t) lock->getStorage()->getNumberSteps());
    
    sb->appendEol();
  }
  

  HttpResponse * PaloHttpHandler::generateOkResponse () {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    sb.appendText("1;");
    sb.appendEol();

    return response;
  }



  HttpResponse * PaloHttpHandler::generateOkResponse (Server * server) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    sb.appendText("1;");
    sb.appendEol();

    response->setToken(X_PALO_SERVER, server->getToken());

    return response;
  }



  HttpResponse * PaloHttpHandler::generateOkResponse (Database * database) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    sb.appendText("1;");
    sb.appendEol();

    response->setToken(X_PALO_DATABASE, database->getToken());

    return response;
  }



  HttpResponse * PaloHttpHandler::generateOkResponse (Cube * cube) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    sb.appendText("1;");
    sb.appendEol();

    response->setToken(X_PALO_CUBE, cube->getToken());

    return response;
  }



  HttpResponse * PaloHttpHandler::generateOkResponse (Dimension * dimension) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();

    sb.appendText("1;");
    sb.appendEol();

    response->setToken(X_PALO_DIMENSION, dimension->getToken());

    return response;
  }



  HttpResponse * PaloHttpHandler::generateNotImplementedResponse () {
    HttpResponse * response = new HttpResponse(HttpResponse::NOT_IMPLEMENTED);
    StringBuffer& sb = response->getBody();

    sb.appendText("sorry not implemented");
    sb.appendEol();

    return response;
  }



  void PaloHttpHandler::checkToken (Server* server, const HttpRequest* request) {
    static const string X_PALO_SERVER_COLON = PaloHttpHandler::X_PALO_SERVER + ":";

    const string& token = request->getHeaderField(X_PALO_SERVER_COLON);

    if (token.empty()) {
      return;
    }

    uint32_t value = StringUtils::stringToUnsignedInteger(token);

    if (server->getToken() != value) {
      throw ParameterException(ErrorException::ERROR_SERVER_TOKEN_OUTDATED,
                               "server token outdated",
                               "server", "server");
    }
  }



  void PaloHttpHandler::checkToken (Database* database, const HttpRequest* request) {
    static const string X_PALO_DATABASE_COLON = PaloHttpHandler::X_PALO_DATABASE + ":";

    const string& token = request->getHeaderField(X_PALO_DATABASE_COLON);

    if (token.empty()) {
      return;
    }

    uint32_t value = StringUtils::stringToUnsignedInteger(token);

    if (database->getToken() != value) {
      throw ParameterException(ErrorException::ERROR_DATABASE_TOKEN_OUTDATED,
                               "database token outdated",
                               "database", database->getName());
    }
  }



  void PaloHttpHandler::checkToken (Dimension* dimension, const HttpRequest* request) {
    static const string X_PALO_DIMENSION_COLON = PaloHttpHandler::X_PALO_DIMENSION + ":";

    const string& token = request->getHeaderField(X_PALO_DIMENSION_COLON);

    if (token.empty()) {
      return;
    }

    uint32_t value = StringUtils::stringToUnsignedInteger(token);

    if (dimension->getToken() != value) {
      throw ParameterException(ErrorException::ERROR_DIMENSION_TOKEN_OUTDATED,
                               "dimension token outdated",
                               "dimension", dimension->getName());
    }
  }



  void PaloHttpHandler::checkToken (Cube* cube, const HttpRequest* request) {
    static const string X_PALO_CUBE_COLON = PaloHttpHandler::X_PALO_CUBE + ":";

    const string& token = request->getHeaderField(X_PALO_CUBE_COLON);

    if (token.empty()) {
      return;
    }

    uint32_t value = StringUtils::stringToUnsignedInteger(token);

    if (cube->getToken() != value) {
      throw ParameterException(ErrorException::ERROR_CUBE_TOKEN_OUTDATED,
                               "cube token outdated",
                               "cube", cube->getName());
    }
  }



  Database * PaloHttpHandler::findDatabase (const HttpRequest * request, User* user, bool requireLoad) {
    const string& idString = request->getValue(ID_DATABASE);
    
    if (! idString.empty()) {
      long int id = StringUtils::stringToInteger(idString);
      return server->findDatabase(id, user, requireLoad);
    }
    else {
      const string& name = request->getValue(NAME_DATABASE);
      
      return server->findDatabaseByName(name, user, requireLoad);
    }
  }
  
  
  
  Dimension * PaloHttpHandler::findDimension (Database* database, const HttpRequest * request, User* user) {
    const string& idString = request->getValue(ID_DIMENSION);
    
    if (! idString.empty()) {
      long int id = StringUtils::stringToInteger(idString);
      return database->findDimension(id, user);
    }
    else {
      const string& name = request->getValue(NAME_DIMENSION);
      
      return database->findDimensionByName(name, user);
    }
  }
  
  
  
  Cube * PaloHttpHandler::findCube (Database* database, const HttpRequest * request, User* user, bool requireLoad) {
    const string& idString = request->getValue(ID_CUBE);
    
    if (! idString.empty()) {
      long int id = StringUtils::stringToInteger(idString);
      return database->findCube(id, user, requireLoad);
    }
    else {
      const string& name = request->getValue(NAME_CUBE);
      
      return database->findCubeByName(name, user, requireLoad);
    }
  }
  
  
  
  Element * PaloHttpHandler::findElement (Dimension* dimension, const HttpRequest* request, User* user) {
    const string& idString = request->getValue(ID_ELEMENT);
    
    if (! idString.empty()) {
      long int id = StringUtils::stringToInteger(idString);

      return dimension->findElement(id, user);
    }
    else {
      const string& name = request->getValue(NAME_ELEMENT);
      
      return dimension->findElementByName(name, user);
    }
  }



  Rule* PaloHttpHandler::findRule (Cube* cube, const HttpRequest* request, User* user) {
    const string& idString = request->getValue(ID_RULE);
    IdentifierType id = StringUtils::stringToInteger(idString);

    return cube->findRule(id, user);
  }
  
  
  
  void PaloHttpHandler::appendCsvString (StringBuffer * sb, const string& text) {
    // do not escape here, because some string - i.e. lists of identifier - have no special characters
    sb->appendText(text);
    sb->appendChar(';');
  }


  
  void PaloHttpHandler::appendCsvInteger (StringBuffer *sb, int32_t i) {
    sb->appendInteger(i);
    sb->appendChar(';');
  }
  

  
  void PaloHttpHandler::appendCsvInteger (StringBuffer *sb, uint32_t i) {
    sb->appendInteger(i);
    sb->appendChar(';');
  }
  

  
  void PaloHttpHandler::appendCsvInteger (StringBuffer *sb, uint64_t i) {
    sb->appendInteger(i);
    sb->appendChar(';');
  }
  

  
  void PaloHttpHandler::appendCsvDouble (StringBuffer *sb, double d) {
    sb->appendDecimal(d);
    sb->appendChar(';');
  }
  

    
  

  
  Element::ElementType PaloHttpHandler::getElementTypeByIdentifier (const string& name) {
    if (name == "1") {
      return Element::NUMERIC;
    }
    else if (name == "2") {
      return Element::STRING;
    }
    else if (name == "4") {
      return Element::CONSOLIDATED;
    }
    else {
      return Element::UNDEFINED;
    }       
  }


  
  Cube::SplashMode PaloHttpHandler::getSplashMode (const HttpRequest* request) {
    const string& splash = request->getValue(SPLASH);

    if (splash.empty()) {
      return Cube::DEFAULT;
    }
    else {  
      int32_t splashMode = StringUtils::stringToInteger(splash);
      
      switch (splashMode) {
        case 0: return Cube::DISABLED;
        case 1: return Cube::DEFAULT;
        case 2: return Cube::ADD_BASE;
        case 3: return Cube::SET_BASE;
        default:
          throw ParameterException(ErrorException::ERROR_INVALID_SPLASH_MODE,
                                   "wrong value for splash mode",
                                   SPLASH, splash);
      }
    }
  }



  IdentifiersType PaloHttpHandler::getPath (const string& pathString, const vector<Dimension*> * dimensions, bool usePathIds) {
    if (pathString.empty()) {
      throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                               "path is empty, list of element identifiers is missing",
                               ID_ELEMENT, "");
    }

    size_t numDimensions = dimensions->size();
    IdentifiersType path(numDimensions); 
    string buffer = pathString;
    uint32_t i;
    size_t pos1 = 0;

    for (i = 0; pos1 < buffer.size() && i < numDimensions;  i++) {
      string idString = StringUtils::getNextElement(buffer, pos1, ',');

      path[i] = usePathIds
        ? StringUtils::stringToInteger(idString)
        : (*dimensions)[i]->findElementByName(idString, 0)->getIdentifier();
    }

    if (pos1 < buffer.size() || i < numDimensions) {
      throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                               "wrong number of path elements",
                               "path", pathString);
    }

    return path;
  }



  IdentifiersType PaloHttpHandler::getPath (const HttpRequest* request, const vector<Dimension*> * dimensions) {
    const string& pathIds = request->getValue(ID_PATH);

    if (pathIds.empty()) {
      return getPath(request->getValue(NAME_PATH), dimensions, false);
    }
    else {
      return getPath(pathIds, dimensions, true);
    }
  }



  IdentifiersType PaloHttpHandler::getPathTo (const HttpRequest* request, const vector<Dimension*> * dimensions) {
    const string& pathIds = request->getValue(ID_PATH_TO);

    if (pathIds.empty()) {
      return getPath(request->getValue(NAME_PATH_TO), dimensions, false);
    }
    else {
      return getPath(pathIds, dimensions, true);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief Convert an areaString in CSV format to an area usable for further processing.
  /// @param[in] areaString : The string that was sent to Palo by the client and holds the area information.
  /// @param[in] dimensions : Vector of dimensions that span the area
  /// @param[out] numResult : Total number of cells the area holds.
  /// @param[in] usePathIds : True if the area is described by IDs. Alternatively, names can be used.
  /// @return The return value represents a list of coordinate vectors. They span the actual area.(cartesian product)
  /// @throw ParameterExeption -- the passed area has too many or too few dimensions
  ////////////////////////////////////////////////////////////////////////////////

  vector<IdentifiersType> PaloHttpHandler::getArea (const string& areaString,
                                                    const vector<Dimension*> * dimensions, 
                                                    uint32_t& numResult,
                                                    bool usePathIds,
                                                    bool useBaseOnly) {
    //Logger::debug << "getArea: " << areaString << endl;
                                                      
    string allPath = areaString;
    size_t numDimensions = dimensions->size();

    vector<IdentifiersType> area(numDimensions);

    numResult = 1;

    uint32_t i;
    size_t pos1 = 0;

    for (i = 0;  pos1 < allPath.size() && i < numDimensions;  i++) {
      IdentifiersType path;
      string buffer = StringUtils::getNextElement(allPath, pos1, ',');
      
      if (buffer == "*") {
        vector<Element*> elements = dimensions->at(i)->getElements(0);

        for (vector<Element*>::iterator i = elements.begin(); i != elements.end(); i++) {
          Element * e = *i;

          if (! (useBaseOnly && e->getElementType() == Element::CONSOLIDATED)) {
            path.push_back(e->getIdentifier());
          }
        }
      }
      else {
        for (size_t pos2 = 0;  pos2 < buffer.size(); ) {
          string idString = StringUtils::getNextElement(buffer, pos2, ':');
  
          IdentifierType id = usePathIds
            ? StringUtils::stringToInteger(idString)
            : (*dimensions)[i]->findElementByName(idString, 0)->getIdentifier();

          path.push_back(id);
        }
      }
      
      numResult *= (uint32_t) path.size();

      area[i] = path;
    }

    if (pos1 < allPath.size() || i < numDimensions) {
      throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                               "wrong number of area elements",
                               "area", areaString);
    }

    return area;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief Check if the request contains an ID-Area or a "Name-Area" and call the
  ///        corresponing get_area sub-function.
  /// @param[in] request The HttpRequest from the client
  /// @param[in] dimensions The dimensions spanning the area
  /// @param[out] numResult Number of cells the area has
  /// @return Array of coordinates. The area is the cartesian product.
  ////////////////////////////////////////////////////////////////////////////////

  vector<IdentifiersType> PaloHttpHandler::getArea (const HttpRequest* request, const vector<Dimension*> * dimensions, uint32_t& numResult, bool useBaseOnly) {
    const string& pathIds = request->getValue(ID_AREA);

    if (pathIds.empty()) {
      return getArea(request->getValue(NAME_AREA), dimensions, numResult, false, useBaseOnly);
    }
    else {
      return getArea(pathIds, dimensions, numResult, true, useBaseOnly);
    }
  }



  HttpResponse * PaloHttpHandler::generateLoginResponse (Server *server, PaloSession * session) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);
    StringBuffer& sb = response->getBody();
    
    // append session identifier
    sb.appendText(session->getEncodedIdentifier());
    sb.appendChar(';');

    // append time to live
    sb.appendInteger((uint32_t) session->getTtlIntervall());
    sb.appendChar(';');
    
    response->setToken(X_PALO_SERVER, server->getToken());

    return response;
  }



  PaloSession * PaloHttpHandler::findSession (const string& value) {
    if (value == "") {
      throw ParameterException(ErrorException::ERROR_INVALID_SESSION,
                               "missing session identifier",
                               SESSION, "");
    }
     
    // find session
    return PaloSession::findSession(value);
  }


  PaloSession * PaloHttpHandler::findSession (const HttpRequest * request) {
    return findSession(request->getValue(SESSION));
  }


  void PaloHttpHandler::checkLogin (const HttpRequest * request) {

    // get session
    session = findSession(request);

    // get current user
    user = session->getUser();
    
    if (user && !user->canLogin()) {
      throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                "login error",
                                 NAME_USER, user->getName());
    }
    
  }



  void PaloHttpHandler::setRequireUser (bool require) {
    requireUser = require;
  }
  

  
  bool PaloHttpHandler::getBoolean (const HttpRequest * request, const string& param, bool defaultValue) {
    const string& value = request->getValue(param);
    
    if (value == "") {
      return defaultValue;
    }
    
    if (value == "0") {
      return false;
    }
    
    return true;
  }
}

///////////////////
// module extension
///////////////////

#if defined(ENABLE_HTTP_MODULE)
using namespace palo;

static bool PaloInterface (ServerInfo_t* info) {

  // check version
  if (strcmp(info->version, Server::getVersion()) == 0) {
    Logger::info << "version " << Server::getVersion() << " match in PaloInterface" << endl;
  }
  else {
    Logger::warning << "version " << Server::getVersion() << " mismatch in PaloInterface" << endl;
    return false;
  }

  // check revision
  if (strcmp(info->revision, Server::getRevision()) == 0) {
    Logger::info << "revision " << Server::getRevision() << " match in PaloInterface" << endl;
  }
  else {
    Logger::warning << "revision " << Server::getRevision() << " mismatch in PaloInterface" << endl;
    return false;
  }

  // return info about extension
  string description = "palo http interface";

  info->description = new char[description.size() + 1];
  strcpy(info->description, description.c_str());

  info->httpInterface = &ConstructPaloHttpServer;

  return true;
}

INIT_EXTENSION(PaloInterface);

#endif
