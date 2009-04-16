////////////////////////////////////////////////////////////////////////////////
/// @brief rule parse handler
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

#ifndef HTTP_SERVER_RULE_PARSE_HANDLER_H
#define HTTP_SERVER_RULE_PARSE_HANDLER_H 1

#include "palo.h"

#include <string>
#include <iostream>

#include "Parser/RuleParserDriver.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief rule parser
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS RuleParseHandler : public PaloHttpHandler {
    
  public:
    RuleParseHandler (Server * server) 
      : PaloHttpHandler(server) {
    }

  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      const string& definition = request->getValue(DEFINITION);
      const string& functions = request->getValue(FUNCTIONS);
      bool validate = functions.empty();

      RuleNode * r = 0;

      // validate against cube data
      if (validate) {
        Database* database = findDatabase(request, 0);
        Cube* cube = findCube(database, request, 0);
      
        RuleParserDriver driver;

        driver.parse(definition);

        r = driver.getResult();

        if (r == 0) {
          throw ParameterException(ErrorException::ERROR_PARSING_RULE,
                                   driver.getErrorMessage(),
                                   "definition", definition);
        }

        string errorMsg;
        bool ok = r->validate(server, database, cube, errorMsg);

        if (!ok) {
          delete r;

          throw ParameterException(ErrorException::ERROR_PARSING_RULE,
                                   errorMsg,
                                   "definition", definition);
        }
      }

      // do no validate against cube data
      else {
        set<string> functionList;          

        string all = functions;
        size_t pos1 = 0;

        while (pos1 < all.size()) {
          string buffer = StringUtils::getNextElement(all, pos1, ',');

          if (! buffer.empty()) {
            functionList.insert(StringUtils::tolower(buffer));
          }            
        }

        RuleParserDriver driver(&functionList);

        driver.parse(definition);

        r = driver.getResult();

        if (r == 0) {
          throw ParameterException(ErrorException::ERROR_PARSING_RULE,
                                   driver.getErrorMessage(),
                                   "definition", definition);
        }

        string errorMsg;
        bool ok = r->validate(server, 0, 0, errorMsg);

        if (!ok) {
          delete r;

          throw ParameterException(ErrorException::ERROR_PARSING_RULE,
                                   errorMsg,
                                   "definition", definition);
        }
      }


      // generate output
      HttpResponse * response = new HttpResponse(HttpResponse::OK);
      response->setContentType("text/plain;charset=utf-8");
      StringBuffer& sb = response->getBody();

      r->appendXmlRepresentation(&sb, 0, ! validate);
      delete r;
      
      return response;
    }
  };

}

#endif
