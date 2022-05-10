%{
  #include "../Parser-Bison/UF-C.tab.h"

  int line_num = 1;
  int stringLength = 0;
%}

%option noyywrap

%x COMMENTS
%x READING_STRING


%%
\/\*            { BEGIN(COMMENTS); } // start of a comment: go to a 'COMMENTS' state.
<COMMENTS>\*\/  { BEGIN(INITIAL); }  // end of a comment: go back to normal parsing.
<COMMENTS>\n    { line_num++; }      // still have to increment line numbers inside of comments!
<COMMENTS>.     ;                    // ignore every other character while we are in this state

\" { BEGIN(READING_STRING); stringLength = 0;}
<READING_STRING>\" { BEGIN(INITIAL); stringLength++; }
<READING_STRING>\n { ++line_num; stringLength++;}
<READING_STRING>[^\"]* { yylval.sval = strdup(yytext); stringLength++; return STRING_CONSTANT; }

"is starting its training with" {return FUNC_DEF_BEGIN_ARGS;}
"to increase its"  { return FUNC_DEF_END_ARGS;}

"training is over" {return END_FUNC;}
[ \t] ;
end            { return END; }


"has this number of fans :" {return FANS;} //declaration of variables: int
"has an IQ of" { return IQ; }  //float
"broken bones" {return BONES;} //char
"announces" {return ANNOUNCES;}  //declare a string

"IQ" {return TYPE_FLOAT;}
"fans" {return TYPE_INT;}
"size" {return TYPE_STRING;}
"effectiveness" {return TYPE_VOID;}

"is thrown out" {return RETURN;}
"hits" {return ASSIGN;}
"bets on" {return BEGIN_CONDITION;}
"takes the rest of the bets" {return BEGIN_ELSE;}
"and" {return AND;}
"using" {return BEGIN_ARGS;}
"with" {return BEGIN_ARGS;}
"enrolls" {return BEGIN_ARGS;}
"and gives the money to" {return BEGIN_RETURN_VAR;}
"A new competition begins" {return BEGIN_TEST;}
"And the gambling den opens" {return BEGIN_BRANCH;} 
"Match" {return BEGIN_COMPARISON; }
":" {return COLON;}
"-" {return HYPHEN;}
"," {return COMMA;}



"challenges" {return TEST_GTR;}
"fights" {return TEST_STR_GTR;}

"beats down" {return WHILE;}
"until they give up" {return LOOP_NOTNULL;}
"until someone splits them up" {return LOOP_EQ;}
"until they fight back" {return LOOP_GTR;}
"they come to an agreement" {return LOOP_NOT_EQ;}
"meanwhile" {return LOOP_BEGIN_ACTION;}

"the match is interrupted" {return BREAK;}
"end of the round" {return CONTINUE;}

"the ring girl shows" {return PRINT;}
"the fans of" {return PRINT_INT;}
"the wits of" {return PRINT_FLOAT;}
"the flow of" {return PRINT_STRING;}


"joins" {return ADD;}
"removes" {return MINUS;}
"tears apart" {return DIVIDE;}
"deals with" {return MULTIPLY;}

[-]?[0-9]+ {yylval.ival= atoi(yytext); return INT;} 
[-]?[0-9]+\.[0-9]+ { yylval.fval = atof(yytext); return FLOAT; }


[a-zA-Z0-9]+   {
  yylval.sval = strdup(yytext);
  return STRING;
}
\n             { ++line_num; return ENDL; }
.              ;
%%
