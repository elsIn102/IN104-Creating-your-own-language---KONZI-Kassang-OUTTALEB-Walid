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

  new struct Argval
  {
    char* name;
    char type; //0 pour int, 1 pour float et 2 pour string

    int ival;
    float fval;
    char* sval;
  }

  new struct ArgList
  {
    struct Argval argval;
    struct ArgList* next;
  }

  char* printArgs(struct ArgList al)
  {
    struct ArgList next = al.next;
    cout << al.argval.type;

    while (next != NULL)
    {
      cout << " - " << next.argval.type;
      next = next.next;
    }

    cout << endl;

  }
%}

%union {
  int ival;
  unsigned int uival;
  float fval;
  char* sval;

  struct ArgList argList;
}

// define the constant-string tokens:
%token IQ FANS BONES KG
%token END ENDL
%token BEGIN_ARGS END_ARGS END_FUNC RETURN

%token AND_CONDITION OR_CONDITION AND RETURN_VAR

// define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the union:
%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING
%token <uival> UINT

%type<sval> id
%type<argList> args

%%

// the first rule defined is the highest-level rule, which in our
// case is just the concept of a whole "snazzle file":
UF-C:
  template body_section footer {
      cout << "done with a snazzle file!" << endl;
    }
  ;

template:
  template varDefs
  | template function_defs
  | varDefs
  | function_defs
  ;
varDefs:
  varDefs varDef
  | varDef
  ;
function_defs:
  function_defs function_def
  |function_def
  ;

varDef:
  id FANS INT ENDLS {
    cout << "New defined int:" << $1 << " of value: " << $3 << endl;
    free($1);
  }
  | id BONES INT ENDLS { // Char values
    // val = atoc
    cout <<"New defined string:"<< $1 << " of value: " << $3 <<endl;
    free($1);
  }
  |id IQ FLOAT ENDLS {
      cout << "New defined float: " << $1 << " of value: " << $3 << endl;
      free($1);
    }
  |id KG UINT ENDLS {
    cout << "New defined unsigned int: " << $1 << " of value: " << $3 << endl;
    free($1);
  }
  ;

function_def:
  id BEGIN_ARGS args END_ARGS func_body
    {
      cout << "function " << $1 << " defined with arguments : " << printArgs($3) << endl;
      free($1);
      //free args
    }
  ;
func_body:
  body_lines {id|INT|FLOAT} RETURN END_FUNC
  {
    cout << "function returned : " << $2 << endl;
    //free id
  }
  ;


args:
  args ',' id { $$.argval.type = 2; $$.argval.sval = $3; $$.next = $1; }
  | args ',' FLOAT { $$.argval.type = 1; $$.argval.fval = $3; $$.next = $1; }
  | args ',' INT { $$.argval.type = 0; $$.argval.ival = $3; $$.next = $1; }
  | id { $$.argval.type = 2; $$.argval.sval = $1; }
  | FLOAT { $$.argval.type = 1; $$.argval.fval = $1; }
  | INT { $$.argval.type = 0; $$.argval.ival = $1; }
  | 
  ;

id:
  STRING { $$ = $1; }
  ;


body_section:
  body_lines
  ;
body_lines:
  body_lines body_line
  | body_line
  ;
body_line:
  assignment
  | test
  | func_call
  | loop
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