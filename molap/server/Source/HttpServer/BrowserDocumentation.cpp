////////////////////////////////////////////////////////////////////////////////
/// @brief provides data documentation
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


#include "HttpServer/BrowserDocumentation.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>

#include "Olap/NormalCube.h"
#include "Olap/NormalDimension.h"
#include "Olap/SystemCube.h"
#include "Olap/SystemDimension.h"

namespace palo {
  BrowserDocumentation::BrowserDocumentation (const string& message) {
    vector<string> m;
    m.push_back(message);
    values["@message"] = m;
    vector<string> x;
    x.push_back("Copyright 2006-2008 Jedox AG, written by triagens GmbH");
    values["@footerText"] = x;
  }



  bool BrowserDocumentation::hasDocumentationEntry (const string& name) {
    map<string, vector<string> >::const_iterator i = values.find(name);
    return (i == values.end()) ? false : true;
  }

    

  const vector<string>& BrowserDocumentation::getDocumentationEntries (const string& name) {
    static vector<string> error;
    map<string, vector<string> >::const_iterator i = values.find(name);
    return (i == values.end()) ? error : i->second;
  }



  const string& BrowserDocumentation::getDocumentationEntry (const string& name, size_t index) {
    static string error = "";

    map<string, vector<string> >::const_iterator i = values.find(name);

    if (i == values.end()) {
      return error;
    }

    const vector<string> * entries = &(i->second);

    if (index < 0 || entries->size() <= index) {
      return error;
    }

    return (*entries)[index];
  }
  
  
  
  string BrowserDocumentation::convertToString (const IdentifiersType* elements) {
    ostringstream s;
    bool start = true;
    
    for (IdentifiersType::const_iterator i = elements->begin(); i != elements->end(); i++) {
      if (start) {
        start = false;
      }
      else {
        s << ",";
      }

      s << (IdentifierType)(*i);
    }
    
    return s.str();
  }



  string BrowserDocumentation::convertElementTypeToString (Element::ElementType elementType) {
    switch(elementType) {
      case Element::NUMERIC: 
        return "numeric";

      case Element::STRING:
        return "string";

      case Element::CONSOLIDATED:
        return "consolidated";

      default:
        return "undefined";
    }
  }



  string BrowserDocumentation::convertStatusToString (Database* database) {
    switch (database->getStatus()) {
      case Database::UNLOADED:
        return "not loaded";

      case Database::LOADED:
        return "loaded/saved";

      case Database::CHANGED:
        return "changed/new";

      default :
        return "error";
    }      
  }



  string BrowserDocumentation::convertStatusToString (Cube* cube) {
    switch (cube->getStatus()) {
      case Cube::UNLOADED:
        return "not loaded";

      case Cube::LOADED:
        return "loaded/saved";

      case Cube::CHANGED:
        return "changed/new";

      default :
        return "error";
    }      
  }



  string BrowserDocumentation::convertTypeToString (ItemType type) {
    switch (type) {
      case NORMAL:
        return "normal";

      case SYSTEM:
        return "system";

      default :
        return "error";
    }      
  }



  string BrowserDocumentation::convertTypeToString (Dimension* dimension) {
    ItemType type = dimension->getType();
    if (type == NORMAL) {
      return "normal";
    }
    else if (type == SYSTEM) {
      SystemDimension* d =  dynamic_cast<SystemDimension*>(dimension);
      if (d->getSubType() == SystemDimension::ATTRIBUTE_DIMENSION) { 
        return "attributes";
      }
      return "system";
    }
    else if (type == USER_INFO) {
      return "user info";
    }
    return "error";
  }



  string BrowserDocumentation::convertTypeToString (Cube* cube) {
    ItemType type = cube->getType();
    if (type == NORMAL) {
      return "normal";
    }
    else if (type == USER_INFO) {
      return "user info";
    }
    else if (type == SYSTEM) {
      SystemCube* c =  dynamic_cast<SystemCube*>(cube);
      if (c->getSubType() == SystemCube::ATTRIBUTES_CUBE) { 
        return "attributes";
      }
      return "system";
    }
    return "error";
  }



  void BrowserDocumentation::defineDatabase (Database* database) {
    vector<Database*> databases(1, database);
    defineDatabases(&databases);
  }



  void BrowserDocumentation::defineDatabases (const vector<Database*> * databases) {
    vector<string> idenfier;
    vector<string> name;
    vector<string> status;
    vector<string> type;
    vector<string> dimensions;
    vector<string> cubes;

    for (vector<Database*>::const_iterator i = databases->begin();  i != databases->end();  i++) {
      Database * database = *i;

      idenfier.push_back(  StringUtils::convertToString(database->getIdentifier()));
      name.push_back(      StringUtils::escapeHtml(database->getName()));
      status.push_back(    convertStatusToString(database));
      type.push_back(      convertTypeToString(database->getType()));
      dimensions.push_back(StringUtils::convertToString((uint32_t) database->sizeDimensions()));
      cubes.push_back(     StringUtils::convertToString((uint32_t) database->sizeCubes()));
    }
      
    values["@database_identifier"]     = idenfier;
    values["@database_name"]           = name;
    values["@database_status"]         = status;
    values["@database_type"]           = type;
    values["@database_num_dimensions"] = dimensions;
    values["@database_num_cubes"]      = cubes;
  }



  void BrowserDocumentation::defineDimension (Dimension* dimension) {
    vector<Dimension*> dimensions(1, dimension);
    defineDimensions(&dimensions);
  }



  void BrowserDocumentation::defineDimensions (const vector<Dimension*> * dimensions) {
    vector<string> identifier;
    vector<string> name;
    vector<string> numElements;
    vector<string> maxLevel;
    vector<string> maxIndent;
    vector<string> maxDepth;
    vector<string> type;
    vector<string> numBaseElements;    
    vector<string> numCells;    

    double nCells = 1; 
    for (vector<Dimension*>::const_iterator i = dimensions->begin();  i != dimensions->end();  i++) {
      Dimension * dimension = *i;

      identifier.push_back( StringUtils::convertToString(dimension->getIdentifier()));
      name.push_back(       StringUtils::escapeHtml(dimension->getName()));
      numElements.push_back(StringUtils::convertToString((int32_t) dimension->sizeElements()));
      maxLevel.push_back(   StringUtils::convertToString((int32_t) dimension->getLevel()));
      maxIndent.push_back(  StringUtils::convertToString((int32_t) dimension->getIndent()));
      maxDepth.push_back(   StringUtils::convertToString((int32_t) dimension->getDepth()));        
      type.push_back(       convertTypeToString(dimension));
      
      int32_t num = 0; 
      vector<Element*> el = dimension->getElements(0);
      for (vector<Element*>::iterator j = el.begin(); j != el.end(); j++) {
        if ((*j)->getElementType() == Element::NUMERIC) {
          num++;
        }
      }
      numBaseElements.push_back(StringUtils::convertToString(num));
      nCells *= num;
    }
    numCells.push_back(StringUtils::convertToString(nCells));

    values["@dimension_identifier"]   = identifier;
    values["@dimension_name"]         = name;
    values["@dimension_num_elements"] = numElements;
    values["@dimension_max_level"]    = maxLevel;
    values["@dimension_max_indent"]   = maxIndent;
    values["@dimension_max_depth"]    = maxDepth;
    values["@dimension_type"]         = type;
    values["@dimension_numeric_elements"] = numBaseElements;
    values["@dimension_numeric_cells"] = numCells;
  }



  void BrowserDocumentation::defineCube (Cube* cube) {
    vector<Cube*> cubes(1, cube);
    defineCubes(&cubes);
  }



  void BrowserDocumentation::defineCubes (const vector<Cube*> * cubes) {
    vector<string> identifier;
    vector<string> name;
    vector<string> dimensions;
    vector<string> status;    
    vector<string> type;    
    vector<string> size;    

    for (vector<Cube*>::const_iterator i = cubes->begin();  i != cubes->end();  i++) {
      Cube * cube = *i;

      identifier.push_back(StringUtils::convertToString(cube->getIdentifier()));
      name.push_back(      StringUtils::escapeHtml(cube->getName()));

      // convert dimension list into string
      const vector<Dimension*>* dims = cube->getDimensions();
      IdentifiersType identifiers(dims->size(), 0);
      transform(dims->begin(), dims->end(), identifiers.begin(), mem_fun(&Dimension::getIdentifier));

      dimensions.push_back(convertToString(&identifiers));
      status.push_back(convertStatusToString(cube));
      type.push_back(convertTypeToString(cube));
      size.push_back(StringUtils::convertToString(cube->sizeFilledCells()));
    }

    values["@cube_identifier"]  = identifier;
    values["@cube_name"]        = name;
    values["@cube_status"]      = status;
    values["@cube_type"]        = type;
    values["@cube_dimensions"]  = dimensions;
    values["@cube_size"]        = size;
  }



  void BrowserDocumentation::defineElement (Dimension * dimension,
                                            Element* element,
                                            const string& prefix) {
    vector<Element*> elements(1, element);
    defineElements(dimension, &elements, prefix);
  }



  void BrowserDocumentation::defineElements (Dimension * dimension,
                                             const vector<Element*> * elements,
                                             const string& prefix) {
    vector<string> idenifier;
    vector<string> name;
    vector<string> position;    
    vector<string> level;
    vector<string> indent;
    vector<string> depths;
    vector<string> parents;
    vector<string> children;
    vector<string> type;
    vector<string> strCons;

    for(vector<Element*>::const_iterator i = elements->begin();  i != elements->end();  i++) {
      Element * element = *i;

      idenifier.push_back(StringUtils::convertToString(element->getIdentifier()));
      name.push_back(     StringUtils::escapeHtml(element->getName()));
      position.push_back( StringUtils::convertToString(element->getPosition()));
      level.push_back(    StringUtils::convertToString(element->getLevel(dimension)));
      indent.push_back(   StringUtils::convertToString(element->getIndent(dimension)));
      depths.push_back(   StringUtils::convertToString(element->getDepth(dimension)));
      children.push_back( StringUtils::convertToString((int32_t) dimension->getChildren(element)->size()));
      parents.push_back(  StringUtils::convertToString((int32_t) dimension->getParents(element)->size()));
      type.push_back(     convertElementTypeToString(element->getElementType()));
      strCons.push_back(  dimension->isStringConsolidation(element) ? "yes" : "no" );
    }      

    values[prefix + "_identifier"]           = idenifier;
    values[prefix + "_name"]                 = name;
    values[prefix + "_position"]             = position;
    values[prefix + "_level"]                = level;
    values[prefix + "_indent"]               = indent;
    values[prefix + "_depth"]                = depths;
    values[prefix + "_parents"]              = parents;
    values[prefix + "_children"]             = children;
    values[prefix + "_type"]                 = type;
    values[prefix + "_string_consolidation"] = strCons;
  }
}
