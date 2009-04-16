////////////////////////////////////////////////////////////////////////////////
/// @brief provides cube data documentation
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


#include "HttpServer/CubeBrowserDocumentation.h"

#include <iostream>
#include <sstream>

#include "Olap/CellPath.h"
#include "Olap/Dimension.h"
#include "Olap/Rule.h"

namespace palo {
  CubeBrowserDocumentation::CubeBrowserDocumentation (Database * database,
                                                      Cube * cube, 
                                                      AreaStorage *storage, 
                                                      vector<IdentifiersType>* cellPaths, 
                                                      const string& pathString,
                                                      const string& message) 
    : BrowserDocumentation(message) {
    vector<string> ids(1, StringUtils::convertToString(database->getIdentifier()));
    values["@database_identifier"] = ids;

    defineCube(cube);
    defineDimensions(cube->getDimensions());
    
    vector<string> cellPathsS(1, pathString);
    values["@cell_path_value"] = cellPathsS;
    
    vector<string> identifiers;
    vector<string> type;
    vector<string> rule;
    vector<string> value;

    size_t last = storage->size();
    if (last > 1000) {
      last = 1000;
    }
    
    vector<IdentifiersType>::iterator iter = cellPaths->begin();
    for (size_t i = 0; i < last; i++, iter++) {
      identifiers.push_back(convertToString( &(*iter) ));

      Cube::CellValueType* v = storage->getCell(i); 
            
      if (v && v->type != Element::UNDEFINED) {
        type.push_back(convertElementTypeToString(v->type));
        rule.push_back(v->rule == Rule::NO_RULE ? "-" : StringUtils::convertToString(v->rule));
          
        switch(v->type) {
          case Element::NUMERIC: 
              value.push_back(StringUtils::convertToString(v->doubleValue));
              break;
            
          case Element::STRING:
              value.push_back(StringUtils::escapeHtml(v->charValue));
              break;

          default:
              value.push_back("*ERROR*");
              break;
        }
      }
      else if (v && v->type == Element::UNDEFINED) {
        if (v->doubleValue == 0.0) {
          type.push_back(convertElementTypeToString(Element::NUMERIC));
          value.push_back("-");
          rule.push_back(v->rule == Rule::NO_RULE ? "-" : StringUtils::convertToString(v->rule));
        }
        else {
          // error
          type.push_back("*ERROR*");
          value.push_back("*ERROR*");
          rule.push_back("-");
        }
      }
      else {
        // this should not happen:
        type.push_back("*ERROR*");
        value.push_back("*ERROR*");
      }
                
    }

    values["@cell_path"]  = identifiers;
    values["@cell_type"]  = type;
    values["@cell_value"] = value;
    values["@cell_rule"]  = rule;
  }
}
