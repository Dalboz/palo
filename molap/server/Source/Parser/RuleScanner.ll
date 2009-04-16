%{                                            /* -*- C++ -*- */
#include <cstdlib>
#include <errno.h>
#include <limits.h>
#include <string>
#include "Parser/RuleParserDriver.h"
#include "Parser/RuleParser.h"
#include "Collections/StringUtils.h"
#include "InputOutput/Logger.h"

#if defined(_MSC_VER)
#pragma warning( disable : 4018 )
#endif

/* Work around an incompatibility in flex (at least versions
   2.5.31 through 2.5.33): it generates code that does
   not conform to C89.  See Debian bug 333231
   <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.  */
# undef yywrap
# define yywrap() 1

/* By default yylex returns int, we use token_type.
   Unfortunately yyterminate by default returns 0, which is
   not of token_type.  */
#define yyterminate() return token::END

std::string myStringBuffer;

%}

%option noyywrap nounput batch debug
%x STRINGS
%x EL_STRINGS
%x VAR_STRINGS

id            [a-zA-Z][a-zA-Z_0-9]*
blank         [ \t]
ZERO          [0]
DIGIT         [0-9]
DIGIT1        [1-9]
FUNC_NAME     [a-zA-Z][a-zA-Z_\.0-9]*

%{
# define YY_USER_ACTION  yylloc->columns (yyleng);
%}
%%
%{
  yylloc->step ();
%}
{blank}+   yylloc->step ();
[\n]+      yylloc->lines (yyleng); yylloc->step ();

%{
  typedef yy::RuleParser::token token;
%}


 /* normal strings */

\"               { BEGIN STRINGS; myStringBuffer = ""; }

<STRINGS>\"\"    { myStringBuffer.append("\""); }

<STRINGS>\"      {
                   BEGIN 0;
                   yylval->sval = new std::string (myStringBuffer);
                   return token::STRING;
                 }

<STRINGS>.       { myStringBuffer.append(1, yytext[0]); }


 /* strings for dimension and element names */

'                { BEGIN EL_STRINGS; myStringBuffer = ""; }

<EL_STRINGS>''   { myStringBuffer.append("'"); }

<EL_STRINGS>'    {
                   BEGIN 0;
                   yylval->sval = new std::string (myStringBuffer);
                   return token::EL_STRING;
                 }

<EL_STRINGS>.    { myStringBuffer.append(1, yytext[0]); }

 /* strings for element variables */

!'               { BEGIN VAR_STRINGS; myStringBuffer = ""; }

<VAR_STRINGS>''  { myStringBuffer.append("'"); }

<VAR_STRINGS>'   {
                   BEGIN 0;
                   yylval->sval = new std::string (myStringBuffer);
                   return token::VAR_STRING;
                 }

<VAR_STRINGS>.   { myStringBuffer.append(1, yytext[0]); }



[-\+\*/\[\],:@\{\}\(\)] { return yy::RuleParser::token_type (yytext[0]); }

">="            { return token::GE; }
"<="            { return token::LE; }
"=="            { return token::EQ; }
"!="|"<>"       { return token::NE; }
">"             { return token::GT; }
"<"             { return token::LT; }
"="             { return token::ASSIGN; }
"[["            { return token::MARKER_OPEN; }
"]]"            { return token::MARKER_CLOSE; }
"{{"            { return token::SMARKER_OPEN; }
"}}"            { return token::SMARKER_CLOSE; }

"C:"            { return token::CONS_FLAG; }
"N:"            { return token::BASE_FLAG; }

{DIGIT1}{DIGIT}* {
                   char *p;
                   yylval->ival = strtol(yytext, &p, 10);
                   if (*p != '\0') {
                     Logger::error << "error in number\n" << std::endl;
                     yylval->ival = 0;
                   }
                   return token::INTEGER;
                 }

{ZERO}                                   {
                   yylval->ival = 0;
                   return token::INTEGER;
                                                                 }

{DIGIT1}{DIGIT}*\.{DIGIT}* {
                   char *p;
                   yylval->dval = strtod(yytext, &p);
                   if (*p != '\0') {
                     Logger::error << "error in number\n" << std::endl;
                     yylval->dval = 0.0;
                   }
                   return token::DOUBLE;
                 }

{ZERO}\.{DIGIT}* {
                   char *p;
                   yylval->dval = strtod(yytext, &p);
                   if (*p != '\0') {
                     Logger::error << "error in number\n" << std::endl;
                     yylval->dval = 0.0;
                   }
                   return token::DOUBLE;
                 }

{FUNC_NAME}      {
                   yylval->sval = new std::string (StringUtils::tolower(yytext));
                   return token::FUNCTION;
                 }


.                {
                   Logger::error << "error unknown character " << *yytext << endl;
                   driver.error (*yylloc, "invalid character");
                 }


%%

void RuleParserDriver::scan_begin () {
  // yy_flex_debug = trace_scanning;
  yy_flex_debug = 0;
  yy_scan_string( ruleString.c_str() );
}

void RuleParserDriver::scan_end () {
}
