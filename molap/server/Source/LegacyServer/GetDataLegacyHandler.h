////////////////////////////////////////////////////////////////////////////////
/// @brief getdata
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

#ifndef LEGACY_SERVER_GET_DATA_LEGACY_HANDLER_H
#define LEGACY_SERVER_GET_DATA_LEGACY_HANDLER_H 1

#include "palo.h"

#include "Collections/StringUtils.h"

#include "Olap/Database.h"
#include "Exceptions/ParameterException.h"
#include "Olap/Server.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief getdata
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS GetDataLegacyHandler : public LegacyRequestHandler {
  public:
    GetDataLegacyHandler (Server * server) 
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::getdata");

      // getdata is called directly, therefore no function exists in arguments
      if (arguments->size() != 3) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting database, cube, path as arguments for getdata",
                                 "number of arguments", (int) arguments->size());
      }
      
      Database * database         = findDatabase(arguments, 0, user);
      Cube * cube                 = findCube(database, arguments, 1, user);
      ArgumentMulti * pathValue   = asMulti(arguments, 2); // path
      
      // construct path
      const vector<LegacyArgument*> * pathArg = pathValue->getValue();
      const vector<Dimension*> * dimensions = cube->getDimensions();

      if (pathArg->size() != dimensions->size()) {
        throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                 "illegal path length",
                                 "length of path", (int) pathArg->size());
      }

      vector<Element*> path;

      for (size_t i = 0;  i < pathArg->size();  i++) {
        Element * element = (*dimensions)[i]->findElementByName(asString(pathArg, i)->getValue(), user);

        path.push_back(element);
      }

      CellPath cp(cube, &path);

      // get value
      bool found;
      set< pair<Rule*, IdentifiersType> > ruleHistory;
      Cube::CellValueType value = cube->getCellValue(&cp, &found, user, 0, &ruleHistory);

      if (found) {
        switch (value.type) {
          case Element::NUMERIC:
            return new ArgumentDouble(value.doubleValue);

          case Element::STRING:
            return new ArgumentString(value.charValue);

          default: 
            throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                     "illegal cell value",
                                     "TYPE", (int) value.type);
        }
      }
      else {
        if (cp.getPathType() == Element::STRING) {
          return new ArgumentString("");
        }
        else {
          return new ArgumentDouble(0);
        }
      }
    }
  };

}

#endif

