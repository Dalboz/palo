////////////////////////////////////////////////////////////////////////////////
/// @brief cells values
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

#ifndef HTTP_SERVER_CELL_VALUES_HANDLER_H
#define HTTP_SERVER_CELL_VALUES_HANDLER_H 1

#include "palo.h"

#include <string>

#include "Olap/Rule.h"

#include "HttpServer/PaloHttpHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief cells value
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CellValuesHandler : public PaloHttpHandler {
  public:
    CellValuesHandler (Server * server) 
      : PaloHttpHandler(server) {
    }

  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      Database* database = findDatabase(request, 0);
      Cube* cube         = findCube(database, request, 0);
      
      checkToken(cube, request);

      const string& pathIds = request->getValue(ID_PATHS);
      
      // Logger::debug << "CellValuesHandler: " << pathIds << endl;
      
      bool usePathIds = ! pathIds.empty();
      const string& pathString = usePathIds ? pathIds : request->getValue(NAME_PATHS);

      if (pathString.empty()) {
        throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                 "path is empty, list of element identifiers is missing",
                                 ID_ELEMENT, "");
      }

      string allPath = pathString;
      
      const vector<Dimension*> * dimensions = cube->getDimensions();
      
      vector<IdentifiersType> cellPathes;
      IdentifiersType emptyPath;

      for (size_t pos1 = 0;  pos1 < allPath.size(); ) {
        string buffer = StringUtils::getNextElement(allPath, pos1, ':');

        try {
          cellPathes.push_back(getPath(buffer, dimensions, usePathIds));
        }
        catch (ParameterException e) {
          cellPathes.push_back(emptyPath);
        }
      }
      
      string showRuleParam = request->getValue(SHOW_RULE);
      bool showRule = false;

      if (! showRuleParam.empty() && showRuleParam == "1") {
        showRule = true;
      }

      size_t numResult = cellPathes.size();
      AreaStorage storage(dimensions, &cellPathes, numResult, true);
      cube->getListCellValues(&storage, &cellPathes, user);              
      
      HttpResponse * response = new HttpResponse(HttpResponse::OK);
      response->setToken(X_PALO_CUBE, cube->getToken());
      
      size_t last = storage.size();
      vector<IdentifiersType>::iterator iter = cellPathes.begin();

	  Cube::CellLockInfo lockInfo = 0;	  
	  const string& showLockParam = request->getValue(SHOW_LOCK_INFO);
	  bool showLockInfo = showLockParam == "1";
	  IdentifierType userId = user ? user->getIdentifier() : 0;

      for (size_t i = 0; i < last; i++, iter++) {
        Cube::CellValueType* value = storage.getCell(i); 
		if (showLockInfo) {
			try {
				CellPath cp(cube, &(*iter)); //may throw an exception if path is not in correct format
				lockInfo = cube->getCellLockInfo(&cp, userId);		
			} catch (...) {
				lockInfo = 0;
			}
		}
        buildResponse(value, &response->getBody(), showRule, showLockInfo, lockInfo);    
      }
      
      return response;
    }
    
    void buildResponse (Cube::CellValueType* value,
                        StringBuffer* sb,
                        bool showRule, 
						bool showLockInfo,
						Cube::CellLockInfo lockInfo) {

      if (value && value->type != Element::UNDEFINED) {
        appendCsvInteger(sb, (int32_t) value->type);
                  
        switch(value->type) {
          case Element::NUMERIC: 
              appendCsvString(sb, "1");
              appendCsvDouble(sb, value->doubleValue);
              break;
            
          case Element::STRING:
              if (value->charValue.empty()) {
                appendCsvString(sb, "0;");
              }
              else {
                appendCsvString(sb, "1");
                appendCsvString(sb, StringUtils::escapeString(value->charValue));
              }
              break;

          default:
              appendCsvString(sb, "1");
              appendCsvString(sb, "");
              break;
        }
      }
      else if (value && value->type == Element::UNDEFINED) {
        if (value->doubleValue == 0.0) {
          // no value found
          appendCsvInteger(sb, (int32_t) Element::NUMERIC);
          appendCsvString(sb, "0;");
        }
        else {
          // error
          appendCsvInteger(sb, (int32_t) 99);
          appendCsvInteger(sb, (int32_t) value->doubleValue);
          appendCsvString( sb, "cannot evaluate path");
        }
      }
      else {
        // this should not happen:
        appendCsvInteger(sb, (int32_t) 99);
        appendCsvInteger(sb, (int32_t) ErrorException::ERROR_INTERNAL);
        appendCsvString( sb, ErrorException::getDescriptionErrorType(ErrorException::ERROR_INTERNAL));
      }      

      if (showRule) {
        if (value && value->rule != Rule::NO_RULE) {
          appendCsvInteger(sb, (uint32_t) value->rule);
        }
        else {
          sb->appendChar(';');
        }
      }

	  if (showLockInfo)
		  appendCsvInteger(sb, lockInfo);

      sb->appendEol();
    }
    
  };

}

#endif
