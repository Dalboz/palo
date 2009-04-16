////////////////////////////////////////////////////////////////////////////////
/// @brief database_list_cubes
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

#ifndef LEGACY_SERVER_DATABASE_LIST_CUBES_LEGACY_HANDLER_H
#define LEGACY_SERVER_DATABASE_LIST_CUBES_LEGACY_HANDLER_H 1

#include "palo.h"

#include "InputOutput/Logger.h"

#include "Olap/Server.h"
#include "Exceptions/ParameterException.h"
#include "Olap/Cube.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief database_list_cubes
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS DatabaseListCubesLegacyHandler : public LegacyRequestHandler {
  public:
    DatabaseListCubesLegacyHandler (Server * server)
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::database_list_cubes");

      if (arguments->size() != 2) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting database as argument for database_list_cubes",
                                 "number of arguments", (int) arguments->size() - 1);
      }

      vector<Cube*> cubes = findDatabase(arguments, 1, user)->getCubes(user);
      
      ArgumentMulti * multi = new ArgumentMulti();

      for (vector<Cube*>::const_iterator iter = cubes.begin();  iter != cubes.end();  iter++) {
        const string& name = (*iter)->getName();

        multi->append(new ArgumentString(name));
      }

      return multi;
    }
  };

}

#endif

