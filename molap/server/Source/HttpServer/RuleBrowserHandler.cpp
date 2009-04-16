////////////////////////////////////////////////////////////////////////////////
/// @brief provides cube rule browser
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


#include "HttpServer/RuleBrowserHandler.h"

#include "Exceptions/ParameterException.h"

#include "Collections/StringUtils.h"

#include "InputOutput/HtmlFormatter.h"

#include "Olap/Dimension.h"
#include "Olap/Rule.h"

#include "Parser/RuleParserDriver.h"

#include "HttpServer/RuleBrowserDocumentation.h"

namespace palo {
  HttpResponse * RuleBrowserHandler::handlePaloRequest (const HttpRequest * request) {
    string message;      
    Database* database = findDatabase(request, 0);
    Cube* cube = findCube(database, request, false);

    string ruleStringMessage;
    const string& idString = request->getValue("delete");
    const string& activateString = request->getValue("activate");
    if (! idString.empty() ) {
      long int id = StringUtils::stringToInteger(idString);
      cube->deleteRule(id, 0);
      message = createHtmlMessage( "Info", "rule deleted"); 
    }
    else if (!activateString.empty()) {
      long int id = StringUtils::stringToInteger(activateString);
      Rule* rule = cube->findRule(id, 0);
      if (rule) {
        if (rule->isActive()) {
          cube->activateRule(rule, false, 0);          
          message = createHtmlMessage( "Info", "rule deactivated");
        }
        else {
          cube->activateRule(rule, true, 0);          
          message = createHtmlMessage( "Info", "rule activated");
        }
      } 
    }
    else {
      const string& ruleString = request->getValue("rule");
      ruleStringMessage = StringUtils::escapeHtml(ruleString);
  
      if (! ruleStringMessage.empty()) {
        RuleParserDriver driver;
  
        driver.parse(ruleString);
        RuleNode* r = driver.getResult();
  
        if (r) {
          // validate parse tree
          string errorMsg;
          bool ok = r->validate(server, database, cube, errorMsg);
  
          if (!ok) {
            delete r;
            message = createHtmlMessage( "Error", errorMsg);
          }
          else {
            cube->createRule(r, "PALO", "generated via server browser", true, 0);
            message = createHtmlMessage( "Info", "added new rule");
          }
        }
        else {
          message = createHtmlMessage( "Error", driver.getErrorMessage());
        }
      }
    }

    HttpResponse * response = new HttpResponse(HttpResponse::OK);

    RuleBrowserDocumentation sbd(database, cube, ruleStringMessage, message);
    HtmlFormatter hf(templatePath + "/browser_cube_rule.tmpl");

    response->getBody().copy(hf.getDocumentation(&sbd));
    response->setContentType("text/html;charset=utf-8");
    
    return response;
  }
    
}
