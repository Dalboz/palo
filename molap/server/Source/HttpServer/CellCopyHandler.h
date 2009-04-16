////////////////////////////////////////////////////////////////////////////////
/// @brief cell copy
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

#ifndef HTTP_SERVER_CELL_COPY_HANDLER_H
#define HTTP_SERVER_CELL_COPY_HANDLER_H 1

#include "palo.h"

#include <string>
#include <iostream>

#include "Olap/CellPath.h"
#include "Olap/Dimension.h"
#include "Exceptions/ParameterException.h"

#include "HttpServer/PaloHttpHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief cell replace
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CellCopyHandler : public PaloHttpHandler {
    
  public:
    CellCopyHandler (Server * server) 
      : PaloHttpHandler(server) {
    }

  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      Database* database = findDatabase(request, 0);
      Cube* cube = findCube(database, request, 0);

      checkToken(cube, request);
      
      // get path
      IdentifiersType pathFrom = getPath(request, cube->getDimensions());
      IdentifiersType pathTo   = getPathTo(request, cube->getDimensions());

      // get value
      const string& valueString = request->getValue(VALUE);

      // construct cell path and set value
      CellPath cpFrom(cube, &pathFrom);
      CellPath cpTo(cube, &pathTo);

      if (valueString == "") {
        Logger::trace << "copy cell from = <" << request->getValue(ID_PATH) 
                      << "> to = <" << request->getValue(ID_PATH_TO) << ">" << endl;
        cube->copyCellValues(&cpFrom, &cpTo, user, session);
      }
      else {
        double value = StringUtils::stringToDouble(valueString);
        Logger::trace << "copy/like cell from = <" << request->getValue(ID_PATH) 
                      << "> to = <" << request->getValue(ID_PATH_TO) << ">" 
                      << " value = " << value << endl;
        cube->copyLikeCellValues(&cpFrom, &cpTo, user, session, value);
      }
      
      return generateOkResponse(cube);
    }
  };

}

#endif
