////////////////////////////////////////////////////////////////////////////////
/// @brief provides cube browser
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


#include "HttpServer/CubeBrowserHandler.h"

#include "Exceptions/ParameterException.h"

#include "Collections/StringUtils.h"

#include "InputOutput/HtmlFormatter.h"

#include "Olap/CellPath.h"
#include "Olap/Dimension.h"

#include "HttpServer/CubeBrowserDocumentation.h"

namespace palo {
  HttpResponse * CubeBrowserHandler::handlePaloRequest (const HttpRequest * request) {
    string message;      
    Database* database = findDatabase(request, 0);
    Cube* cube = findCube(database, request, false);

    string action = request->getValue("action");
      
    if (! action.empty()) {
      try {
        if (action == "load") {
          database->loadCube(cube, user);
          message = "cube loaded";
        }
        else if (action == "save") {
          database->saveCube(cube, user);
          message = "cube saved";
        }
          
        if (! message.empty()) {
          message = createHtmlMessage( "Info", message);
        }
      }
      catch (const ErrorException& e) {
        message = createHtmlMessage( "Error", e.getMessage());
      }
    }

    vector<CellPath> cellPath; 
    const string& pathString = request->getValue("path");
    string pathStringMessage = pathString;

    // get cube cells
    const vector<Dimension*> * dimensions = cube->getDimensions();
    uint32_t numResult;
    vector<IdentifiersType> area;

    if (pathString.empty()) {
      pathStringMessage = "0";

      for (size_t i = 1;  i < cube->getDimensions()->size();  i++) {
        pathStringMessage += ",0";
      }

      numResult = 0;
      area.resize(dimensions->size());
    }
    else {
      try {
        area = getArea(pathStringMessage, dimensions, numResult, true, false);
      }
      catch (ParameterException e) {
        area.resize(dimensions->size());
        numResult = 0;
        message = createHtmlMessage( "Error", e.getMessage());
      }
    }
          
    vector<IdentifiersType> cellPaths;
    AreaStorage storage(dimensions, &area, numResult, false);
    cube->getAreaCellValues(&storage, &area, &cellPaths, user);

    HttpResponse * response = new HttpResponse(HttpResponse::OK);

    CubeBrowserDocumentation sbd(database, cube, &storage, &cellPaths, pathStringMessage, message);
    HtmlFormatter hf(templatePath + "/browser_cube.tmpl");

    response->getBody().copy(hf.getDocumentation(&sbd));
    response->setContentType("text/html;charset=utf-8");
    
    return response;
  }

    
    
  void CubeBrowserHandler::buildCellPath (Cube * cube,
                                          const vector<Dimension*>* dimensions,
                                          uint32_t level,
                                          vector<string> * pathIdentifier,
                                          IdentifiersType * path, 
                                          vector<CellPath> * cellPath,
                                          uint32_t * counter) {
    if (*counter > 1000) {
      return;
    }
      
    if (level == dimensions->size()) {
      CellPath cp(cube, path);
      cellPath->push_back(cp);
      (*counter)++;
    }
    else {
      string x = pathIdentifier->at(level);
      
      if (x != "*") {
        IdentifierType i = StringUtils::stringToInteger(x);
        path->at(level) = i;
        buildCellPath(cube, dimensions, level+1, pathIdentifier, path, cellPath, counter);
      }
      else {
        vector<Element*> elements = dimensions->at(level)->getElements(0);
        
        for (vector<Element*>::iterator i = elements.begin(); i != elements.end(); i++){
          path->at(level) = (*i)->getIdentifier();
          buildCellPath(cube, dimensions, level+1, pathIdentifier, path, cellPath, counter);
        }
      }
    }
  }
}
