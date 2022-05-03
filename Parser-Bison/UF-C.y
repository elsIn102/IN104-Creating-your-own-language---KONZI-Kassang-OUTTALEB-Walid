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
%token END ENDL

%token ASSIGN AND
%token IQ FANS BONES KG
%token PRINT

%token GTR STR_GTR
%token COLON HYPHEN COMMA

%token BEGIN_TEST BEGIN_BRANCH
%token BEGIN_PROPOSITION BEGIN_CONDITION 

%token BEGIN_ARGS END_ARGS END_FUNC RETURN
%token FUNC_BEGIN_ARGS BREAK CONTINUE

%token BEGIN_RETURN_VAR

%token WHILE LOOP_NOTNULL LOOP_GTR LOOP_EQ LOOP_DIFF LOOP_BEGIN_ACTION

// define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the union:
%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING
%token <uival> UINT
%token VOID

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
  body_lines id RETURN END_FUNC
  | body_lines INT RETURN END_FUNC
  | body_lines FLOAT RETURN END_FUNC
  {
    cout << "function returned : " << $2 << endl;
    //free id
  }
  ;


args:
  args {','|AND} id { $$.argval.type = 2; $$.argval.sval = $3; $$.next = $1; }
  | args {','|AND} FLOAT { $$.argval.type = 1; $$.argval.fval = $3; $$.next = $1; }
  | args {','|AND} INT { $$.argval.type = 0; $$.argval.ival = $3; $$.next = $1; }
  | id { $$.argval.type = 2; $$.argval.sval = $1; }
  | FLOAT { $$.argval.type = 1; $$.argval.fval = $1; }
  | INT { $$.argval.type = 0; $$.argval.ival = $1; }
  | VOID {}
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
  | print
  | BREAK
  | CONTINUE
  | 
  ;
print:
  PRINT args
  ;
assignment:
  id ASSIGN id
  ;
test:
  BEGIN_TEST test_propositions BEGIN_BRANCH test_branchs
  ;
test_propositions:
  test_propositions HYPHEN test_proposition
  | HYPHEN test_proposition
  ;
test_proposition:
  BEGIN_PROPOSITION INT COLON id operator id
  ;
operator:
  GTR
  | STR_GTR
  ;
test_branchs:
  test_branchs HYPHEN test_branch
  | HYPHEN test_branch
  ;
test_branch:
  id BEGIN_CONDITION test_values BEGIN_ARGS args BEGIN_RETURN_VAR ids
  ;
test_values:
  test_values COMMA ids
  | ids
  ;
func_call:
  id ASSIGN ids FUNC_BEGIN_ARGS args
  ;
loop:
  id WHILE id LOOP_NOTNULL LOOP_BEGIN_ACTION body_line
  | id WHILE id LOOP_GTR LOOP_BEGIN_ACTION body_line
  | id WHILE id LOOP_EQ LOOP_BEGIN_ACTION body_line
  | id WHILE id LOOP_DIFF LOOP_BEGIN_ACTION body_line
  ;
ids:
  ids AND id
  | id;
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