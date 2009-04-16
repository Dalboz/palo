////////////////////////////////////////////////////////////////////////////////
/// @brief parser driver
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

#include "Parser/RuleParserDriver.h"
#include "Parser/RuleParser.h"
#include "Parser/FunctionNodeFactory.h"
#include "Parser/PaloFunctionNodeFactory.h"
#include "Parser/ParseFunctionNodeFactory.h"

#include <sstream>

RuleParserDriver::RuleParserDriver ()
  : trace_scanning(false), trace_parsing(false) {
  result = 0;
  factory = new PaloFunctionNodeFactory();
}


RuleParserDriver::RuleParserDriver (set<string>* functionList)
  : trace_scanning(false), trace_parsing(false) {
  result = 0;
  if (functionList && functionList->size() > 0) {
    factory = new ParseFunctionNodeFactory(functionList);
  }
  else {
    factory = new PaloFunctionNodeFactory();
  }
}


RuleParserDriver::~RuleParserDriver () {
  if (factory) {
    delete factory;
  }
}

void RuleParserDriver::parse (const string &rule) {
  ruleString = rule;
  scan_begin();
  yy::RuleParser parser(*this);
  parser.set_debug_level(trace_parsing);
  parser.parse();
  scan_end();
}

void RuleParserDriver::error (const yy::location& loc, const string& m) {
  stringstream s;

  yy::position last = loc.end - 1;
   
  s << m << " at position " << last.column << " of line "<< last.line;
  
  errorMessage = s.str();
}

void RuleParserDriver::error (const string& m) {
  errorMessage = m;
}


FunctionNode* RuleParserDriver::createFunction (const string& name, vector<Node*> *params) {
  return factory->createFunction(name, params);
}
