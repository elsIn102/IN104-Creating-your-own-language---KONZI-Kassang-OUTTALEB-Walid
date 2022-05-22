%{
  #include "../Parser-Bison/UF-C.tab.h"

  int line_num = 1;
  int char_pos_in_line = 1, current_token_length = 0, previous_token_length = 0;
  int stringLength = 0;

  #define YY_USER_ACTION char_pos_in_line += yyleng; previous_token_length = current_token_length; current_token_length = yyleng;

  void ResetCharacterPosInLine()
  {
    char_pos_in_line = 1;
    previous_token_length = 0;
    current_token_length = 0;
  }
%}

%option noyywrap

%x COMMENTS
%x READING_STRING


%%
\/\*            { BEGIN(COMMENTS); } // start of a comment: go to a 'COMMENTS' state.
<COMMENTS>\*\/  { BEGIN(INITIAL); }  // end of a comment: go back to normal parsing.
<COMMENTS>\n    { line_num++; ResetCharacterPosInLine(); }      // still have to increment line numbers inside of comments!
<COMMENTS>.     ;                    // ignore every other character while we are in this state

\" { BEGIN(READING_STRING); stringLength = 0;}
<READING_STRING>\" { BEGIN(INITIAL); stringLength++; }
<READING_STRING>\n { ++line_num; stringLength++; ResetCharacterPosInLine(); }
<READING_STRING>[^\"]* { yylval.sval = strdup(yytext); stringLength++; return STRING_CONSTANT; }

"is starting their training with" {return FUNC_DEF_BEGIN_ARGS;}
"to increase their"  { return FUNC_DEF_END_ARGS;}
"the smart" { return FLOAT_FUNC_ARG; }
"the famous" { return INT_FUNC_ARG; }
"the massive" { return STRING_FUNC_ARG; }

"noone" { return VOID; }

[Tt]"raining is over" {return END_FUNC;}
[ \t] ;
end            { return END; }


"has this number of fans"[ ]?":" {return FANS;} //declaration of variables: int
"has an IQ of" { return IQ; }  //float
"announces" {return ANNOUNCES;}  //declare a string

"IQ" {return TYPE_FLOAT;}
"fame" {return TYPE_INT;}
"size" {return TYPE_STRING;}
"effectiveness" {return TYPE_VOID;}

"is thrown out" {return RETURN;}
"hits" {return ASSIGN;}
"punches" { return ASSIGN_FUNC; }
"bets on" {return BEGIN_CONDITION;}
"takes the rest of the bets" {return BEGIN_ELSE;}
[Ff]"inally"[,]? { return BEGIN_ELSE_BRANCH;}
[Tt]"he gambling den closes" { return END_CONDITION; }
"and" {return AND;}
"using" {return BEGIN_ARGS;}
"with" {return BEGIN_ARGS;}
"enrolls" {return BEGIN_ARGS;}
"and gives the money to" {return BEGIN_RETURN_VAR;}
[Aa]" new tournament begins" {return BEGIN_TEST;}
[Aa]"nd the gambling den opens" {return BEGIN_BRANCH;} 
[Mm]"atch" {return BEGIN_COMPARISON; }
":" {return COLON;}
"-" {return HYPHEN;}
"," {return COMMA;}



"challenges" {return TEST_GTR;}
"fights" {return TEST_STR_GTR;}

"beats down" {return WHILE;}
"until they give up" {return LOOP_NOTNULL;}
"until someone splits them up" {return LOOP_EQ;}
"until they fight back" {return LOOP_GTR;}
"until they come to an agreement" {return LOOP_NOT_EQ;}
[Mm]"eanwhile"[,]? {return LOOP_BEGIN_ACTION;}

[Tt]"he match is interrupted" {return BREAK;}
[Ee]"nd of the round" {return CONTINUE;}

[tT]"he ring girl shows" {return PRINT;}
"the fans of" {return PRINT_INT;}
"the wits of" {return PRINT_FLOAT;}
"the flow of" {return PRINT_STRING;}

[Aa]" time out is announced" { return PRINT_ENDL; }


"joins" {return ADD;}
"tosses away" {return MINUS;}
"tears apart" {return DIVIDE;}
"deals with" {return MULTIPLY;}

([Aa]"nd ")?[Tt]"he competition begins" { return DEFINITIONS_END; }

[-]?[0-9]+ {yylval.ival= atoi(yytext); return INT;} 
[-]?[0-9]+\.[0-9]+ { yylval.fval = atof(yytext); return FLOAT; }


[a-zA-Z0-9_]+   {
  yylval.sval = strdup(yytext);
  return STRING;
}
\n             { ++line_num; ResetCharacterPosInLine(); return ENDL; }
.              ;
%%
