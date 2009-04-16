%skeleton "lalr1.cc"                          /*  -*- C++ -*- */
%defines
%define "parser_class_name" "RuleParser"

%{
# include <string>
# include <vector>
# include <utility>
class RuleParserDriver;

#include "Parser/DestinationNode.h"
#include "Parser/ExprNode.h"
#include "Parser/Node.h"
#include "Parser/SourceNode.h"
#include "Parser/VariableNode.h"
using namespace palo;
using namespace std;
%}

// The parsing context.
%parse-param { RuleParserDriver& driver }
%lex-param   { RuleParserDriver& driver }

%locations
%initial-action
{
  // Initialize the initial location.
  @$.begin.filename = @$.end.filename = &driver.ruleString;
};
%debug
%error-verbose
// Symbols.
%union
{
  int                               ival;
  double                            dval;
  std::string                       *sval;
  DestinationNode                   *dnval;
  ExprNode                          *enval;
  Node                              *nval;
  vector< pair<string*, string*> *> *vpsval;
  vector< pair<int, int> *>         *vpival;
  pair<string*, string*>            *psval;
  pair<int, int>                    *pival;
  vector< Node* >                   *vpnval;
};

%{
# include "RuleParserDriver.h"
%}

%token                 END      0       "end of file"
%token                 ASSIGN           "="
%token                 CONS_FLAG        
%token                 BASE_FLAG
%token                 GE 
%token                 LE 
%token                 EQ 
%token                 NE 
%token                 GT 
%token                 LT
%token                 MARKER_OPEN
%token                 MARKER_CLOSE
%token                 SMARKER_OPEN
%token                 SMARKER_CLOSE

%token   <sval>        STRING           "string"
%token   <sval>        EL_STRING        "element_string"
%token   <sval>        VAR_STRING       "variable_string"
%token   <sval>        FUNCTION         "function"
%token   <ival>        INTEGER          "integer"
%token   <dval>        DOUBLE           "double"

%type    <enval>       exp
%type    <dnval>       destination
%type    <enval>       source
%type    <enval>       marker
%type    <nval>        rule

%type    <psval>       element          
%type    <pival>       elementId          
%type    <vpsval>      elements         
%type    <vpival>      elementsIds         
%type    <vpsval>      markerElements         
%type    <vpival>      markerElementsIds         
%type    <vpnval>      parameters      
%type    <vpnval>      markers

%printer    { debug_stream() << *$$; } "string"

%destructor { /* delete string */
              if ($$) {
                delete $$;
              }
            }                           STRING EL_STRING FUNCTION

%destructor { /* delete node */
              if ($$) {
                delete $$;
              }
            }                           exp destination source marker rule

%destructor { /* delete pair */
              if ($$) {
                if ($$->first) {
                  delete $$->first;
                }
                if ($$->second) {
                  delete $$->second;
                }
                delete $$;
              }
            }                           element

%destructor { /* delete pair */
              if ($$) {
                delete $$;
              }
            }                           elementId

%destructor { /* delete vector of pairs */
              if ($$) {
                for (vector< pair<string*, string*> *>::iterator i = $$->begin(); i != $$->end(); i++) {
                  if ( *i ) {
                    if ((*i)->first) {
                      delete (*i)->first;
                    }
                    if ((*i)->second) {
                      delete (*i)->second;
                    }
                    delete (*i);
                  }
                }
                delete $$;
              }
            }                           elements

%destructor { /* delete vector of pairs */
              if ($$) {
                for (vector< pair<int, int> *>::iterator i = $$->begin(); i != $$->end(); i++) {
                  if ( *i ) {
                    delete (*i);
                  }
                }
                delete $$;
              }
            }                           elementsIds

%printer    { debug_stream() << $$; }  "integer" exp


%%
%start rule;


rule:
  destination ASSIGN exp END {
                                      driver.setResult(new RuleNode(RuleNode::NONE, $1, $3));
                                      $$ = 0;
                                    }
  | destination ASSIGN CONS_FLAG exp END {
                                      driver.setResult(new RuleNode(RuleNode::CONSOLIDATION, $1, $4));
                                      $$ = 0;
                                    }
  | destination ASSIGN BASE_FLAG exp END {
                                      driver.setResult(new RuleNode(RuleNode::BASE, $1, $4));
                                      $$ = 0;
                                    }
  | destination ASSIGN BASE_FLAG exp '@' markers END {
                                      driver.setResult(new RuleNode(RuleNode::BASE, $1, $4, $6));
                                      $$ = 0;
                                    }
  ;

%left GE LE EQ NE GT LT;
%left '+' '-';
%left '*' '/';
exp:
  source                            { $$ = $1; }
  | marker                          { $$ = $1; }
  | INTEGER                         { $$ = new DoubleNode($1 * 1.0); }
  | DOUBLE                          { $$ = new DoubleNode($1); }
  | '+' INTEGER                     { $$ = new DoubleNode($2 * 1.0); }
  | '+' DOUBLE                      { $$ = new DoubleNode($2); }
  | '-' INTEGER                     { $$ = new DoubleNode($2 * -1.0); }
  | '-' DOUBLE                      { $$ = new DoubleNode($2 * -1.0); }
  | STRING                          { $$ = new StringNode($1); }
  | VAR_STRING                      { $$ = new VariableNode($1); }
  | exp '+' exp                     {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back($1);
                                      v->push_back($3);
                                      $$ = driver.createFunction("+", v);
                                    }
  | exp '-' exp                     {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back($1);
                                      v->push_back($3);
                                      $$ = driver.createFunction("-", v);
                                    }
  | exp '*' exp                     {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back($1);
                                      v->push_back($3);
                                      $$ = driver.createFunction("*", v);
                                    }
  | exp '/' exp                     {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back($1);
                                      v->push_back($3);
                                      $$ = driver.createFunction("/", v);
                                    }
  | exp GE exp                      {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back($1);
                                      v->push_back($3);
                                      $$ = driver.createFunction(">=", v);
                                    }
  | exp LE exp                      {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back($1);
                                      v->push_back($3);
                                      $$ = driver.createFunction("<=", v);
                                    }
  | exp EQ exp                      {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back($1);
                                      v->push_back($3);
                                      $$ = driver.createFunction("==", v);
                                    }
  | exp NE exp                      {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back($1);
                                      v->push_back($3);
                                      $$ = driver.createFunction("!=", v);
                                    }
  | exp GT exp                      {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back($1);
                                      v->push_back($3);
                                      $$ = driver.createFunction(">", v);
                                    }
  | exp LT exp                      {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back($1);
                                      v->push_back($3);
                                      $$ = driver.createFunction("<", v);
                                    }
  | '(' exp ')'                     {
                                      $$ = $2;
                                    }
  | FUNCTION parameters ')'         {
                                      $$ = driver.createFunction(*($1), $2);
                                      delete $1;
                                    }
  ;

parameters:
  '(' exp                           {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back($2);
                                      $$ = v; 
                                    }
  | '('                             {
                                      vector<Node*> *v = new vector<Node*>;
                                      $$ = v; 
                                    }
  | parameters ',' exp              {
                                      $1->push_back($3);
                                      $$ = $1; 
                                    }
  ;

destination:
  elements ']'                      {
                                      $$ = new DestinationNode($1, 0);
                                    }
  | elementsIds '}'                 {
                                      $$ = new DestinationNode(0, $1);
                                    }
  ;

source:
  elements ']'                      {
                                      $$ = new SourceNode($1, 0);
                                    }
  | elementsIds '}'                 {
                                      $$ = new SourceNode(0, $1);
                                    }
  ;


markers:
  marker                                {
                                          vector<Node*> *v = new vector<Node*>;
                                          $$ = v;
                                          $$->push_back($1);
                                        }
  | FUNCTION parameters ')'             {
                                          Node* func = driver.createFunction(*($1), $2);
                                          delete $1;

                                          vector<Node*> *v = new vector<Node*>;
                                          $$ = v;
                                          $$->push_back(func);
                                        }
  | markers ',' marker                  {
                                          $$ = $1;
                                          $$->push_back($3);
                                        }
  | markers ',' FUNCTION parameters ')' {
                                          Node* func = driver.createFunction(*($3), $4);
                                          delete $3;

                                          $$ = $1;
                                          $$->push_back(func);
                                        }
  ;

marker:
  markerElements MARKER_CLOSE       {
                                      $$ = new SourceNode($1, 0, true);
                                    }
  | markerElementsIds SMARKER_CLOSE {
                                      $$ = new SourceNode(0, $1, true);
                                    }
  ;


elements:
  '[' element                       {
                                      vector<pair<string*, string*>*> *v = new vector<pair<string*, string*>*>;
                                      v->push_back($2);
                                      $$ = v;
                                    }
  | '['                             {
                                      vector<pair<string*, string*>*> *v = new vector<pair<string*, string*>*>;
                                      v->push_back(0);
                                      $$ = v;
                                    }
  | elements ',' element            {
                                      $1->push_back($3);
                                      $$ = $1;
                                    }
  | elements ','                    {
                                      $1->push_back(0);
                                      $$ = $1;
                                    }
  ;

markerElements:
  MARKER_OPEN element               {
                                      vector<pair<string*, string*>*> *v = new vector<pair<string*, string*>*>;
                                      v->push_back($2);
                                      $$ = v;
                                    }
  | MARKER_OPEN                     {
                                      vector<pair<string*, string*>*> *v = new vector<pair<string*, string*>*>;
                                      v->push_back(0);
                                      $$ = v;
                                    }
  | markerElements ',' element      {
                                      $1->push_back($3);
                                      $$ = $1;
                                    }
  | markerElements ','              {
                                      $1->push_back(0);
                                      $$ = $1;
                                    }
  ;

element:
  EL_STRING                         {
                                      pair<string*,string*> *p = new pair<string*, string*>;
                                      p->first = 0;
                                      p->second= $1;
                                      $$ = p;
                                    }
  | EL_STRING ':' EL_STRING         {
                                      pair<string*,string*> *p = new pair<string*, string*>;
                                      p->first = $1;
                                      p->second= $3;
                                      $$ = p;
                                    }
  | EL_STRING ':'                   {
                                      pair<string*,string*> *p = new pair<string*, string*>;
                                      p->first = $1;
                                      p->second= 0;
                                      $$ = p;
                                    }
  ;

elementsIds:
  '{' elementId                     {
                                      vector<pair<int, int>*> *v = new vector<pair<int, int>*>;
                                      v->push_back($2);
                                      $$ = v;
                                    }
  | '{'                             {
                                      vector<pair<int, int>*> *v = new vector<pair<int, int>*>;
                                      v->push_back(0);
                                      $$ = v;
                                    }
  | elementsIds ',' elementId       {
                                      $1->push_back($3);
                                      $$ = $1;
                                    }
  | elementsIds ','                 {
                                      $1->push_back(0);
                                      $$ = $1;
                                    }
  ;

markerElementsIds:
  SMARKER_OPEN elementId            {
                                      vector<pair<int, int>*> *v = new vector<pair<int, int>*>;
                                      v->push_back($2);
                                      $$ = v;
                                    }
  | SMARKER_OPEN                    {
                                      vector<pair<int, int>*> *v = new vector<pair<int, int>*>;
                                      v->push_back(0);
                                      $$ = v;
                                    }
  | markerElementsIds ',' elementId {
                                      $1->push_back($3);
                                      $$ = $1;
                                    }
  | markerElementsIds ','           {
                                      $1->push_back(0);
                                      $$ = $1;
                                    }
  ;

elementId:
  INTEGER ':' INTEGER               {
                                      pair<int,int> *p = new pair<int, int>;
                                      p->first = $1;
                                      p->second= $3;
                                      $$ = p;
                                    }
  | INTEGER '@' INTEGER             {
                                      pair<int,int> *p = new pair<int, int>;
                                      p->first = -(($1) + 1);
                                      p->second= $3;
                                      $$ = p;
                                    }
  | INTEGER ':'                     {
                                      pair<int,int> *p = new pair<int, int>;
                                      p->first = $1;
                                      p->second= -1;
                                      $$ = p;
                                    }
  ;


%%

void  yy::RuleParser::error (const yy::RuleParser::location_type& l, const string& m) {
  driver.error(l, m);
}
