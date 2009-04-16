////////////////////////////////////////////////////////////////////////////////
/// @brief rule create handler
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

#ifndef HTTP_SERVER_RULE_CREATE_HANDLER_H
#define HTTP_SERVER_RULE_CREATE_HANDLER_H 1

#include "palo.h"

#include <string>
#include <iostream>

#include "Olap/Rule.h"

#include "Parser/RuleParserDriver.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief element browser
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS RuleCreateHandler : public PaloHttpHandler {
    
  public:
    RuleCreateHandler (Server * server) 
      : PaloHttpHandler(server) {
    }

  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      Database* database   = findDatabase(request, 0);
      Cube* cube = findCube(database, request, 0);

      checkToken(cube, request);

      const string& definition = request->getValue(DEFINITION);
      const string& activate = request->getValue(ACTIVATE);

      RuleParserDriver driver;

      driver.parse(definition);
      RuleNode* r = driver.getResult();

      if (r) {          
          
        // display result as identifier or name
        bool useIdentifier = false;
        const string& e = request->getValue(USE_IDENTIFIER);

        if (!e.empty() && e == "1") {
          useIdentifier = true;
        }

        // validate parse tree
        string errorMsg;
        bool ok = r->validate(server, database, cube, errorMsg);

        if (!ok) {
          delete r;
          throw ParameterException(ErrorException::ERROR_PARSING_RULE,
                                   errorMsg,
                                   "definition", definition);
        }

        const string& external = request->getValue(EXTERNAL_IDENTIFIER);
        const string& comment = request->getValue(COMMENT);
        
        bool isActive = true;
        if (activate == "0") {
          isActive = false;
        }

        return generateRuleResponse(cube->createRule(r, external, comment, isActive, user), useIdentifier);
      }
      else {

        // got no parse tree 
        throw ParameterException(ErrorException::ERROR_PARSING_RULE,
                                 driver.getErrorMessage(),
                                 "definition", definition);
      }
    }
  };

}

#endif
