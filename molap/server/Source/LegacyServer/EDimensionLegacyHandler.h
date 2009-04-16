////////////////////////////////////////////////////////////////////////////////
/// @brief edimension
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

#ifndef LEGACY_SERVER_E_DIMENSION_LEGACY_HANDLER_H
#define LEGACY_SERVER_E_DIMENSION_LEGACY_HANDLER_H 1

#include "palo.h"

#include "Exceptions/ParameterException.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief edimension
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS EDimensionLegacyHandler : public LegacyRequestHandler {
  public:
    EDimensionLegacyHandler (Server * server) 
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::edimension");

      if (arguments->size() != 4) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting database, dimension elements, and unique flag as arguments for edimension",
                                 "number of arguments", (int)(arguments->size() - 1));
      }

      Database * database                      = findDatabase(arguments, 1, user);
      const vector<LegacyArgument*> * elements = asMulti(arguments, 2)->getValue();
      bool unique                              = asInt32(arguments, 3)->getValue() != 0;

      // get dimensions
      vector<Dimension*> dimensions = database->getDimensions(user);
      vector<Dimension*> result;

      for (vector<Dimension*>::const_iterator iter = dimensions.begin();  iter != dimensions.end();  iter++) {
        Dimension * dimension = *iter;

        if (contains(dimension, elements)) {
          if (unique && ! result.empty()) {
            throw ParameterException(ErrorException::ERROR_DIMENSION_EXISTS,
                                     "dimension is not unique",
                                     "dimension", dimension->getName());
          }

          result.push_back(dimension);
        }
      }

      if (result.empty()) {
        throw ParameterException(ErrorException::ERROR_DIMENSION_NOT_FOUND,
                                 "no dimension found",
                                 "number of elements", (int) elements->size());
      }

      return new ArgumentString(result[0]->getName());
    }

  private:
    bool contains (Dimension * dimension, const vector<LegacyArgument*> * elements) {
      for (size_t i = 0;  i < elements->size();  i++) {
        if (dimension->lookupElementByName(asString(elements, i)->getValue()) == 0) {
          return false;
        }
      }

      return true;
    }
  };

}

#endif

