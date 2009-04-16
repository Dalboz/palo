////////////////////////////////////////////////////////////////////////////////
/// @brief server_shutdown
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

#ifndef LEGACY_SERVER_SERVER_SHUTDOWN_LEGACY_HANDLER_H
#define LEGACY_SERVER_SERVER_SHUTDOWN_LEGACY_HANDLER_H 1

#include "palo.h"

#include "InputOutput/Logger.h"

#include "Olap/Server.h"
#include "Exceptions/ParameterException.h"
#include "Olap/Server.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief server_shutdown
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS ServerShutdownLegacyHandler : public LegacyRequestHandler {
  public:
    ServerShutdownLegacyHandler (Server * server, Scheduler * scheduler)
      : LegacyRequestHandler(server), scheduler(scheduler) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::server_shutdown");

      if (arguments->size() != 1) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting no arguments for server_shutdown",
                                 "number of arguments", (int) arguments->size() - 1);
      }

      server->beginShutdown(user);

      return new ArgumentVoid();
    }

  private:
    Scheduler* scheduler;
    
  };

}

#endif

