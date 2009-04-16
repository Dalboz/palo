////////////////////////////////////////////////////////////////////////////////
/// @brief eadd_or_update
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

#ifndef LEGACY_SERVER_E_ADD_OR_UPDATE_LEGACY_HANDLER_H
#define LEGACY_SERVER_E_ADD_OR_UPDATE_LEGACY_HANDLER_H 1

#include "palo.h"

#include "Olap/Database.h"
#include "Exceptions/ParameterException.h"
#include "Olap/Server.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief eadd_or_update
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS EAddOrUpdateLegacyHandler : public LegacyRequestHandler {
  public:
    EAddOrUpdateLegacyHandler (Server * server) 
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::eadd_or_update");

      if (arguments->size() != 8) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting 7 arguments for eadd_or_update",
                                 "number of arguments", (int) arguments->size() - 1);
      }

      Database * database   = findDatabase(arguments, 1, user);
      Dimension * dimension = findDimension(database, arguments, 2, user);

      const string name                = asString(arguments, 3)->getValue();
      UpdateElementMode mode           = legacyUpdateElementMode(asInt32(arguments, 4));
      Element::ElementType elementType = legacyElementType(asInt32(arguments, 5));
      ArgumentMulti * children         = asMulti(arguments, 6); // name, type, factor
      bool append                      = legacyBool(asInt32(arguments, 7));

      // check if element exists
      Element * element = dimension->lookupElementByName(name);

      if (element == 0) {
        if (mode == UPDATE) {
          throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_FOUND,
                                   "cannot update non-existing element",
                                   "name", name);
        }
      }
      else {
        if (mode == FORCE_ADD) {
          throw ParameterException(ErrorException::ERROR_ELEMENT_EXISTS,
                                   "cannot add existing element", "name", name);
        }
        else if (mode == ADD) {
          return new ArgumentVoid();
        }
      }

      // create element if element does not exists
      bool isCreated = false;

      if (element == 0) {
        element   = dimension->addElement(name, elementType, user);
        isCreated = true;
      }

      // cleanup in case of error
      try {

        // change type
        if (element->getElementType() != elementType) {
          dimension->changeElementType(element, elementType, user, false);
        }

        if (elementType == Element::CONSOLIDATED) {
          if (! append) {
            dimension->removeChildren(user, element);
          }
          
          const vector<LegacyArgument*>* c = children->getValue();
          
          if (0 < c->size()) {
            ElementsWeightType children;
            
            for (size_t i = 0;  i < c->size();  i++) {
              const vector<LegacyArgument*>* cel = asMulti(c, i)->getValue();
              
              if (cel->size() != 3) {
                throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                         "expecting element description",
                                         "length of description", (int) cel->size());
              }
              
              Element * child = dimension->findElementByName(asString(cel, 0)->getValue(), user);
              double weight   = asDouble(cel, 2)->getValue();
              
              children.push_back(pair<Element*,double>(child, weight));
            }
            
            dimension->addChildren(element, &children, user);
          }
        }
      }
      catch (...) {
        if (isCreated) {
          dimension->deleteElement(element, user);
        }

        throw;
      }

      return new ArgumentVoid();
    }
  };

}

#endif

