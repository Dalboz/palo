////////////////////////////////////////////////////////////////////////////////
/// @brief esibling
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

#ifndef LEGACY_SERVER_E_SIBLING_LEGACY_HANDLER_H
#define LEGACY_SERVER_E_SIBLING_LEGACY_HANDLER_H 1

#include "palo.h"

#include "Exceptions/ParameterException.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {
  using namespace std;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief esibling
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS ESiblingLegacyHandler : public LegacyRequestHandler {
  public:
    ESiblingLegacyHandler (Server * server) 
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::esibling");

      if (arguments->size() != 5) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting database, dimension, element, and count as arguments for esibling",
                                 "number of arguments", (int) arguments->size() - 1);
      }

      Database * database   = findDatabase(arguments, 1, user);
      Dimension * dimension = findDimension(database, arguments, 2, user);
      Element * element     = findElement(dimension, arguments, 3, user);
      int32_t offset        = asInt32(arguments, 4)->getValue();

      // if offset is 0, we return the element itself
      if (offset == 0) {
        return generateElementResult(element);
      }

      // check first parent
      const Dimension::ParentsType* parentsList = dimension->getParents(element);

      if (parentsList->empty()) {
        throw ParameterException(ErrorException::ERROR_ELEMENT_NO_CHILD_OF,
                                 "element has no parent",
                                 "offset", (int) offset);
      }

      Element* parent = (*parentsList)[0];
      const ElementsWeightType * childrenList = dimension->getChildren(parent);

      ssize_t childPos = 0;

      for (ElementsWeightType::const_iterator iter = childrenList->begin();  iter != childrenList->end();  childPos++, iter++) {
        if ((*iter).first == element) {
          break;
        }
      }

      if (childPos >= (ssize_t) childrenList->size()) {
        throw ErrorException(ErrorException::ERROR_INTERNAL,
                             "element not found in dimension");
      }

      // in case of negative offset, try to find element left of the given element
      if (offset < 0) {
        if (childPos + offset < 0) {
          throw ParameterException(ErrorException::ERROR_INVALID_OFFSET,
                                   "illegal offset",
                                   "offset", (int) offset);
        }
        else {
          Element * result = (*childrenList)[childPos + offset].first;
          return generateElementResult(result);
        }
      }

      // in case of positive offset, try to find element right of the given element
      if (childPos + offset < (ssize_t) childrenList->size()) {
        Element * result = (*childrenList)[childPos + offset].first;
        return generateElementResult(result);
      }

      offset -= (ssize_t)(childrenList->size() - childPos);

      // try other parents
      for (Dimension::ParentsType::const_iterator iter = parentsList->begin() + 1;  iter != parentsList->end();  iter++) {
        childrenList = dimension->getChildren(*iter);

        for (ElementsWeightType::const_iterator c = childrenList->begin();  c != childrenList->end();  c++) {
          if ((*c).first != element) {
            if (offset == 0) {
              return generateElementResult((*c).first);
            }

            offset--;
          }
        }
      }

      throw ParameterException(ErrorException::ERROR_INVALID_OFFSET,
                               "illegal offset",
                               "offset", (int) asInt32(arguments, 4)->getValue());
    }
  };

}

#endif

