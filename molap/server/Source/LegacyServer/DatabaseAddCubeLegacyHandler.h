////////////////////////////////////////////////////////////////////////////////
/// @brief database_add_cube
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

#ifndef LEGACY_SERVER_DATABASE_ADD_CUBE_LEGACY_HANDLER_H
#define LEGACY_SERVER_DATABASE_ADD_CUBE_LEGACY_HANDLER_H 1

#include "palo.h"

#include "Olap/Database.h"
#include "Exceptions/ParameterException.h"
#include "Olap/Server.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief database_add_cube
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS DatabaseAddCubeLegacyHandler : public LegacyRequestHandler {
  public:
    DatabaseAddCubeLegacyHandler (Server * server) 
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::database_add_cube");

      if (arguments->size() != 4) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting database, cube, and dimension as arguments for database_add_cube",
                                 "number of arguments", (int) arguments->size() - 1);
      }
      
      Database * database = findDatabase(arguments, 1, user);
      const string name = asString(arguments, 2)->getValue();

      vector<Dimension*> dimensions; 

      const vector<LegacyArgument*> * dimensionNames = asMulti(arguments, 3)->getValue();

      for (size_t i = 0;  i < dimensionNames->size();  i++) {
        const string dimensionName = asString(dimensionNames, i)->getValue();
        Dimension * dimension = database->findDimensionByName(dimensionName, user);

        dimensions.push_back(dimension);
      }

      database->addCube(name, &dimensions, user, false);

      return new ArgumentVoid();
    }
  };

}

#endif

