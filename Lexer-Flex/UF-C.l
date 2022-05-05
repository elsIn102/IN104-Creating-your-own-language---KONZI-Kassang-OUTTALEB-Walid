%{
  #include "../Parser-Bison/UF-C.tab.h"

  int line_num = 1;
%}

%option noyywrap

%x COMMENTS


%%
\/\*            { BEGIN(COMMENTS); } // start of a comment: go to a 'COMMENTS' state.
<COMMENTS>\*\/  { BEGIN(INITIAL); }  // end of a comment: go back to normal parsing.
<COMMENTS>\n    { line_num++; }      // still have to increment line numbers inside of comments!
<COMMENTS>.     ;                    // ignore every other character while we are in this state

"is starting its training with" {return BEGIN_ARGS;}
"in order to increase its"  { return END_ARGS;}

"training is over" {return END_FUNC;}
[ \t] ;
end            { return END; }
"has fans in number of" {return FANS;}
"has an IQ of" { return IQ; }
"broken bones" {return BONES;}

"is thrown out" {return RETURN;}
"hits" {return ASSIGN;}
"bets on" {return BEGIN_CONDITION;}
"takes the rest of the bets" {return BEGIN_ELSE;}
"and" {return AND;}
"using" {return BEGIN_ARGS;}
"giving the money to" {return BEGIN_RETURN_VAR;}
"A new competition begins" {return BEGIN_TEST;}
"And the gambling den opens" {return BEGIN_BRANCH;} 
"Match" {return BEGIN_COMPARISON; }
":" {return COLON;}
"-" {return HYPHEN;}
"," {return COMMA;}



"challenges" {return TEST_GTR;}
"fights" {return TEST_STR_GTR;}
"beats down until" {return WHILE;}
"gives up" {return LOOP_NOTNULL;}
"someone splits them up" {return LOOP_DIFF;}
"they come to an agreement" {return LOOP_EQ;}
"the match is interrupted" {return BREAK;}
"end of the round" {return CONTINUE;}
"the ring girl shows" {return PRINT;}


"joins" {return ADD;}
"removes" {return MINUS;}
"tears apart" {return DIVIDE;}
"deals with" {return MULTIPLY;}

[-]*[0-9]+ {yylval.ival= atoi(yytext); return INT;} 
[-]*[0-9]+\.[0-9]+ { yylval.fval = atof(yytext); return FLOAT; }


[a-zA-Z0-9]+   {
  yylval.sval = strdup(yytext);
  return STRING;
}
\n             { ++line_num; return ENDL; }
.              ;
%%
