////////////////////////////////////////////////////////////////////////////////
/// @brief provides element data documentation
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


#include "HttpServer/ElementBrowserDocumentation.h"

#include <functional>
#include <iostream>
#include <sstream>

#include "Olap/Dimension.h"

namespace palo {
  ElementBrowserDocumentation::ElementBrowserDocumentation (Database * database,
                                                            Dimension * dimension, 
                                                            Element * element,
                                                            const string& message) 
    : BrowserDocumentation(message) {
    vector<string> ids;
    ids.push_back(StringUtils::convertToString(database->getIdentifier()));
    values["@database_identifier"] = ids;

    vector<string> did;
    did.push_back(StringUtils::convertToString(dimension->getIdentifier()));
    values["@dimension_identifier"] = did;
    
    defineElement(dimension, element, "@element");

    // the parents of the element
    const Dimension::ParentsType * parents = dimension->getParents(element);
    defineElements(dimension, parents, "@parent");

    // the children of the element
    const ElementsWeightType * children = dimension->getChildren(element);
    vector<Element*> childrenElements;

    for (ElementsWeightType::const_iterator i = children->begin();  i != children->end();  i++) {
      childrenElements.push_back((*i).first);
    }

    defineElements(dimension, &childrenElements, "@child");

    // and their weight
    vector<string> weight;    

    for(ElementsWeightType::const_iterator i = children->begin();  i != children->end();  i++) {
      weight.push_back(StringUtils::convertToString((*i).second));
    }

    values["@child_weight"] = weight;
  }
}
