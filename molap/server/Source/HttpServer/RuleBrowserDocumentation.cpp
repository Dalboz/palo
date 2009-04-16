////////////////////////////////////////////////////////////////////////////////
/// @brief provides cube rule  data documentation
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


#include "HttpServer/RuleBrowserDocumentation.h"

#include <iostream>
#include <sstream>

#include "Olap/Cube.h"
#include "Olap/Rule.h"

namespace palo {
  RuleBrowserDocumentation::RuleBrowserDocumentation (Database * database,
                                                      Cube * cube, 
                                                      const string& ruleString,
                                                      const string& message) 
    : BrowserDocumentation(message) {
    vector<string> ids(1, StringUtils::convertToString(database->getIdentifier()));
    values["@database_identifier"] = ids;

    vector<string> ids2(1, StringUtils::convertToString(cube->getIdentifier()));
    values["@cube_identifier"] = ids2;

    vector<string> newRule(1, ruleString);
    values["@rule_new_rule"] = newRule;
    
    vector<string> identifiers;
    vector<string> text;
    vector<string> external;
    vector<string> comment;
    vector<string> timestamp;
    vector<string> active;

    vector<Rule*> rules = cube->getRules(0);    

    if (rules.size() > 0) {
      for (vector<Rule*>::iterator i = rules.begin();  i != rules.end();  i++) {
        identifiers.push_back(StringUtils::convertToString((*i)->getIdentifier()));

        StringBuffer sb;
        sb.initialize();
        (*i)->appendRepresentation(&sb, true);
        text.push_back(StringUtils::escapeHtml(sb.c_str()));
        sb.free(); 

        external.push_back(StringUtils::escapeHtml((*i)->getExternal()));
        comment.push_back(StringUtils::escapeHtml((*i)->getComment()));
        timestamp.push_back(StringUtils::convertToString((uint32_t)((*i)->getTimeStamp())));
        
        if ((*i)->isActive()) {
          active.push_back("yes");
        }
        else {
          active.push_back("no");
        }
      }

      values["@rule_identifier"]  = identifiers;
      values["@rule_text"]  = text;
      values["@rule_external"] = external;
      values["@rule_comment"] = comment;
      values["@rule_timestamp"] = timestamp;
      values["@rule_active"] = active;
    }
  }
}
