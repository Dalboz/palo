////////////////////////////////////////////////////////////////////////////////
/// @brief setdata
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

#ifndef LEGACY_SERVER_SET_DATA_LEGACY_HANDLER_H
#define LEGACY_SERVER_SET_DATA_LEGACY_HANDLER_H 1

#include "palo.h"

#include "Olap/Database.h"
#include "Exceptions/ParameterException.h"
#include "Olap/Lock.h"
#include "Olap/Server.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief setdata
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS SetDataLegacyHandler : public LegacyRequestHandler {
  public:
    SetDataLegacyHandler (Server * server) 
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession* session) {
      Statistics::Timer timer("LegacyHandler::setdata");

      // setdata is called directly, therefore no function exists in arguments
      if (arguments->size() != 4) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting database, cube, path-value, splash as arguments for setdata",
                                 "number of arguments", (int) arguments->size());
      }
      
      Database * database         = findDatabase(arguments, 0, user);
      Cube * cube                 = findCube(database, arguments, 1, user);
      ArgumentMulti * pathValue   = asMulti(arguments, 2); // path, value
      Cube::SplashMode splashMode = legacySplashMode(asInt32(arguments, 3));
      
      // construct path
      const vector<LegacyArgument*> * pv = pathValue->getValue();

      if (pv->size() != 2) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting path and value",
                                 "length of path and value", (int) pv->size());
      }

      const vector<LegacyArgument*> * pathArg = asMulti(pv, 0)->getValue();
      const vector<Dimension*> * dimensions = cube->getDimensions();

      if (pathArg->size() != dimensions->size()) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "illegal path length",
                                 "length of path", (int) pathArg->size());
      }

      IdentifiersType path;

      for (size_t i = 0;  i < pathArg->size();  i++) {
        IdentifierType id = (*dimensions)[i]->findElementByName(asString(pathArg, i)->getValue(), user)->getIdentifier();

        path.push_back(id);
      }

      CellPath cp(cube, &path);
      Element::ElementType cpType = cp.getPathType();

      // get value
      LegacyArgument * v = (*pv)[1];
      uint32_t type = v->getArgumentType();
      semaphore_t semaphore = 0;

      if (type == LegacyArgument::NET_ARG_TYPE_DOUBLE) {
        double value = reinterpret_cast<ArgumentDouble*>(v)->getValue();

        if (cpType == Element::NUMERIC || cpType == Element::CONSOLIDATED) {
          semaphore = cube->setCellValue(&cp, value, user, session, true, true, false, splashMode, Lock::checkLock);
        }
        else {
          throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                   "path is string, value is numeric",
                                   "VALUE", value);
        }
      }
      else if (type == LegacyArgument::NET_ARG_TYPE_STRING) {
        const string value = reinterpret_cast<ArgumentString*>(v)->getValue();

        if (cpType == Element::STRING) {
          semaphore = cube->setCellValue(&cp, value, user, session, true, true, Lock::checkLock);
        }
        else {
          throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                   "path is numeric, value is string",
                                   "VALUE", value);
        }
      }

      return semaphore == 0 ? ((LegacyArgument*) new ArgumentVoid()) : ((LegacyArgument*) new ArgumentDelayed(semaphore));
    }
  };

}

#endif

