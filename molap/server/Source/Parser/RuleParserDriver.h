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

#ifndef PARSER_DRIVER_H
#define PARSER_DRIVER_H 1

#include <string>
#include <map>

#include "Parser/DestinationNode.h"
#include "Parser/DoubleNode.h"
#include "Parser/ExprNode.h"
#include "Parser/FunctionNode.h"
#include "Parser/FunctionNodeFactory.h"
#include "Parser/RuleNode.h"
#include "Parser/SourceNode.h"
#include "Parser/StringNode.h"

#include "Parser/RuleParser.h"

#if defined(_MSC_VER)
#pragma warning( disable : 4267 4244 4800 )
#endif

// Announce to Flex the prototype we want for lexing function, ...
# define YY_DECL                                        \
  yy::RuleParser::token_type                         \
  yylex (yy::RuleParser::semantic_type* yylval,      \
         yy::RuleParser::location_type* yylloc,      \
         RuleParserDriver& driver)
// ... and declare it for the parser's sake.

YY_DECL;

using namespace std;

// Conducting the whole scanning and parsing of Calc++.
class RuleParserDriver {

  public:
    RuleParserDriver ();
    RuleParserDriver (set<string>* functionList);
    virtual ~RuleParserDriver ();

    RuleNode* getResult () {
      return result;
    }

    void setResult (RuleNode* node) {
      result = node;
    }

    string getErrorMessage () {
      return errorMessage;
    }
     
    // Handling the scanner.
    void scan_begin ();
    void scan_end ();
    bool trace_scanning;

    // Handling the parser.
    void parse (const string&);
    
    string ruleString;
    bool trace_parsing;

    // Error handling.
    void error (const yy::location& l, const string& m);
    void error (const string& m);

    FunctionNode* createFunction (const string& name, vector<Node*> *params);
    
  private:
    RuleNode* result;
    string    errorMessage;
    FunctionNodeFactory* factory;

};

#endif
