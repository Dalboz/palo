////////////////////////////////////////////////////////////////////////////////
/// @brief cell export
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

#ifndef HTTP_SERVER_CELL_EXPORT_HANDLER_H
#define HTTP_SERVER_CELL_EXPORT_HANDLER_H 1

#include "palo.h"

#include <string>

#include "InputOutput/Condition.h"

#include "Olap/CubeLooper.h"
#include "Olap/ExportStorage.h"

#include "HttpServer/PaloHttpHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief cell export
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CellExportHandler : public PaloHttpHandler {
  public:
    CellExportHandler (Server * server) 
      : PaloHttpHandler(server) {
    }

  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      Database* database = findDatabase(request, 0);
      cube = findCube(database, request, 0);

      checkToken(cube, request);

      // base cells or all cells
      baseOnly = false;
      const string& base_only = request->getValue(BASE_ONLY);

      if (!base_only.empty() && base_only == "1") {
        baseOnly = true;
      }

      // base cells or all cells
      skipEmpty = true;
      const string& e = request->getValue(SKIP_EMPTY);

      if (!e.empty() && e == "0") {
        skipEmpty = false;
      }

      // blocksize
      blocksize = 1000;
      const string& bs = request->getValue(BLOCKSIZE);

      if (! bs.empty()) {
        blocksize = StringUtils::stringToUnsignedInteger(bs);
      }

      // condition
      const string& cd = request->getValue(CONDITION);

      if (cd.empty()) {
        condition = 0;
      }
      else {
        condition = Condition::parseCondition(cd);
      }

      // begin after
      const string& pathIds = request->getValue(ID_PATH);
      
      bool usePathIds = ! pathIds.empty();
      const string& pathString = usePathIds ? pathIds : request->getValue(NAME_PATH);

      // export rule based cell values?
      bool useRules = getBoolean(request, USE_RULES, false);
      
      IdentifiersType beginAfter;
      bool useBeginAfter = false;

      // setup dimensions
      dimensions = cube->getDimensions();
      numDimensions = dimensions->size();

      if (! pathString.empty()) {
        beginAfter = getPath(pathString, dimensions, usePathIds);
        useBeginAfter = true;
      }

      // setup area
      area.clear();
      area.resize(numDimensions);
      
      const string& areaIds = request->getValue(ID_AREA);
      
      bool useAreaIds = ! areaIds.empty();
      const string& areaString = useAreaIds ? areaIds : request->getValue(NAME_AREA);

      if (areaString.empty()) {
        for (size_t i = 0;  i < numDimensions;  i++) {
          vector<Element*> elements = (*dimensions)[i]->getElements(0);

          for (vector<Element*>::iterator j = elements.begin();  j != elements.end();  j++) {
            Element * element = *j;

            if (! baseOnly || (element->getElementType() == Element::NUMERIC || element->getElementType() == Element::STRING)) {
              area[i].insert(element->getIdentifier());
            }
          }
        }
      }
      else {
        uint32_t numResult;
        vector<IdentifiersType> a = getArea(areaString, dimensions, numResult, useAreaIds, false);

        for (size_t i = 0;  i < numDimensions;  i++) {
          area[i].insert(a[i].begin(), a[i].end());
        }
      }
      
      // create a sorted area
      vector<IdentifiersType> vectorArea;
      vectorArea.resize(numDimensions);
      for (size_t i = 0;  i < numDimensions;  i++) {
        vectorArea[i].insert(vectorArea[i].begin(), area[i].begin(), area[i].end());
      }

      HttpResponse * response = new HttpResponse(HttpResponse::OK);
      response->setToken(X_PALO_CUBE, cube->getToken());
      StringBuffer * stringBuffer = &response->getBody();           

      try {

        // create an export storage
        ExportStorage storage(dimensions, &vectorArea, blocksize);
      
        // compute values        
        vector<IdentifiersType> resultPaths;
        cube->getExportValues(&storage, 
                             &vectorArea,  
                             &resultPaths,
                             &beginAfter,
                             useBeginAfter,
                             useRules,
                             baseOnly,
                             skipEmpty,
                             condition,
                             user);

        // build response                      
        size_t last = storage.size();
        IdentifiersType path(area.size());
        for (size_t i = 0; i < last; i++) {
          Cube::CellValueType* value = storage.getCell(i, &path); 
          buildResponse(value, &path, stringBuffer);    
        }

        appendCsvDouble(stringBuffer, storage.getPosNumber());
        appendCsvDouble(stringBuffer, storage.getAreaSize());
        stringBuffer->appendEol();
                
        delete condition;
        return response;            
      }
      catch (...) {
        delete response;
        if (condition != 0) { 
            delete condition; 
        }
        throw;
      }        
    }

  public:

    void buildResponse (Cube::CellValueType* value,
                        const IdentifiersType* path,
                        StringBuffer* sb) {

      if (value->type != Element::UNDEFINED) {
        appendCsvInteger(sb, (int32_t) value->type);
        
        appendCsvString(sb, "1");
          
        switch(value->type) {
          case Element::NUMERIC: 
              appendCsvDouble(sb, value->doubleValue);
              break;
            
          case Element::STRING:
              appendCsvString(sb, StringUtils::escapeString(value->charValue));
              break;

          default:
              appendCsvString(sb, "");
              break;
        }
      }
      else if (value->type == Element::UNDEFINED) {
        // empty value found
        CellPath cellPath(cube, path);
        if (cellPath.getPathType() == Element::STRING) {
          appendCsvInteger(sb, (int32_t) Element::STRING);
          appendCsvString(sb, "0;");
        }
        else {
          appendCsvInteger(sb, (int32_t) Element::NUMERIC);
          appendCsvString(sb, "0;");
        }
      }
      
      for (size_t i = 0;  i < path->size();  i++) {
        if (0 < i) { sb->appendChar(','); }
        sb->appendInteger(path->at(i));
      }

      sb->appendChar(';');
      sb->appendEol();
    }
    

  private:
    Cube* cube;
    uint32_t blocksize;
    HttpResponse * response;
    StringBuffer * stringBuffer;
    Condition * condition;
    bool baseOnly;
    bool skipEmpty;
    const vector<Dimension*> * dimensions;
    size_t numDimensions;
    vector< set<IdentifierType> > area;
  };

}

#endif
