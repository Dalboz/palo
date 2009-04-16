////////////////////////////////////////////////////////////////////////////////
/// @brief legacy server
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

#include "LegacyServer/LegacyServer.h"

#include <iostream>

#include "InputOutput/Logger.h"
#include "InputOutput/Scheduler.h"

#include "LegacyServer/CubeCommitLogLegacyHandler.h"
#include "LegacyServer/CubeDeleteLegacyHandler.h"
#include "LegacyServer/CubeListDimensionsLegacyHandler.h"
#include "LegacyServer/CubeLoadLegacyHandler.h"
#include "LegacyServer/CubeUnloadLegacyHandler.h"
#include "LegacyServer/DatabaseAddCubeLegacyHandler.h"
#include "LegacyServer/DatabaseAddDimensionLegacyHandler.h"
#include "LegacyServer/DatabaseDeleteLegacyHandler.h"
#include "LegacyServer/DatabaseListCubesLegacyHandler.h"
#include "LegacyServer/DatabaseListDimensionsLegacyHandler.h"
#include "LegacyServer/DatabaseLoadLegacyHandler.h"
#include "LegacyServer/DatabaseRenameDimensionLegacyHandler.h"
#include "LegacyServer/DatabaseSaveLegacyHandler.h"
#include "LegacyServer/DatabaseUnloadLegacyHandler.h"
#include "LegacyServer/DimElementListConsolidationElementsLegacyHandler.h"
#include "LegacyServer/DimensionClearLegacyHandler.h"
#include "LegacyServer/DimensionCommitLogLegacyHandler.h"
#include "LegacyServer/DimensionDeleteLegacyHandler.h"
#include "LegacyServer/CubeListDimensionsLegacyHandler.h"
#include "LegacyServer/DimensionListDimElementsLegacyHandler.h"
#include "LegacyServer/DimensionProcessLegacyHandler.h"
#include "LegacyServer/DimensionUnloadLegacyHandler.h"
#include "LegacyServer/EAddOrUpdateLegacyHandler.h"
#include "LegacyServer/EChildCountLegacyHandler.h"
#include "LegacyServer/EChildNameLegacyHandler.h"
#include "LegacyServer/ECountLegacyHandler.h"
#include "LegacyServer/EDeleteLegacyHandler.h"
#include "LegacyServer/EDimensionLegacyHandler.h"
#include "LegacyServer/EFirstLegacyHandler.h"
#include "LegacyServer/EIndentLegacyHandler.h"
#include "LegacyServer/EIndexLegacyHandler.h"
#include "LegacyServer/EIsChildLegacyHandler.h"
#include "LegacyServer/ELevelLegacyHandler.h"
#include "LegacyServer/EMoveLegacyHandler.h"
#include "LegacyServer/ENameLegacyHandler.h"
#include "LegacyServer/ENextLegacyHandler.h"
#include "LegacyServer/EParentCountLegacyHandler.h"
#include "LegacyServer/EParentNameLegacyHandler.h"
#include "LegacyServer/EPrevLegacyHandler.h"
#include "LegacyServer/ERenameLegacyHandler.h"
#include "LegacyServer/ESiblingLegacyHandler.h"
#include "LegacyServer/ETopLevelLegacyHandler.h"
#include "LegacyServer/ETypeLegacyHandler.h"
#include "LegacyServer/EWeightLegacyHandler.h"
#include "LegacyServer/GetDataAreaLegacyHandler.h"
#include "LegacyServer/GetDataExportLegacyHandler.h"
#include "LegacyServer/GetDataLegacyHandler.h"
#include "LegacyServer/GetDataMultiLegacyHandler.h"
#include "LegacyServer/LegacyServerTask.h"
#include "LegacyServer/PingLegacyHandler.h"
#include "LegacyServer/RootAddDatabaseLegacyHandler.h"
#include "LegacyServer/RootListDatabasesLegacyHandler.h"
#include "LegacyServer/RootSaveLegacyHandler.h"
#include "LegacyServer/ServerShutdownLegacyHandler.h"
#include "LegacyServer/SetDataLegacyHandler.h"

#if defined(ENABLE_LEGACY_MODULE)
#include "Programs/extension.h"
#endif

namespace palo {

  /////////////////
  // local handlers
  /////////////////

  class NotFoundLegacyHandler : public LegacyRequestHandler {
  public:
    NotFoundLegacyHandler (Server* server)
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User*, PaloSession*) {
      if (arguments->empty()) {
        throw ParameterException(ErrorException::ERROR_INV_CMD,
                                 "unknown function called",
                                 "function", "unknown");
      }
      else {
        throw ParameterException(ErrorException::ERROR_INV_CMD,
                                 "unknown function called",
                                 "function", asString(arguments, 0)->getValue());
      }
    }
  };


  /////////////////////
  // static constructor
  /////////////////////

  LegacyServer * ConstructPaloLegacyServer (Scheduler * scheduler,
                                            Server * server,
                                            const string& address,
                                            int port) {

    LegacyServer * legacy = new LegacyServer(server, scheduler);

    legacy->addHandler("cube_commit_log",                         new CubeCommitLogLegacyHandler(server));
    legacy->addHandler("cube_delete",                             new CubeDeleteLegacyHandler(server));
    legacy->addHandler("cube_list_dimensions",                    new CubeListDimensionsLegacyHandler(server));
    legacy->addHandler("cube_load",                               new CubeLoadLegacyHandler(server));
    legacy->addHandler("cube_unload",                             new CubeUnloadLegacyHandler(server));
    legacy->addHandler("database_add_cube",                       new DatabaseAddCubeLegacyHandler(server));
    legacy->addHandler("database_add_dimension",                  new DatabaseAddDimensionLegacyHandler(server));
    legacy->addHandler("database_delete",                         new DatabaseDeleteLegacyHandler(server));
    legacy->addHandler("database_list_cubes",                     new DatabaseListCubesLegacyHandler(server));
    legacy->addHandler("database_list_dimensions",                new DatabaseListDimensionsLegacyHandler(server));
    legacy->addHandler("database_load",                           new DatabaseLoadLegacyHandler(server));
    legacy->addHandler("database_rename_dimension",               new DatabaseRenameDimensionLegacyHandler(server));
    legacy->addHandler("database_save",                           new DatabaseSaveLegacyHandler(server));
    legacy->addHandler("database_unload",                         new DatabaseUnloadLegacyHandler(server));
    legacy->addHandler("dim_element_list_consolidation_elements", new DimElementListConsolidationElementsLegacyHandler(server));
    legacy->addHandler("dimension_clear",                         new DimensionClearLegacyHandler(server));
    legacy->addHandler("dimension_commit_log",                    new DimensionCommitLogLegacyHandler(server));
    legacy->addHandler("dimension_delete",                        new DimensionDeleteLegacyHandler(server));
    legacy->addHandler("dimension_list_cubes",                    new CubeListDimensionsLegacyHandler(server));
    legacy->addHandler("dimension_list_dim_elements",             new DimensionListDimElementsLegacyHandler(server));
    legacy->addHandler("dimension_process",                       new DimensionProcessLegacyHandler(server));
    legacy->addHandler("dimension_unload",                        new DimensionUnloadLegacyHandler(server));
    legacy->addHandler("eadd_or_update",                          new EAddOrUpdateLegacyHandler(server));
    legacy->addHandler("echildcount",                             new EChildCountLegacyHandler(server));
    legacy->addHandler("echildname",                              new EChildNameLegacyHandler(server));
    legacy->addHandler("ecount",                                  new ECountLegacyHandler(server));
    legacy->addHandler("edelete",                                 new EDeleteLegacyHandler(server));
    legacy->addHandler("edimension",                              new EDimensionLegacyHandler(server));
    legacy->addHandler("efirst",                                  new EFirstLegacyHandler(server));
    legacy->addHandler("eindent",                                 new EIndentLegacyHandler(server));
    legacy->addHandler("eindex",                                  new EIndexLegacyHandler(server));
    legacy->addHandler("eischild",                                new EIsChildLegacyHandler(server));
    legacy->addHandler("elevel",                                  new ELevelLegacyHandler(server));
    legacy->addHandler("emove",                                   new EMoveLegacyHandler(server));
    legacy->addHandler("ename",                                   new ENameLegacyHandler(server));
    legacy->addHandler("enext",                                   new ENextLegacyHandler(server));
    legacy->addHandler("eparentcount",                            new EParentCountLegacyHandler(server));
    legacy->addHandler("eparentname",                             new EParentNameLegacyHandler(server));
    legacy->addHandler("eprev",                                   new EPrevLegacyHandler(server));
    legacy->addHandler("erename",                                 new ERenameLegacyHandler(server));
    legacy->addHandler("esibling",                                new ESiblingLegacyHandler(server));
    legacy->addHandler("etoplevel",                               new ETopLevelLegacyHandler(server));
    legacy->addHandler("etype",                                   new ETypeLegacyHandler(server));
    legacy->addHandler("eweight",                                 new EWeightLegacyHandler(server));
    legacy->addHandler("getdata",                                 new GetDataLegacyHandler(server));
    legacy->addHandler("getdata_area",                            new GetDataAreaLegacyHandler(server));
    legacy->addHandler("getdata_export",                          new GetDataExportLegacyHandler(server));
    legacy->addHandler("getdata_multi",                           new GetDataMultiLegacyHandler(server));
    legacy->addHandler("ping",                                    new PingLegacyHandler(server));
    legacy->addHandler("root_add_database",                       new RootAddDatabaseLegacyHandler(server));
    legacy->addHandler("root_list_databases",                     new RootListDatabasesLegacyHandler(server));
    legacy->addHandler("root_save",                               new RootSaveLegacyHandler(server));
    legacy->addHandler("server_shutdown",                         new ServerShutdownLegacyHandler(server, scheduler));
    legacy->addHandler("setdata",                                 new SetDataLegacyHandler(server));

    // connect server with the listing port
    bool ok;

    if (address.empty()) {
      ok = scheduler->addListenTask(legacy, port);
    }
    else {
      ok = scheduler->addListenTask(legacy, address, port);
    }

    if (! ok) {
      Logger::error << "cannot open port '" << port << "' on address '" << address << "'" << endl;
      return 0;
    }
    else {
      Logger::info << "legacy port '" << port << "' on address '" << address << "' open" << endl;
      return legacy;
    }
  }

  ///////////////
  // constructors
  ///////////////

  LegacyServer::LegacyServer (Server * server, Scheduler* scheduler)
    : IoTask(INVALID_SOCKET, INVALID_SOCKET),
      server(server),
      scheduler(scheduler), 
      handlers(1000, Name2HandlerDesc()),
      notFoundHandler(new NotFoundLegacyHandler(server)) {
    getdataHandler = notFoundHandler;
    setdataHandler = notFoundHandler;
    semaphore = scheduler->createSemaphore();
  }



  LegacyServer::~LegacyServer () {
    for (set<LegacyServerTask*>::iterator i = tasks.begin();  i != tasks.end();  i++) {
      LegacyServerTask * task = *i;

      scheduler->removeIoTask(task);

      closesocket(task->getReadSocket());
    }

    scheduler->deleteSemaphore(semaphore);
  }



  void LegacyServer::handleConnected (socket_t fd) {
    LegacyServerTask * task = new LegacyServerTask(fd, this);

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



  void LegacyServer::handleClientHangup (LegacyServerTask * task) {
    Logger::info << "lost client connection on " << task->getReadSocket() << endl;

    LoginWorker* loginWorker = server->getLoginWorker();

    if (loginWorker) {
      User* user = task->getUser();

      if (user) {
        loginWorker->executeLogout(semaphore, user->getName());
      }
      else {
        loginWorker->executeLogout(semaphore, "<unknown>");
      }
    }            

    scheduler->removeIoTask(task);
    
    closesocket(task->getReadSocket());
    tasks.erase(task);
  }



  void LegacyServer::handleShutdown () {
    Logger::trace << "beginning shutdown of listener on " << readSocket << endl;

    closesocket(readSocket);

    scheduler->removeListenTask(this);
  }



  void LegacyServer::addHandler (const string& name, LegacyRequestHandler* handler) {
    Logger::trace << "defining legacy handler '" << name << "'" << endl;

    NameHandlerPair * nhp = handlers.findKey(name);

    if (nhp != 0) {
      delete nhp;
    }

    nhp = new NameHandlerPair(name, handler);

    handlers.addElement(nhp);

    if (name == "getdata") {
      getdataHandler = handler;
    }
    else if (name == "setdata") {
      setdataHandler = handler;
    }
  }
}

///////////////////
// module extension
///////////////////

#if defined(ENABLE_LEGACY_MODULE)
using namespace palo;
extern char * Version;
extern char * Revision;

static bool PaloInterface (ServerInfo_t* info) {

  // check version
  if (strcmp(info->version, ::Version) == 0) {
    Logger::info << "version " << ::Version << " match in PaloInterface" << endl;
  }
  else {
    Logger::warning << "version " << ::Version << " mismatch in PaloInterface" << endl;
    return false;
  }

  // check revision
  if (strcmp(info->revision, ::Revision) == 0) {
    Logger::info << "revision " << ::Revision << " match in PaloInterface" << endl;
  }
  else {
    Logger::warning << "revision " << ::Revision << " mismatch in PaloInterface" << endl;
    return false;
  }

  // return info about extension
  string description = "palo legacy interface";

  info->description = new char[description.size() + 1];
  strcpy(info->description, description.c_str());

  info->legacyInterface = &ConstructPaloLegacyServer;

  return true;
}

INIT_EXTENSION(PaloInterface);

#endif
