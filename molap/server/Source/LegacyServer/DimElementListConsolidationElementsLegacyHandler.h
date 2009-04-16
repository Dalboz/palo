////////////////////////////////////////////////////////////////////////////////
/// @brief dim_element_list_consolidation_elements
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

#ifndef LEGACY_SERVER_DIM_ELEMENT_LIST_CONSOLIDATION_ELEMENTS_LEGACY_HANDLER_H
#define LEGACY_SERVER_DIM_ELEMENT_LIST_CONSOLIDATION_ELEMENTS_LEGACY_HANDLER_H 1

#include "palo.h"

#include "Olap/Database.h"
#include "Exceptions/ParameterException.h"
#include "Olap/Server.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief dim_element_list_consolidation_elements
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS DimElementListConsolidationElementsLegacyHandler : public LegacyRequestHandler {
  public:
    DimElementListConsolidationElementsLegacyHandler (Server * server) 
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::dim_element_list_consolidation_elements");

      if (arguments->size() != 4) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting database, dimension, and element as arguments for dim_element_list_consolidation_elements",
                                 "number of arguments", (int) arguments->size() - 1);
      }

      Database * database   = findDatabase(arguments, 1, user);
      Dimension * dimension = findDimension(database, arguments, 2, user);
      Element * element     = findElement(dimension, arguments, 3, user);

      ArgumentMulti * multi = new ArgumentMulti();

      const ElementsWeightType * childrenList = dimension->getChildren(element);    

      for (ElementsWeightType::const_iterator cIter = childrenList->begin();  cIter != childrenList->end();  cIter++) {
        ArgumentMulti * c = new ArgumentMulti();
        Element * child = (*cIter).first;

        c->append(new ArgumentString(child->getName()));
        c->append(new ArgumentInt32(legacyType(child->getElementType())));
        c->append(new ArgumentDouble((*cIter).second));
    
        multi->append(c);
      }

      return multi;
    }
  };

}

#endif

