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
"weights"      {return KG;}
"broken bones" {return BONES;}

"is thrown out" {return RETURN;}
"hits" {return ASSIGN;}
"bets on" {return AND_CONDITIONS;}
"and" {return AND;}
"then supports" {return OR_CONDITIONS;}
"using" {return BEGIN_ARGS;}
"giving the money to" {return RETURN_VAR;}


"beats down until" {return WHILE;}
"gives up" {return EQUAL_ZERO;}
"until someone splits them up" {return DIFFERENT;}
"they come to an agreement" {return EQUAL;}
"the match is interrupted" {return BREAK;}
"end of the round" {return CONTINUE;}
"the ring girl shows" {return PRINT;}

[-]*[0-9]+\.[0-9]+ { yylval.fval = atof(yytext); return FLOAT; }
[-]*[0-9]+ {yylval.ival= atoi(yytext); return INT} 
[0-9]+ { yylval.ival = atoi(yytext); return UINT; }//on n'utilise pas les unsigned

[a-zA-Z0-9]+   {
  yylval.sval = strdup(yytext);
  return STRING;
}
\n             { ++line_num; return ENDL; }
.              ;
%%
