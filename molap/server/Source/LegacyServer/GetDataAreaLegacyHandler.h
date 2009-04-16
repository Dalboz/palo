////////////////////////////////////////////////////////////////////////////////
/// @brief getdata_area
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

#ifndef LEGACY_SERVER_GET_DATA_AREA_LEGACY_HANDLER_H
#define LEGACY_SERVER_GET_DATA_AREA_LEGACY_HANDLER_H 1

#include "palo.h"

#include <iostream>

#include "Collections/StringUtils.h"

#include "Olap/CellPath.h"
#include "Olap/Database.h"
#include "Exceptions/ParameterException.h"
#include "Olap/Server.h"

#include "LegacyServer/LegacyRequestHandler.h"
#include "LegacyServer/LegacyServerTask.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief getdata_area
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS GetDataAreaLegacyHandler : public LegacyRequestHandler {
  public:
    GetDataAreaLegacyHandler (Server * server) 
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::getdata_area");

      if (arguments->size() != 5) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting database, cube, paths, and integer (ignored) as arguments for getdata_area",
                                 "number of arguments", (int) arguments->size() - 1);
      }
      
      this->user = user;

      Database * database   = findDatabase(arguments, 1, user);
      Cube * cube           = findCube(database, arguments, 2, user);
      ArgumentMulti * multi = asMulti(arguments, 3);

      // check size of paths
      const vector<Dimension*> * dimensions = cube->getDimensions();
      const vector<LegacyArgument*> * value = multi->getValue();

      if (value->size() != dimensions->size()) {
        throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                 "wrong number of dimensions",
                                 "number of dimensions", (int)(dimensions->size()));
      }

      // extract paths
      vector< vector<string> > paths;
      size_t numResult = 1;

      for (size_t i = 0;  i < value->size();  i++) {
        const vector<LegacyArgument*> * single = asMulti(value, i)->getValue();
        vector<string> stringPath;
        
        for (size_t j = 0;  j < single->size();  j++) {
          stringPath.push_back(asString(single, j)->getValue());
        }

        paths.push_back(stringPath);
        numResult *= single->size();
      }

      // results gets delete in case of error
      ArgumentMulti * result = new ArgumentMulti();

      // loop over all combinations
      if (0 < numResult) {
        vector<size_t> combinations(paths.size(), 0);

        try {
          loop(cube, dimensions, paths, combinations, result);
        }
        catch (...) {
          delete result;
          throw;
        }
      }

      return result;
    }

  private:
    void loop (Cube * cube,
               const vector< Dimension* > * dimensions,
               const vector< vector<string> >& paths,
               vector<size_t>& combinations, 
               ArgumentMulti * result) {

      bool eval  = true;
      int length = (int) paths.size();

      for (int i = length - 1;  0 <= i;) {
        if (eval) {
          evaluate(cube, dimensions, paths, combinations, result);
        }

        size_t position = combinations[i];
        const vector<string>& dim = paths[i];

        if (position + 1 < dim.size()) {
          combinations[i] = (int) position + 1;
          i = length - 1;
          eval = true;
        }
        else {
          i--;

          for (int k = i + 1;  k < length;  k++) {
            combinations[k] = 0;
          }

          eval = false;
        }
      }
    }



    void evaluate (Cube * cube,
                   const vector< Dimension* > * dimensions,
                   const vector< vector<string> >& paths,
                   vector<size_t>& combinations, 
                   ArgumentMulti * area) {

      try {

        // construct path
        IdentifiersType path;

        for (size_t i = 0;  i < paths.size();  i++) {
          const string& name = paths[i][combinations[i]];
          path.push_back((*dimensions)[i]->findElementByName(name, user)->getIdentifier());
        }

        CellPath cp(cube, &path);
          
        // get value
        bool found;
        set< pair<Rule*, IdentifiersType> > ruleHistory;
        Cube::CellValueType value = cube->getCellValue(&cp, &found, 0, 0, &ruleHistory);
        
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
        area->append(result);
      }
      catch (const ParameterException&) {
        area->append(new ArgumentError(LegacyServerTask::PALO_ERR_INV_ARG));
      }

    }

  private:
    User* user;
  };

}

#endif

