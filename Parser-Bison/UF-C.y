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

  /*
  new struct Argval
  {
    char* name;
    char type; //0 pour int, 1 pour float et 2 pour string

    int ival;
    float fval;
    char* sval;
  }
  */
%}

%code requires {
  enum AstType 
  {
    atList, atLogicalOr, atLogicalAnd,
    atTest, atComparisonDeclaration, atComparisonId, atTestBranch, atTestElseBranch,
    atAssignment, atFuncCall, atBreak, atReturn,
    atId, atInt, atFloat, atVoid,
    atAdd, atMinus, atMultiply, atDivide, atPrint
  };

  enum ComparatorType
  {
    gtr, str_gtr
  };

  struct AstNode
  {
    enum AstType type;
    enum ComparatorType comparator;

    char* s; int i; float f;

    struct AstNode *child1;
    struct AstNode *child2;
    struct AstNode *child3;
    struct AstNode *child4;
  };


  void PointerNullError()
  {

    return;
  }

  struct AstNode* createListNode (struct AstNode* elem1, struct AstNode* elem2)
  {
    struct AstNode* node = (struct AstNode*) malloc(sizeof (struct AstNode)); 
      if (node==NULL)
        PointerNullError();

      node->type = atList;
      node->child1 = elem1;
      node->child2 = elem2;

      return node;
  }
}

%union {
  int ival;
  float fval;
  char* sval;

  struct AstNode *nodeVal;
  enum AstType typeVal;
  enum ComparatorType comparatorVal;
}

// define the constant-string tokens:
%token END ENDL

%token ASSIGN AND
%token IQ FANS BONES
%token PRINT

%token ADD MINUS MULTIPLY DIVIDE

%token TEST_GTR TEST_STR_GTR
%token COLON HYPHEN COMMA

%token BEGIN_TEST BEGIN_BRANCH BEGIN_ELSE
%token BEGIN_COMPARISON BEGIN_CONDITION 

%token BEGIN_ARGS END_ARGS END_FUNC RETURN
%token FUNC_BEGIN_ARGS BREAK CONTINUE

%token BEGIN_RETURN_VAR

%token WHILE LOOP_NOTNULL LOOP_GTR LOOP_EQ LOOP_DIFF LOOP_BEGIN_ACTION

// define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the union:
%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING
%token VOID

%type<typeVal> operator
%type<comparatorVal> comparator
%type<nodeVal> id constant exp args print body_line body_lines return assignment nonVoidArg ids test test_comparisons_declarations test_comparison_declaration
%type<nodeVal> disjunctive_normal_form_comparisons andComparisons comparisonId test_branch test_else_branch test_branchs

%%

// the first rule defined is the highest-level rule, which in our
// case is just the concept of a whole "snazzle file":
UF-C:
  template body_lines footer {
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
  ;

function_def:
  id BEGIN_ARGS args END_ARGS func_body
    {
      cout << "function " << $1 << " defined with arguments : " << endl;
      free($1);
      //free args
    }
  ;
func_body:
  body_lines END_FUNC
  {
    cout << "function returned " << endl;
  }
  ;
args:
  args and id { $$ = createListNode($1, $3); }
  | args and constant { $$ = createListNode($1, $3); }
  | nonVoidArg { $$ = $1; }
  | VOID
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode)); 
      if (node==NULL)
        PointerNullError();

      node->type = atVoid;

      $$ = node;
    }
  ;
nonVoidArg:
  id { $$ = $1; }
  | constant { $$ = $1; }
  ;
and:
  AND
  | ','
  ;
id:
  STRING
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode)); 
      if (node==NULL)
        PointerNullError();

      node->type = atId;
      node->s = $1;

      $$ = node;
    }
  ;
constant:
  INT
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode)); 
      if (node==NULL)
        PointerNullError();

      node->type = atInt;
      node->i = $1;

      $$ = node;
    }
  | FLOAT
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode)); 
      if (node==NULL)
        PointerNullError();

      node->type = atFloat;
      node->f = $1;

      $$ = node;
    }
  ;
body_lines:
  body_lines body_line { $$ = createListNode($1, $2); }
  | body_line { $$ = $1; }
  ;
body_line:
  exp { $$ = $1; }
  | assignment
  | test
  | func_call
  | loop
  | print { $$ = $1; }
  | BREAK
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode)); 
      if (node==NULL)
        PointerNullError();

      node->type = atBreak;

      $$ = node;
    }
  | CONTINUE
  | return { $$ = $1; }
  | 
  ;

return:
  nonVoidArg RETURN
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode)); 
      if (node==NULL)
        PointerNullError();

      node->type = atReturn;
      node->child1 = $1;

      $$ = node;
    }
  ;
exp:
  exp operator exp 
    { 
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode)); 
      if (node==NULL)
        PointerNullError();

      node->type = $2;
      node->child1 = $1; 
      node->child2 = $3;

      $$ = node;
    }
  | constant { $$ = $1; }
  ;
operator:
  ADD { $$ = atAdd; }
  | MINUS { $$ = atMinus; }
  | MULTIPLY { $$ = atDivide; }
  | DIVIDE { $$ = atMultiply; }
  ;
print:
  PRINT args 
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode));
        if (node==NULL)
          PointerNullError();

        node->type = atPrint; 
        node->child1 = $2;
        
        $$ = node;
    }
  ;
assignment:
  id ASSIGN id
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode));
      if (node==NULL)
        PointerNullError();

      node->type = atAssignment; 
      node->child1 = $1;
      node->child2 = $3;
      
      $$ = node;
    }
  ;
test:
  BEGIN_TEST test_comparisons_declarations BEGIN_BRANCH test_branchs
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode));
      if (node==NULL)
        PointerNullError();

      node->type = atTest;
      node->child1 = $2;
      node->child2 = $4;
      
      $$ = node;
    }
  ;
test_comparisons_declarations:
  test_comparisons_declarations HYPHEN test_comparison_declaration 
    { $$ = createListNode($1, $3); }
  | HYPHEN test_comparison_declaration { $$ = $2; }
  ;
test_comparison_declaration:
  BEGIN_COMPARISON INT COLON nonVoidArg comparator nonVoidArg
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode));
      if (node==NULL)
        PointerNullError();

      node->type = atComparisonDeclaration;
      node->i = $2; //Numéro du match et donc id de la comparaison

      node->comparator = $5;
      node->child1 = $4;
      node->child2 = $6;
      
      $$ = node;
    }
  ;
comparator:
  TEST_GTR { $$ = gtr; }
  | TEST_STR_GTR { $$ = str_gtr; }
  ;
test_branchs:
  test_branchs HYPHEN test_branch { $$ = createListNode($1, $3); }
  | test_branchs HYPHEN test_else_branch { $$ = createListNode($1, $3); }
  | HYPHEN test_branch { $$ = $2; }
  ;
ids:
  ids and id { $$ = createListNode($1, $3); }
  | id { $$ = $1; }
  ;
test_branch:
  id BEGIN_CONDITION disjunctive_normal_form_comparisons BEGIN_ARGS args BEGIN_RETURN_VAR ids
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode));
      if (node==NULL)
        PointerNullError();

      node->type = atTestBranch;
      node->child1 = $3;
      node->child2 = $7;
      node->child3 = $1;
      node->child4 = $5;
      
      $$ = node;
    }
  ;
test_else_branch:
  id BEGIN_ELSE BEGIN_ARGS args BEGIN_RETURN_VAR ids
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode)); 
      if (node==NULL)
        PointerNullError();

      node->type = atTestElseBranch;
      node->child1 = $6;
      node->child2 = $1;
      node->child3 = $4;

      $$ = node;
    }
  ;
disjunctive_normal_form_comparisons:
  disjunctive_normal_form_comparisons COMMA andComparisons
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode));
      if (node==NULL)
        PointerNullError();

      node->type = atLogicalOr;
      node->child1 = $1;
      node->child2 = $3;
      
      $$ = node;
    }
  | andComparisons { $$ = $1; }
  ;
andComparisons:
  andComparisons AND comparisonId
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode));
      if (node==NULL)
        PointerNullError();

      node->type = atLogicalAnd;
      node->child1 = $1;
      node->child2 = $3;
      
      $$ = node;
    }
  | comparisonId { $$ = $1; }
  ;
comparisonId:
  INT
    {
      struct AstNode *node = (struct AstNode*) malloc(sizeof (struct AstNode));
      if (node==NULL)
        PointerNullError();

      node->type = atComparisonId;
      node->i = $1;
      
      $$ = node;
    }
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