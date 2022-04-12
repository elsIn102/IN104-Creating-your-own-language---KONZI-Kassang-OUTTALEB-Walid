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

[ \t] ;
sNaZZle        { return SNAZZLE; }
type           { return TYPE; }
end            { return END; }
[0-9]+\.[0-9]+ { yylval.fval = atof(yytext); return FLOAT; }
[0-9]+         { yylval.ival = atoi(yytext); return INT; }
[a-zA-Z0-9]+   {
  yylval.sval = strdup(yytext);
  return STRING;
}
\n             { ++line_num; return ENDL; }
.              ;
%%