////////////////////////////////////////////////////////////////////////////////
/// @brief eweight
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

#ifndef LEGACY_SERVER_E_WEIGHT_LEGACY_HANDLER_H
#define LEGACY_SERVER_E_WEIGHT_LEGACY_HANDLER_H 1

#include "palo.h"

#include "Exceptions/ParameterException.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief eweight
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS EWeightLegacyHandler : public LegacyRequestHandler {
  public:
    EWeightLegacyHandler (Server * server) 
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::eweight");

      if (arguments->size() != 5) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting database, dimension, parent and child as arguments for eweight",
                                 "number of arguments", (int) arguments->size() - 1);
      }

      Database * database   = findDatabase(arguments, 1, user);
      Dimension * dimension = findDimension(database, arguments, 2, user);
      Element * parent      = findElement(dimension, arguments, 3, user);
      Element * child       = findElement(dimension, arguments, 4, user);

      const ElementsWeightType * children = dimension->getChildren(parent);

      for (ElementsWeightType::const_iterator i = children->begin();  i != children->end();  i++) {
        Element * c = (*i).first;

        if (c == child) {
          return new ArgumentDouble((*i).second);
        }
      }

      throw ParameterException(ErrorException::ERROR_ELEMENT_NO_CHILD_OF,
                               "child is not in parent's children list",
                               "parent", parent->getName());
    }
  };

}

#endif

