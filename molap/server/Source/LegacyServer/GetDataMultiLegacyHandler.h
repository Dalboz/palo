////////////////////////////////////////////////////////////////////////////////
/// @brief getdata_multi
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

#ifndef LEGACY_SERVER_GET_DATA_MULTI_LEGACY_HANDLER_H
#define LEGACY_SERVER_GET_DATA_MULTI_LEGACY_HANDLER_H 1

#include "palo.h"

#include <iostream>

#include "Collections/StringUtils.h"

#include "Olap/CellPath.h"
#include "Olap/Database.h"
#include "Exceptions/ParameterException.h"
#include "Olap/Server.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief getdata_multi
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS GetDataMultiLegacyHandler : public LegacyRequestHandler {
  public:
    GetDataMultiLegacyHandler (Server * server) 
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::getdata_multi");

      if (arguments->size() != 4) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting database, cube, paths as arguments for for getdata_multi",
                                 "number of arguments", (int) arguments->size() - 1);
      }
      
      Database * database   = findDatabase(arguments, 1, user);
      Cube * cube           = findCube(database, arguments, 2, user);

      // get rows, columns, and array
      const vector<LegacyArgument*> * paths = asMulti(arguments, 3)->getValue(); // rows, columns, paths
      
      uint32_t rows = asUInt32(paths, 0)->getValue();
      uint32_t cols = asUInt32(paths, 1)->getValue();

      const vector<Dimension*> * dimensions = cube->getDimensions();
        
      if (cols != dimensions->size()) {
        throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                 "illegal path length",
                                 "length of path", (int) cols);
      }

      if (cols * rows != paths->size() - 2) {
        throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                 "illegal number of paths",
                                 "path strings", (int) paths->size() - 2);
      }

      // clear partial result in case of error
      ArgumentMulti * multi = new ArgumentMulti();

      try {

        // loop over rows
        size_t pos = 2;

        for (uint32_t r = 0;  r < rows;  r++) {

          // catch errors and replace them by an error value
          try {

            // construct path
            IdentifiersType path;

            for (size_t i = 0;  i < cols;  i++, pos++) {
              const string& name = asString(paths, pos)->getValue();
              IdentifierType id = (*dimensions)[i]->findElementByName(name, user)->getIdentifier();
            
              path.push_back(id);
            }
          
            CellPath cp(cube, &path);
          
            // get value
            bool found;
            set< pair<Rule*, IdentifiersType> > ruleHistory;
            Cube::CellValueType value = cube->getCellValue(&cp, &found, user, 0, &ruleHistory);

            LegacyArgument * result = 0;
          
            if (found) {
              switch (value.type) {
                case Element::NUMERIC:
                  result = new ArgumentDouble(value.doubleValue);
                  break;

                case Element::STRING:
                  result = new ArgumentString(value.charValue);
                  break;

                default: 
                  throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                           "illegal cell value",
                                           "TYPE", (int) value.type);
                  break;
              }
            }
            else {
              if (cp.getPathType() == Element::STRING) {
                result = new ArgumentString("");
              }
              else {
                result = new ArgumentDouble(0);
              }
            }

            // save result
            multi->append(result);
          }
          catch (const ParameterException&) {
            multi->append(new ArgumentError(LegacyServerTask::PALO_ERR_INV_ARG));
          }
        }

        return multi;
      }
      catch (...) {
        delete multi;
        throw;
      }
    }
  };

}

#endif

