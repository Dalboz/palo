/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison LALR(1) parsers in C++

   Copyright (C) 2002, 2003, 2004, 2005, 2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


#include "RuleParser.h"

/* User implementation prologue.  */


# include "RuleParserDriver.h"


/* Line 317 of lalr1.cc.  */


#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* FIXME: INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#define YYUSE(e) ((void) (e))

/* A pseudo ostream that takes yydebug_ into account.  */
# define YYCDEBUG							\
  for (bool yydebugcond_ = yydebug_; yydebugcond_; yydebugcond_ = false)	\
    (*yycdebug_)

/* Enable debugging if requested.  */
#if YYDEBUG

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)	\
do {							\
  if (yydebug_)						\
    {							\
      *yycdebug_ << Title << ' ';			\
      yy_symbol_print_ ((Type), (Value), (Location));	\
      *yycdebug_ << std::endl;				\
    }							\
} while (false)

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug_)				\
    yy_reduce_print_ (Rule);		\
} while (false)

# define YY_STACK_PRINT()		\
do {					\
  if (yydebug_)				\
    yystack_print_ ();			\
} while (false)

#else /* !YYDEBUG */

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_REDUCE_PRINT(Rule)
# define YY_STACK_PRINT()

#endif /* !YYDEBUG */

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab

namespace yy
{
#if YYERROR_VERBOSE

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  RuleParser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr = "";
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              /* Fall through.  */
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }

#endif

  /// Build a parser object.
  RuleParser::RuleParser (RuleParserDriver& driver_yyarg)
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
      driver (driver_yyarg)
  {
  }

  RuleParser::~RuleParser ()
  {
  }

#if YYDEBUG
  /*--------------------------------.
  | Print this symbol on YYOUTPUT.  |
  `--------------------------------*/

  inline void
  RuleParser::yy_symbol_value_print_ (int yytype,
			   const semantic_type* yyvaluep, const location_type* yylocationp)
  {
    YYUSE (yylocationp);
    YYUSE (yyvaluep);
    switch (yytype)
      {
        case 16: /* "\"string\"" */

	{ debug_stream() << *(yyvaluep->sval); };

	break;
      case 20: /* "\"integer\"" */

	{ debug_stream() << (yyvaluep->ival); };

	break;
      case 37: /* "exp" */

	{ debug_stream() << (yyvaluep->enval); };

	break;
       default:
	  break;
      }
  }


  void
  RuleParser::yy_symbol_print_ (int yytype,
			   const semantic_type* yyvaluep, const location_type* yylocationp)
  {
    *yycdebug_ << (yytype < yyntokens_ ? "token" : "nterm")
	       << ' ' << yytname_[yytype] << " ("
	       << *yylocationp << ": ";
    yy_symbol_value_print_ (yytype, yyvaluep, yylocationp);
    *yycdebug_ << ')';
  }
#endif /* ! YYDEBUG */

  void
  RuleParser::yydestruct_ (const char* yymsg,
			   int yytype, semantic_type* yyvaluep, location_type* yylocationp)
  {
    YYUSE (yylocationp);
    YYUSE (yymsg);
    YYUSE (yyvaluep);

    YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

    switch (yytype)
      {
        case 16: /* "\"string\"" */

	{ /* delete string */
              if ((yyvaluep->sval)) {
                delete (yyvaluep->sval);
              }
            };

	break;
      case 17: /* "\"element_string\"" */

	{ /* delete string */
              if ((yyvaluep->sval)) {
                delete (yyvaluep->sval);
              }
            };

	break;
      case 19: /* "\"function\"" */

	{ /* delete string */
              if ((yyvaluep->sval)) {
                delete (yyvaluep->sval);
              }
            };

	break;
      case 36: /* "rule" */

	{ /* delete node */
              if ((yyvaluep->nval)) {
                delete (yyvaluep->nval);
              }
            };

	break;
      case 37: /* "exp" */

	{ /* delete node */
              if ((yyvaluep->enval)) {
                delete (yyvaluep->enval);
              }
            };

	break;
      case 39: /* "destination" */

	{ /* delete node */
              if ((yyvaluep->dnval)) {
                delete (yyvaluep->dnval);
              }
            };

	break;
      case 40: /* "source" */

	{ /* delete node */
              if ((yyvaluep->enval)) {
                delete (yyvaluep->enval);
              }
            };

	break;
      case 42: /* "marker" */

	{ /* delete node */
              if ((yyvaluep->enval)) {
                delete (yyvaluep->enval);
              }
            };

	break;
      case 43: /* "elements" */

	{ /* delete vector of pairs */
              if ((yyvaluep->vpsval)) {
                for (vector< pair<string*, string*> *>::iterator i = (yyvaluep->vpsval)->begin(); i != (yyvaluep->vpsval)->end(); i++) {
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
                delete (yyvaluep->vpsval);
              }
            };

	break;
      case 45: /* "element" */

	{ /* delete pair */
              if ((yyvaluep->psval)) {
                if ((yyvaluep->psval)->first) {
                  delete (yyvaluep->psval)->first;
                }
                if ((yyvaluep->psval)->second) {
                  delete (yyvaluep->psval)->second;
                }
                delete (yyvaluep->psval);
              }
            };

	break;
      case 46: /* "elementsIds" */

	{ /* delete vector of pairs */
              if ((yyvaluep->vpival)) {
                for (vector< pair<int, int> *>::iterator i = (yyvaluep->vpival)->begin(); i != (yyvaluep->vpival)->end(); i++) {
                  if ( *i ) {
                    delete (*i);
                  }
                }
                delete (yyvaluep->vpival);
              }
            };

	break;
      case 48: /* "elementId" */

	{ /* delete pair */
              if ((yyvaluep->pival)) {
                delete (yyvaluep->pival);
              }
            };

	break;

	default:
	  break;
      }
  }

  void
  RuleParser::yypop_ (unsigned int n)
  {
    yystate_stack_.pop (n);
    yysemantic_stack_.pop (n);
    yylocation_stack_.pop (n);
  }

  std::ostream&
  RuleParser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  RuleParser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  RuleParser::debug_level_type
  RuleParser::debug_level () const
  {
    return yydebug_;
  }

  void
  RuleParser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }


  int
  RuleParser::parse ()
  {
    /// Look-ahead and look-ahead in internal form.
    int yychar = yyempty_;
    int yytoken = 0;

    /* State.  */
    int yyn;
    int yylen = 0;
    int yystate = 0;

    /* Error handling.  */
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// Semantic value of the look-ahead.
    semantic_type yylval;
    /// Location of the look-ahead.
    location_type yylloc;
    /// The locations where the error started and ended.
    location yyerror_range[2];

    /// $$.
    semantic_type yyval;
    /// @$.
    location_type yyloc;

    int yyresult;

    YYCDEBUG << "Starting parse" << std::endl;


    /* User initialization code.  */
    
{
  // Initialize the initial location.
  yylloc.begin.filename = yylloc.end.filename = &driver.ruleString;
}
  /* Line 555 of yacc.c.  */

    /* Initialize the stacks.  The initial state will be pushed in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystate_stack_ = state_stack_type (0);
    yysemantic_stack_ = semantic_stack_type (0);
    yylocation_stack_ = location_stack_type (0);
    yysemantic_stack_.push (yylval);
    yylocation_stack_.push (yylloc);

    /* New state.  */
  yynewstate:
    yystate_stack_.push (yystate);
    YYCDEBUG << "Entering state " << yystate << std::endl;
    goto yybackup;

    /* Backup.  */
  yybackup:

    /* Try to take a decision without look-ahead.  */
    yyn = yypact_[yystate];
    if (yyn == yypact_ninf_)
      goto yydefault;

    /* Read a look-ahead token.  */
    if (yychar == yyempty_)
      {
	YYCDEBUG << "Reading a token: ";
	yychar = yylex (&yylval, &yylloc, driver);
      }


    /* Convert token to internal form.  */
    if (yychar <= yyeof_)
      {
	yychar = yytoken = yyeof_;
	YYCDEBUG << "Now at end of input." << std::endl;
      }
    else
      {
	yytoken = yytranslate_ (yychar);
	YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
      }

    /* If the proper action on seeing token YYTOKEN is to reduce or to
       detect an error, take that action.  */
    yyn += yytoken;
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yytoken)
      goto yydefault;

    /* Reduce or error.  */
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
	if (yyn == 0 || yyn == yytable_ninf_)
	goto yyerrlab;
	yyn = -yyn;
	goto yyreduce;
      }

    /* Accept?  */
    if (yyn == yyfinal_)
      goto yyacceptlab;

    /* Shift the look-ahead token.  */
    YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

    /* Discard the token being shifted unless it is eof.  */
    if (yychar != yyeof_)
      yychar = yyempty_;

    yysemantic_stack_.push (yylval);
    yylocation_stack_.push (yylloc);

    /* Count tokens shifted since error; after three, turn off error
       status.  */
    if (yyerrstatus_)
      --yyerrstatus_;

    yystate = yyn;
    goto yynewstate;

  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[yystate];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;

  /*-----------------------------.
  | yyreduce -- Do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    /* If YYLEN is nonzero, implement the default value of the action:
       `$$ = $1'.  Otherwise, use the top of the stack.

       Otherwise, the following line sets YYVAL to garbage.
       This behavior is undocumented and Bison
       users should not rely upon it.  */
    if (yylen)
      yyval = yysemantic_stack_[yylen - 1];
    else
      yyval = yysemantic_stack_[0];

    {
      slice<location_type, location_stack_type> slice (yylocation_stack_, yylen);
      YYLLOC_DEFAULT (yyloc, slice, yylen);
    }
    YY_REDUCE_PRINT (yyn);
    switch (yyn)
      {
	  case 2:

    {
                                      driver.setResult(new RuleNode(RuleNode::NONE, (yysemantic_stack_[(4) - (1)].dnval), (yysemantic_stack_[(4) - (3)].enval)));
                                      (yyval.nval) = 0;
                                    ;}
    break;

  case 3:

    {
                                      driver.setResult(new RuleNode(RuleNode::CONSOLIDATION, (yysemantic_stack_[(5) - (1)].dnval), (yysemantic_stack_[(5) - (4)].enval)));
                                      (yyval.nval) = 0;
                                    ;}
    break;

  case 4:

    {
                                      driver.setResult(new RuleNode(RuleNode::BASE, (yysemantic_stack_[(5) - (1)].dnval), (yysemantic_stack_[(5) - (4)].enval)));
                                      (yyval.nval) = 0;
                                    ;}
    break;

  case 5:

    {
                                      driver.setResult(new RuleNode(RuleNode::BASE, (yysemantic_stack_[(7) - (1)].dnval), (yysemantic_stack_[(7) - (4)].enval), (yysemantic_stack_[(7) - (6)].vpnval)));
                                      (yyval.nval) = 0;
                                    ;}
    break;

  case 6:

    { (yyval.enval) = (yysemantic_stack_[(1) - (1)].enval); ;}
    break;

  case 7:

    { (yyval.enval) = (yysemantic_stack_[(1) - (1)].enval); ;}
    break;

  case 8:

    { (yyval.enval) = new DoubleNode((yysemantic_stack_[(1) - (1)].ival) * 1.0); ;}
    break;

  case 9:

    { (yyval.enval) = new DoubleNode((yysemantic_stack_[(1) - (1)].dval)); ;}
    break;

  case 10:

    { (yyval.enval) = new DoubleNode((yysemantic_stack_[(2) - (2)].ival) * 1.0); ;}
    break;

  case 11:

    { (yyval.enval) = new DoubleNode((yysemantic_stack_[(2) - (2)].dval)); ;}
    break;

  case 12:

    { (yyval.enval) = new DoubleNode((yysemantic_stack_[(2) - (2)].ival) * -1.0); ;}
    break;

  case 13:

    { (yyval.enval) = new DoubleNode((yysemantic_stack_[(2) - (2)].dval) * -1.0); ;}
    break;

  case 14:

    { (yyval.enval) = new StringNode((yysemantic_stack_[(1) - (1)].sval)); ;}
    break;

  case 15:

    { (yyval.enval) = new VariableNode((yysemantic_stack_[(1) - (1)].sval)); ;}
    break;

  case 16:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back((yysemantic_stack_[(3) - (1)].enval));
                                      v->push_back((yysemantic_stack_[(3) - (3)].enval));
                                      (yyval.enval) = driver.createFunction("+", v);
                                    ;}
    break;

  case 17:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back((yysemantic_stack_[(3) - (1)].enval));
                                      v->push_back((yysemantic_stack_[(3) - (3)].enval));
                                      (yyval.enval) = driver.createFunction("-", v);
                                    ;}
    break;

  case 18:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back((yysemantic_stack_[(3) - (1)].enval));
                                      v->push_back((yysemantic_stack_[(3) - (3)].enval));
                                      (yyval.enval) = driver.createFunction("*", v);
                                    ;}
    break;

  case 19:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back((yysemantic_stack_[(3) - (1)].enval));
                                      v->push_back((yysemantic_stack_[(3) - (3)].enval));
                                      (yyval.enval) = driver.createFunction("/", v);
                                    ;}
    break;

  case 20:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back((yysemantic_stack_[(3) - (1)].enval));
                                      v->push_back((yysemantic_stack_[(3) - (3)].enval));
                                      (yyval.enval) = driver.createFunction(">=", v);
                                    ;}
    break;

  case 21:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back((yysemantic_stack_[(3) - (1)].enval));
                                      v->push_back((yysemantic_stack_[(3) - (3)].enval));
                                      (yyval.enval) = driver.createFunction("<=", v);
                                    ;}
    break;

  case 22:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back((yysemantic_stack_[(3) - (1)].enval));
                                      v->push_back((yysemantic_stack_[(3) - (3)].enval));
                                      (yyval.enval) = driver.createFunction("==", v);
                                    ;}
    break;

  case 23:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back((yysemantic_stack_[(3) - (1)].enval));
                                      v->push_back((yysemantic_stack_[(3) - (3)].enval));
                                      (yyval.enval) = driver.createFunction("!=", v);
                                    ;}
    break;

  case 24:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back((yysemantic_stack_[(3) - (1)].enval));
                                      v->push_back((yysemantic_stack_[(3) - (3)].enval));
                                      (yyval.enval) = driver.createFunction(">", v);
                                    ;}
    break;

  case 25:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back((yysemantic_stack_[(3) - (1)].enval));
                                      v->push_back((yysemantic_stack_[(3) - (3)].enval));
                                      (yyval.enval) = driver.createFunction("<", v);
                                    ;}
    break;

  case 26:

    {
                                      (yyval.enval) = (yysemantic_stack_[(3) - (2)].enval);
                                    ;}
    break;

  case 27:

    {
                                      (yyval.enval) = driver.createFunction(*((yysemantic_stack_[(3) - (1)].sval)), (yysemantic_stack_[(3) - (2)].vpnval));
                                      delete (yysemantic_stack_[(3) - (1)].sval);
                                    ;}
    break;

  case 28:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      v->push_back((yysemantic_stack_[(2) - (2)].enval));
                                      (yyval.vpnval) = v; 
                                    ;}
    break;

  case 29:

    {
                                      vector<Node*> *v = new vector<Node*>;
                                      (yyval.vpnval) = v; 
                                    ;}
    break;

  case 30:

    {
                                      (yysemantic_stack_[(3) - (1)].vpnval)->push_back((yysemantic_stack_[(3) - (3)].enval));
                                      (yyval.vpnval) = (yysemantic_stack_[(3) - (1)].vpnval); 
                                    ;}
    break;

  case 31:

    {
                                      (yyval.dnval) = new DestinationNode((yysemantic_stack_[(2) - (1)].vpsval), 0);
                                    ;}
    break;

  case 32:

    {
                                      (yyval.dnval) = new DestinationNode(0, (yysemantic_stack_[(2) - (1)].vpival));
                                    ;}
    break;

  case 33:

    {
                                      (yyval.enval) = new SourceNode((yysemantic_stack_[(2) - (1)].vpsval), 0);
                                    ;}
    break;

  case 34:

    {
                                      (yyval.enval) = new SourceNode(0, (yysemantic_stack_[(2) - (1)].vpival));
                                    ;}
    break;

  case 35:

    {
                                          vector<Node*> *v = new vector<Node*>;
                                          (yyval.vpnval) = v;
                                          (yyval.vpnval)->push_back((yysemantic_stack_[(1) - (1)].enval));
                                        ;}
    break;

  case 36:

    {
                                          Node* func = driver.createFunction(*((yysemantic_stack_[(3) - (1)].sval)), (yysemantic_stack_[(3) - (2)].vpnval));
                                          delete (yysemantic_stack_[(3) - (1)].sval);

                                          vector<Node*> *v = new vector<Node*>;
                                          (yyval.vpnval) = v;
                                          (yyval.vpnval)->push_back(func);
                                        ;}
    break;

  case 37:

    {
                                          (yyval.vpnval) = (yysemantic_stack_[(3) - (1)].vpnval);
                                          (yyval.vpnval)->push_back((yysemantic_stack_[(3) - (3)].enval));
                                        ;}
    break;

  case 38:

    {
                                          Node* func = driver.createFunction(*((yysemantic_stack_[(5) - (3)].sval)), (yysemantic_stack_[(5) - (4)].vpnval));
                                          delete (yysemantic_stack_[(5) - (3)].sval);

                                          (yyval.vpnval) = (yysemantic_stack_[(5) - (1)].vpnval);
                                          (yyval.vpnval)->push_back(func);
                                        ;}
    break;

  case 39:

    {
                                      (yyval.enval) = new SourceNode((yysemantic_stack_[(2) - (1)].vpsval), 0, true);
                                    ;}
    break;

  case 40:

    {
                                      (yyval.enval) = new SourceNode(0, (yysemantic_stack_[(2) - (1)].vpival), true);
                                    ;}
    break;

  case 41:

    {
                                      vector<pair<string*, string*>*> *v = new vector<pair<string*, string*>*>;
                                      v->push_back((yysemantic_stack_[(2) - (2)].psval));
                                      (yyval.vpsval) = v;
                                    ;}
    break;

  case 42:

    {
                                      vector<pair<string*, string*>*> *v = new vector<pair<string*, string*>*>;
                                      v->push_back(0);
                                      (yyval.vpsval) = v;
                                    ;}
    break;

  case 43:

    {
                                      (yysemantic_stack_[(3) - (1)].vpsval)->push_back((yysemantic_stack_[(3) - (3)].psval));
                                      (yyval.vpsval) = (yysemantic_stack_[(3) - (1)].vpsval);
                                    ;}
    break;

  case 44:

    {
                                      (yysemantic_stack_[(2) - (1)].vpsval)->push_back(0);
                                      (yyval.vpsval) = (yysemantic_stack_[(2) - (1)].vpsval);
                                    ;}
    break;

  case 45:

    {
                                      vector<pair<string*, string*>*> *v = new vector<pair<string*, string*>*>;
                                      v->push_back((yysemantic_stack_[(2) - (2)].psval));
                                      (yyval.vpsval) = v;
                                    ;}
    break;

  case 46:

    {
                                      vector<pair<string*, string*>*> *v = new vector<pair<string*, string*>*>;
                                      v->push_back(0);
                                      (yyval.vpsval) = v;
                                    ;}
    break;

  case 47:

    {
                                      (yysemantic_stack_[(3) - (1)].vpsval)->push_back((yysemantic_stack_[(3) - (3)].psval));
                                      (yyval.vpsval) = (yysemantic_stack_[(3) - (1)].vpsval);
                                    ;}
    break;

  case 48:

    {
                                      (yysemantic_stack_[(2) - (1)].vpsval)->push_back(0);
                                      (yyval.vpsval) = (yysemantic_stack_[(2) - (1)].vpsval);
                                    ;}
    break;

  case 49:

    {
                                      pair<string*,string*> *p = new pair<string*, string*>;
                                      p->first = 0;
                                      p->second= (yysemantic_stack_[(1) - (1)].sval);
                                      (yyval.psval) = p;
                                    ;}
    break;

  case 50:

    {
                                      pair<string*,string*> *p = new pair<string*, string*>;
                                      p->first = (yysemantic_stack_[(3) - (1)].sval);
                                      p->second= (yysemantic_stack_[(3) - (3)].sval);
                                      (yyval.psval) = p;
                                    ;}
    break;

  case 51:

    {
                                      pair<string*,string*> *p = new pair<string*, string*>;
                                      p->first = (yysemantic_stack_[(2) - (1)].sval);
                                      p->second= 0;
                                      (yyval.psval) = p;
                                    ;}
    break;

  case 52:

    {
                                      vector<pair<int, int>*> *v = new vector<pair<int, int>*>;
                                      v->push_back((yysemantic_stack_[(2) - (2)].pival));
                                      (yyval.vpival) = v;
                                    ;}
    break;

  case 53:

    {
                                      vector<pair<int, int>*> *v = new vector<pair<int, int>*>;
                                      v->push_back(0);
                                      (yyval.vpival) = v;
                                    ;}
    break;

  case 54:

    {
                                      (yysemantic_stack_[(3) - (1)].vpival)->push_back((yysemantic_stack_[(3) - (3)].pival));
                                      (yyval.vpival) = (yysemantic_stack_[(3) - (1)].vpival);
                                    ;}
    break;

  case 55:

    {
                                      (yysemantic_stack_[(2) - (1)].vpival)->push_back(0);
                                      (yyval.vpival) = (yysemantic_stack_[(2) - (1)].vpival);
                                    ;}
    break;

  case 56:

    {
                                      vector<pair<int, int>*> *v = new vector<pair<int, int>*>;
                                      v->push_back((yysemantic_stack_[(2) - (2)].pival));
                                      (yyval.vpival) = v;
                                    ;}
    break;

  case 57:

    {
                                      vector<pair<int, int>*> *v = new vector<pair<int, int>*>;
                                      v->push_back(0);
                                      (yyval.vpival) = v;
                                    ;}
    break;

  case 58:

    {
                                      (yysemantic_stack_[(3) - (1)].vpival)->push_back((yysemantic_stack_[(3) - (3)].pival));
                                      (yyval.vpival) = (yysemantic_stack_[(3) - (1)].vpival);
                                    ;}
    break;

  case 59:

    {
                                      (yysemantic_stack_[(2) - (1)].vpival)->push_back(0);
                                      (yyval.vpival) = (yysemantic_stack_[(2) - (1)].vpival);
                                    ;}
    break;

  case 60:

    {
                                      pair<int,int> *p = new pair<int, int>;
                                      p->first = (yysemantic_stack_[(3) - (1)].ival);
                                      p->second= (yysemantic_stack_[(3) - (3)].ival);
                                      (yyval.pival) = p;
                                    ;}
    break;

  case 61:

    {
                                      pair<int,int> *p = new pair<int, int>;
                                      p->first = -(((yysemantic_stack_[(3) - (1)].ival)) + 1);
                                      p->second= (yysemantic_stack_[(3) - (3)].ival);
                                      (yyval.pival) = p;
                                    ;}
    break;

  case 62:

    {
                                      pair<int,int> *p = new pair<int, int>;
                                      p->first = (yysemantic_stack_[(2) - (1)].ival);
                                      p->second= -1;
                                      (yyval.pival) = p;
                                    ;}
    break;


    /* Line 675 of lalr1.cc.  */

	default: break;
      }
    YY_SYMBOL_PRINT ("-> $$ =", yyr1_[yyn], &yyval, &yyloc);

    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();

    yysemantic_stack_.push (yyval);
    yylocation_stack_.push (yyloc);

    /* Shift the result of the reduction.  */
    yyn = yyr1_[yyn];
    yystate = yypgoto_[yyn - yyntokens_] + yystate_stack_[0];
    if (0 <= yystate && yystate <= yylast_
	&& yycheck_[yystate] == yystate_stack_[0])
      yystate = yytable_[yystate];
    else
      yystate = yydefgoto_[yyn - yyntokens_];
    goto yynewstate;

  /*------------------------------------.
  | yyerrlab -- here on detecting error |
  `------------------------------------*/
  yyerrlab:
    /* If not already recovering from an error, report this error.  */
    if (!yyerrstatus_)
      {
	++yynerrs_;
	error (yylloc, yysyntax_error_ (yystate, yytoken));
      }

    yyerror_range[0] = yylloc;
    if (yyerrstatus_ == 3)
      {
	/* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

	if (yychar <= yyeof_)
	  {
	  /* Return failure if at end of input.  */
	  if (yychar == yyeof_)
	    YYABORT;
	  }
	else
	  {
	    yydestruct_ ("Error: discarding", yytoken, &yylval, &yylloc);
	    yychar = yyempty_;
	  }
      }

    /* Else will try to reuse look-ahead token after shifting the error
       token.  */
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:

    /* Pacify compilers like GCC when the user code never invokes
       YYERROR and the label yyerrorlab therefore never appears in user
       code.  */
    if (false)
      goto yyerrorlab;

    yyerror_range[0] = yylocation_stack_[yylen - 1];
    /* Do not reclaim the symbols of the rule which action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    yystate = yystate_stack_[0];
    goto yyerrlab1;

  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;	/* Each real token shifted decrements this.  */

    for (;;)
      {
	yyn = yypact_[yystate];
	if (yyn != yypact_ninf_)
	{
	  yyn += yyterror_;
	  if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == yyterror_)
	    {
	      yyn = yytable_[yyn];
	      if (0 < yyn)
		break;
	    }
	}

	/* Pop the current state because it cannot handle the error token.  */
	if (yystate_stack_.height () == 1)
	YYABORT;

	yyerror_range[0] = yylocation_stack_[0];
	yydestruct_ ("Error: popping",
		     yystos_[yystate],
		     &yysemantic_stack_[0], &yylocation_stack_[0]);
	yypop_ ();
	yystate = yystate_stack_[0];
	YY_STACK_PRINT ();
      }

    if (yyn == yyfinal_)
      goto yyacceptlab;

    yyerror_range[1] = yylloc;
    // Using YYLLOC is tempting, but would change the location of
    // the look-ahead.  YYLOC is available though.
    YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
    yysemantic_stack_.push (yylval);
    yylocation_stack_.push (yyloc);

    /* Shift the error token.  */
    YY_SYMBOL_PRINT ("Shifting", yystos_[yyn],
		   &yysemantic_stack_[0], &yylocation_stack_[0]);

    yystate = yyn;
    goto yynewstate;

    /* Accept.  */
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;

    /* Abort.  */
  yyabortlab:
    yyresult = 1;
    goto yyreturn;

  yyreturn:
    if (yychar != yyeof_ && yychar != yyempty_)
      yydestruct_ ("Cleanup: discarding lookahead", yytoken, &yylval, &yylloc);

    /* Do not reclaim the symbols of the rule which action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (yystate_stack_.height () != 1)
      {
	yydestruct_ ("Cleanup: popping",
		   yystos_[yystate_stack_[0]],
		   &yysemantic_stack_[0],
		   &yylocation_stack_[0]);
	yypop_ ();
      }

    return yyresult;
  }

  // Generate an error message.
  std::string
  RuleParser::yysyntax_error_ (int yystate, int tok)
  {
    std::string res;
    YYUSE (yystate);
#if YYERROR_VERBOSE
    int yyn = yypact_[yystate];
    if (yypact_ninf_ < yyn && yyn <= yylast_)
      {
	/* Start YYX at -YYN if negative to avoid negative indexes in
	   YYCHECK.  */
	int yyxbegin = yyn < 0 ? -yyn : 0;

	/* Stay within bounds of both yycheck and yytname.  */
	int yychecklim = yylast_ - yyn + 1;
	int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
	int count = 0;
	for (int x = yyxbegin; x < yyxend; ++x)
	  if (yycheck_[x + yyn] == x && x != yyterror_)
	    ++count;

	// FIXME: This method of building the message is not compatible
	// with internationalization.  It should work like yacc.c does it.
	// That is, first build a string that looks like this:
	// "syntax error, unexpected %s or %s or %s"
	// Then, invoke YY_ on this string.
	// Finally, use the string as a format to output
	// yytname_[tok], etc.
	// Until this gets fixed, this message appears in English only.
	res = "syntax error, unexpected ";
	res += yytnamerr_ (yytname_[tok]);
	if (count < 5)
	  {
	    count = 0;
	    for (int x = yyxbegin; x < yyxend; ++x)
	      if (yycheck_[x + yyn] == x && x != yyterror_)
		{
		  res += (!count++) ? ", expecting " : " or ";
		  res += yytnamerr_ (yytname_[x]);
		}
	  }
      }
    else
#endif
      res = YY_("syntax error");
    return res;
  }


  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
  const signed char RuleParser::yypact_ninf_ = -83;
  const short int
  RuleParser::yypact_[] =
  {
        -1,    10,    35,    20,    58,   -14,    -5,    32,   -83,   -15,
     -83,   -83,    48,    10,   -83,    35,   -83,    46,    56,    61,
     134,   134,    10,    35,   -83,   -83,    43,   -83,   -83,    14,
      30,   134,    98,   -83,   -83,    44,    -8,    25,    -7,   -83,
     -83,   -83,   -83,   -83,   119,    77,   -83,   -83,   134,    50,
     -83,   -83,   -83,   -83,   163,   -83,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   -83,   -83,    10,   -83,
     -83,    35,   -83,   -83,     0,   169,   -83,   134,   -83,    23,
      23,    23,    23,    23,    23,    64,    64,   -83,   -83,   -83,
     -83,    43,     3,   -83,   169,    63,   -83,    11,   -83,    43,
     -83,    65,   -83
  };

  /* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
     doesn't specify something else to do.  Zero means the default is an
     error.  */
  const unsigned char
  RuleParser::yydefact_[] =
  {
         0,    42,    53,     0,     0,     0,     0,    49,    41,     0,
      52,     1,     0,    44,    31,    55,    32,    51,     0,    62,
       0,     0,    46,    57,    14,    15,     0,     8,     9,     0,
       0,     0,     0,     6,     7,     0,     0,     0,     0,    43,
      54,    50,    61,    60,     0,     0,    45,    56,    29,     0,
      10,    11,    12,    13,     0,     2,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    33,    39,    48,    34,
      40,    59,     3,     4,     0,    28,    27,     0,    26,    20,
      21,    22,    23,    24,    25,    16,    17,    18,    19,    47,
      58,     0,     0,    35,    30,     0,     5,     0,    36,     0,
      37,     0,    38
  };

  /* YYPGOTO[NTERM-NUM].  */
  const signed char
  RuleParser::yypgoto_[] =
  {
       -83,   -83,   -20,   -82,   -83,   -83,   -83,   -68,    95,   -83,
      -9,    96,   -83,   -13
  };

  /* YYDEFGOTO[NTERM-NUM].  */
  const signed char
  RuleParser::yydefgoto_[] =
  {
        -1,     3,    32,    49,     4,    33,    92,    34,    35,    36,
       8,    37,    38,    10
  };

  /* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule which
     number is the opposite.  If zero, do what YYDEFACT says.  */
  const signed char RuleParser::yytable_ninf_ = -1;
  const unsigned char
  RuleParser::yytable_[] =
  {
        44,    45,    40,    96,    39,    67,    93,    18,    70,    95,
      47,    54,    22,    46,    23,    13,    14,   101,    19,    91,
      11,    68,    71,    22,    15,    23,    16,     7,    75,   100,
      99,     1,    97,     2,    50,    51,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    62,    63,    64,    65,
      52,    53,    20,    21,    15,     9,    69,    94,    90,    89,
      22,    12,    23,    41,    24,    17,    25,    26,    27,    28,
      48,    29,    30,    13,    66,    31,    42,    73,    76,    77,
       1,    43,     2,    56,    57,    58,    59,    60,    61,    64,
      65,    98,    77,   102,    77,     5,     6,     0,    55,    74,
      62,    63,    64,    65,    56,    57,    58,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    72,
       0,    62,    63,    64,    65,    56,    57,    58,    59,    60,
      61,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,    63,    64,    65,    22,     0,    23,     0,
      24,     0,    25,    26,    27,    28,     0,    29,    30,     0,
       0,    31,     0,     0,     0,     0,     1,     0,     2,    56,
      57,    58,    59,    60,    61,    56,    57,    58,    59,    60,
      61,     0,     0,     0,     0,     0,    62,    63,    64,    65,
       0,    78,    62,    63,    64,    65
  };

  /* YYCHECK.  */
  const signed char
  RuleParser::yycheck_[] =
  {
        20,    21,    15,     0,    13,    13,    74,    22,    15,    91,
      23,    31,    12,    22,    14,    29,    30,    99,    33,    19,
       0,    29,    29,    12,    29,    14,    31,    17,    48,    97,
      19,    32,    29,    34,    20,    21,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    23,    24,    25,    26,
      20,    21,     4,     5,    29,    20,    31,    77,    71,    68,
      12,     3,    14,    17,    16,    33,    18,    19,    20,    21,
      27,    23,    24,    29,    30,    27,    20,     0,    28,    29,
      32,    20,    34,     6,     7,     8,     9,    10,    11,    25,
      26,    28,    29,    28,    29,     0,     0,    -1,     0,    22,
      23,    24,    25,    26,     6,     7,     8,     9,    10,    11,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     0,
      -1,    23,    24,    25,    26,     6,     7,     8,     9,    10,
      11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    23,    24,    25,    26,    12,    -1,    14,    -1,
      16,    -1,    18,    19,    20,    21,    -1,    23,    24,    -1,
      -1,    27,    -1,    -1,    -1,    -1,    32,    -1,    34,     6,
       7,     8,     9,    10,    11,     6,     7,     8,     9,    10,
      11,    -1,    -1,    -1,    -1,    -1,    23,    24,    25,    26,
      -1,    28,    23,    24,    25,    26
  };

  /* STOS_[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
  const unsigned char
  RuleParser::yystos_[] =
  {
         0,    32,    34,    36,    39,    43,    46,    17,    45,    20,
      48,     0,     3,    29,    30,    29,    31,    33,    22,    33,
       4,     5,    12,    14,    16,    18,    19,    20,    21,    23,
      24,    27,    37,    40,    42,    43,    44,    46,    47,    45,
      48,    17,    20,    20,    37,    37,    45,    48,    27,    38,
      20,    21,    20,    21,    37,     0,     6,     7,     8,     9,
      10,    11,    23,    24,    25,    26,    30,    13,    29,    31,
      15,    29,     0,     0,    22,    37,    28,    29,    28,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    45,
      48,    19,    41,    42,    37,    38,     0,    29,    28,    19,
      42,    38,    28
  };

#if YYDEBUG
  /* TOKEN_NUMBER_[YYLEX-NUM] -- Internal symbol number corresponding
     to YYLEX-NUM.  */
  const unsigned short int
  RuleParser::yytoken_number_[] =
  {
         0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,    64,    43,    45,    42,    47,    40,    41,    44,
      93,   125,    91,    58,   123
  };
#endif

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
  const unsigned char
  RuleParser::yyr1_[] =
  {
         0,    35,    36,    36,    36,    36,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    38,    38,
      38,    39,    39,    40,    40,    41,    41,    41,    41,    42,
      42,    43,    43,    43,    43,    44,    44,    44,    44,    45,
      45,    45,    46,    46,    46,    46,    47,    47,    47,    47,
      48,    48,    48
  };

  /* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
  const unsigned char
  RuleParser::yyr2_[] =
  {
         0,     2,     4,     5,     5,     7,     1,     1,     1,     1,
       2,     2,     2,     2,     1,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     1,
       3,     2,     2,     2,     2,     1,     3,     3,     5,     2,
       2,     2,     1,     3,     2,     2,     1,     3,     2,     1,
       3,     2,     2,     1,     3,     2,     2,     1,     3,     2,
       3,     3,     2
  };

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
  /* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
     First, the terminals, then, starting at \a yyntokens_, nonterminals.  */
  const char*
  const RuleParser::yytname_[] =
  {
    "\"end of file\"", "error", "$undefined", "\"=\"", "CONS_FLAG",
  "BASE_FLAG", "GE", "LE", "EQ", "NE", "GT", "LT", "MARKER_OPEN",
  "MARKER_CLOSE", "SMARKER_OPEN", "SMARKER_CLOSE", "\"string\"",
  "\"element_string\"", "\"variable_string\"", "\"function\"",
  "\"integer\"", "\"double\"", "'@'", "'+'", "'-'", "'*'", "'/'", "'('",
  "')'", "','", "']'", "'}'", "'['", "':'", "'{'", "$accept", "rule",
  "exp", "parameters", "destination", "source", "markers", "marker",
  "elements", "markerElements", "element", "elementsIds",
  "markerElementsIds", "elementId", 0
  };
#endif

#if YYDEBUG
  /* YYRHS -- A `-1'-separated list of the rules' RHS.  */
  const RuleParser::rhs_number_type
  RuleParser::yyrhs_[] =
  {
        36,     0,    -1,    39,     3,    37,     0,    -1,    39,     3,
       4,    37,     0,    -1,    39,     3,     5,    37,     0,    -1,
      39,     3,     5,    37,    22,    41,     0,    -1,    40,    -1,
      42,    -1,    20,    -1,    21,    -1,    23,    20,    -1,    23,
      21,    -1,    24,    20,    -1,    24,    21,    -1,    16,    -1,
      18,    -1,    37,    23,    37,    -1,    37,    24,    37,    -1,
      37,    25,    37,    -1,    37,    26,    37,    -1,    37,     6,
      37,    -1,    37,     7,    37,    -1,    37,     8,    37,    -1,
      37,     9,    37,    -1,    37,    10,    37,    -1,    37,    11,
      37,    -1,    27,    37,    28,    -1,    19,    38,    28,    -1,
      27,    37,    -1,    27,    -1,    38,    29,    37,    -1,    43,
      30,    -1,    46,    31,    -1,    43,    30,    -1,    46,    31,
      -1,    42,    -1,    19,    38,    28,    -1,    41,    29,    42,
      -1,    41,    29,    19,    38,    28,    -1,    44,    13,    -1,
      47,    15,    -1,    32,    45,    -1,    32,    -1,    43,    29,
      45,    -1,    43,    29,    -1,    12,    45,    -1,    12,    -1,
      44,    29,    45,    -1,    44,    29,    -1,    17,    -1,    17,
      33,    17,    -1,    17,    33,    -1,    34,    48,    -1,    34,
      -1,    46,    29,    48,    -1,    46,    29,    -1,    14,    48,
      -1,    14,    -1,    47,    29,    48,    -1,    47,    29,    -1,
      20,    33,    20,    -1,    20,    22,    20,    -1,    20,    33,
      -1
  };

  /* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
     YYRHS.  */
  const unsigned char
  RuleParser::yyprhs_[] =
  {
         0,     0,     3,     8,    14,    20,    28,    30,    32,    34,
      36,    39,    42,    45,    48,    50,    52,    56,    60,    64,
      68,    72,    76,    80,    84,    88,    92,    96,   100,   103,
     105,   109,   112,   115,   118,   121,   123,   127,   131,   137,
     140,   143,   146,   148,   152,   155,   158,   160,   164,   167,
     169,   173,   176,   179,   181,   185,   188,   191,   193,   197,
     200,   204,   208
  };

  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
  const unsigned short int
  RuleParser::yyrline_[] =
  {
         0,   157,   157,   161,   165,   169,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,   195,   201,   207,
     213,   219,   225,   231,   237,   243,   249,   252,   259,   264,
     268,   275,   278,   284,   287,   294,   299,   307,   311,   321,
     324,   331,   336,   341,   345,   352,   357,   362,   366,   373,
     379,   385,   394,   399,   404,   408,   415,   420,   425,   429,
     436,   442,   448
  };

  // Print the state stack on the debug stream.
  void
  RuleParser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (state_stack_type::const_iterator i = yystate_stack_.begin ();
	 i != yystate_stack_.end (); ++i)
      *yycdebug_ << ' ' << *i;
    *yycdebug_ << std::endl;
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  RuleParser::yy_reduce_print_ (int yyrule)
  {
    unsigned int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    /* Print the symbols being reduced, and their result.  */
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
	       << " (line " << yylno << "), ";
    /* The symbols being reduced.  */
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
		       yyrhs_[yyprhs_[yyrule] + yyi],
		       &(yysemantic_stack_[(yynrhs) - (yyi + 1)]),
		       &(yylocation_stack_[(yynrhs) - (yyi + 1)]));
  }
#endif // YYDEBUG

  /* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
  RuleParser::token_number_type
  RuleParser::yytranslate_ (int t)
  {
    static
    const token_number_type
    translate_table[] =
    {
           0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      27,    28,    25,    23,    29,    24,     2,    26,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    33,     2,
       2,     2,     2,     2,    22,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    32,     2,    30,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    34,     2,    31,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21
    };
    if ((unsigned int) t <= yyuser_token_number_max_)
      return translate_table[t];
    else
      return yyundef_token_;
  }

  const int RuleParser::yyeof_ = 0;
  const int RuleParser::yylast_ = 195;
  const int RuleParser::yynnts_ = 14;
  const int RuleParser::yyempty_ = -2;
  const int RuleParser::yyfinal_ = 11;
  const int RuleParser::yyterror_ = 1;
  const int RuleParser::yyerrcode_ = 256;
  const int RuleParser::yyntokens_ = 35;

  const unsigned int RuleParser::yyuser_token_number_max_ = 276;
  const RuleParser::token_number_type RuleParser::yyundef_token_ = 2;

} // namespace yy




void  yy::RuleParser::error (const yy::RuleParser::location_type& l, const string& m) {
  driver.error(l, m);
}

