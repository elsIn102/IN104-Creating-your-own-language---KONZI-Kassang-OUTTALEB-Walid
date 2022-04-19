%{
  #include <cstdio>
  #include <iostream>
  using namespace std;

  // stuff from flex that bison needs to know about:
  extern int yylex();
  extern int yyparse();
  extern FILE *yyin;
  extern int line_num;
 
  void yyerror(const char *s);
%}

%union {
  int ival;
  float fval;
  char* sval;
}

// define the constant-string tokens:
%token IQ 
%token END ENDL

// define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the union:
%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING

%%

// the first rule defined is the highest-level rule, which in our
// case is just the concept of a whole "snazzle file":
snazzle:
  template body_section footer {
      cout << "done with a snazzle file!" << endl;
    }
  ;

template:
  typelines
  ;
typelines:
  typelines typeline
  | typeline
  ;
typeline:
  id IQ FLOAT ENDLS {
      cout << "new defined snazzle type: " << $2 << endl;
      free($2);
    }
  
id:
  STRING
  ;
body_section:
  body_lines
  ;
body_lines:
  body_lines body_line
  | body_line
  ;
body_line:
  INT INT INT INT STRING ENDLS {
      cout << "new snazzle: " << $1 << $2 << $3 << $4 << $5 << endl;
      free($5);
    }
  ;
footer:
  END
  | END ENDLS
  ;
ENDLS:
  ENDLS ENDL
  | ENDL
  ;

%%

int main(int, char**) {
  // open a file handle to a particular file:
  FILE *myfile = fopen("in.ufc", "r");
  // make sure it's valid:
  if (!myfile) {
    cout << "I can't open a.ufc.file!" << endl;
    return -1;
  }
  // Set flex to read from it instead of defaulting to STDIN:
  yyin = myfile;

  // Parse through the input:
  yyparse();
}

void yyerror(const char *s) {
  cout << "EEK, parse error on line " << line_num << "!  Message: " << s << endl;
  // might as well halt now:
  exit(-1);
}