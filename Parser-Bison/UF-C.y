%{
  #include <stdio.h>
  #include <stdlib.h>

  extern int line_num;

  // stuff from flex that bison needs to know about:
  extern int yylex();
  extern int yyparse();
  extern FILE *yyin;
 
  void yyerror(const char *s);
%}

%code requires {
  #include "Utils.h"
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
%token IQ FANS BONES ANNOUNCES
%token PRINT

%token ADD MINUS MULTIPLY DIVIDE

%token TEST_GTR TEST_STR_GTR
%token COLON HYPHEN COMMA

%token BEGIN_TEST BEGIN_BRANCH BEGIN_ELSE
%token BEGIN_COMPARISON BEGIN_CONDITION 

%token BEGIN_ARGS FUNC_DEF_BEGIN_ARGS FUNC_DEF_END_ARGS END_FUNC RETURN
%token BREAK CONTINUE

%token BEGIN_RETURN_VAR

%token WHILE LOOP_NOTNULL LOOP_GTR LOOP_EQ LOOP_NOT_EQ LOOP_BEGIN_ACTION

// define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the union:
%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING
%token VOID

%type<typeVal> operator
%type<comparatorVal> comparator
%type<nodeVal> UF-C definitions
%type<nodeVal> varDef varDefs function_def function_defs function_body
%type<nodeVal> body_line body_lines
%type<nodeVal> while_loop func_call print return assignment
%type<nodeVal> id constant exp args nonVoidArg ids
%type<nodeVal> test test_comparisons_declarations test_comparison_declaration disjunctive_normal_form_comparisons andComparisons comparisonId test_if_branch test_if_branchs test_else_branch test_branchs

%%

// The first rule defined is the highest-level rule
UF-C:
  definitions body_lines footer { $$ = CreateBasicNode(atList, $1, $2, NULL); }
  ;


definitions:
  definitions varDefs { $$ = CreateBasicNode(atList, $1, $2, NULL); }
  | definitions function_defs { $$ = CreateBasicNode(atList, $1, $2, NULL); }
  | varDefs { $$ = $1; }
  | function_defs { $$ = $1; }
  ;
varDefs:
  varDefs varDef { $$ = CreateBasicNode(atList, $1, $2, NULL); }
  | varDef { $$ = $1; }
  ;
varDef:
  id FANS INT 
    {
      struct AstNode *intDefNode = CreateBasicNode(atIntDef, $1, NULL, NULL); 
      intDefNode->i = $3;

      $$ = intDefNode;
    }
  | id BONES INT { // Char values
    // if ($3 < 255)
    printf("New defined char \n");
  }
  | id IQ FLOAT 
    {
      struct AstNode *floatDefNode = CreateBasicNode(atFloatDef, $1, NULL, NULL); 
      floatDefNode->f = $3;

      $$ = floatDefNode;
    }
  | id ANNOUNCES STRING
    {
      struct AstNode *stringDefNode = CreateBasicNode(atStringDef, $1, NULL, NULL); 
      stringDefNode->s = $3;

      $$ = stringDefNode;
    }
  ;
function_defs:
  function_defs function_def { $$ = CreateBasicNode(atList, $1, $2, NULL); }
  |function_def { $$ = $1; }
  ;
function_def:
  id FUNC_DEF_BEGIN_ARGS args FUNC_DEF_END_ARGS function_body
    { $$ = CreateBasicNode(atFuncDef, $1, $3, $5); }
  ;
function_body:
  body_lines END_FUNC { $$ = $1; }
  ;


body_lines:
  body_lines body_line { $$ = CreateBasicNode(atList, $1, $2, NULL); }
  | body_line { $$ = $1; }
  ;
body_line:
  exp { $$ = $1; }
  | assignment { $$ = $1; }
  | test { $$ = $1; }
  | func_call { $$ = $1; }
  | while_loop { $$ = $1; }
  | print { $$ = $1; }
  | BREAK { $$ = CreateBasicNode(atBreak, NULL, NULL, NULL); }
  | CONTINUE { $$ = CreateBasicNode(atContinue, NULL, NULL, NULL); }
  | return { $$ = $1; }
  ;
exp:
  exp operator exp { $$ = CreateBasicNode($2, $1, $3, NULL); }
  | constant { $$ = $1; }
  | id { $$ = $1; }
  ;
assignment:
  id ASSIGN exp { $$ = CreateBasicNode(atAssignment, $1, $3, NULL); }
  | ids ASSIGN func_call { $$ = CreateBasicNode(atAssignment, $1, $3, NULL); }
  ;

test:
  BEGIN_TEST test_comparisons_declarations BEGIN_BRANCH test_branchs { $$ = CreateBasicNode(atTest, $2, $4, NULL); }
  ;
test_comparisons_declarations:
  test_comparisons_declarations HYPHEN test_comparison_declaration { $$ = CreateBasicNode(atList, $1, $3, NULL); }
  | HYPHEN test_comparison_declaration { $$ = $2; }
  ;
test_comparison_declaration:
  BEGIN_COMPARISON INT COLON nonVoidArg comparator nonVoidArg
    {
      struct AstNode *testComparisonNode = CreateBasicNode(atComparisonDeclaration, $4, $6, NULL);
      testComparisonNode->i = $2; //Numéro du match et donc id de la comparaison
      testComparisonNode->comparator = $5;
      
      $$ = testComparisonNode;
    }
  ;
comparator:
  TEST_GTR { $$ = gtr; }
  | TEST_STR_GTR { $$ = str_gtr; }
  ;
test_branchs:
  test_if_branchs HYPHEN test_else_branch { $$ = CreateBasicNode(atList, $1, $3, NULL); }
  | test_if_branchs { $$ = $1; }
test_if_branchs:
  test_if_branchs HYPHEN test_if_branch { $$ = CreateBasicNode(atList, $1, $3, NULL); }
  | HYPHEN test_if_branch { $$ = $2; }
  ;
test_if_branch:
  id BEGIN_CONDITION disjunctive_normal_form_comparisons BEGIN_ARGS args BEGIN_RETURN_VAR ids
    {
      struct AstNode *callFuncNode = CreateBasicNode(atFuncCall, $1, $5, NULL);
      struct AstNode *assignNode = CreateBasicNode(atAssignment, $7, callFuncNode, NULL);
      struct AstNode *ifBranchNode = CreateBasicNode(atTestIfBranch, $3, assignNode, NULL);

      $$ = ifBranchNode;
    }
  ;
disjunctive_normal_form_comparisons:
  disjunctive_normal_form_comparisons COMMA andComparisons 
    { $$ = CreateBasicNode(atLogicalOr, $1, $3, NULL); }
  | andComparisons { $$ = $1; }
  ;
andComparisons:
  andComparisons AND comparisonId
    { $$ = CreateBasicNode(atLogicalAnd, $1, $3, NULL); }
  | comparisonId { $$ = $1; }
  ;
test_else_branch:
  id BEGIN_ELSE BEGIN_ARGS args BEGIN_RETURN_VAR ids
    {
      struct AstNode *callFuncNode = CreateBasicNode(atFuncCall, $1, $4, NULL);
      struct AstNode *assignNode = CreateBasicNode(atAssignment, $6, callFuncNode, NULL);
      struct AstNode *elseBranchNode = CreateBasicNode(atTestElseBranch, assignNode, NULL, NULL);

      $$ = elseBranchNode;
    }
  ;

func_call:
  id BEGIN_ARGS args { $$ = CreateBasicNode(atFuncCall, $1, $3, NULL); }
  ;
while_loop:
  nonVoidArg WHILE nonVoidArg LOOP_NOTNULL LOOP_BEGIN_ACTION assignment { $$ = CreateWhileNode(neq, $3, 0, $6); }
  | id WHILE id LOOP_GTR LOOP_BEGIN_ACTION assignment { $$ = CreateWhileNode(gtr, $1, $3, $6); }
  | id WHILE id LOOP_NOT_EQ LOOP_BEGIN_ACTION assignment { $$ = CreateWhileNode(neq, $1, $3, $6); }
  | id WHILE id LOOP_EQ LOOP_BEGIN_ACTION assignment { $$ = CreateWhileNode(eq, $1, $3, $6); }
  ;
print:
  PRINT args { $$ = CreateBasicNode(atPrint, $2, NULL, NULL); }
  ;
return:
  nonVoidArg RETURN { $$ = CreateBasicNode(atReturn, $1, NULL, NULL); }
  ;


footer:
  END
  | END endls
  ;
endls:
  endls ENDL
  | ENDL
  ;


args:
  args and id { $$ = CreateBasicNode(atList, $1, $3, NULL); }
  | args and constant { $$ = CreateBasicNode(atList, $1, $3, NULL); }
  | nonVoidArg { $$ = $1; }
  | VOID { $$ = CreateBasicNode(atVoid, NULL, NULL, NULL); }
  ;
nonVoidArg:
  id { $$ = $1; }
  | constant { $$ = $1; }
  ;
constant:
  INT
    {
      struct AstNode *intNode = CreateBasicNode(atInt, NULL, NULL, NULL);
      intNode->i = $1;

      $$ = intNode;
    }
  | FLOAT
    {
      struct AstNode *floatNode = CreateBasicNode(atFloat, NULL, NULL, NULL);
      floatNode->f = $1;

      $$ = floatNode;
    }
  ;

operator:
  ADD { $$ = atAdd; }
  | MINUS { $$ = atMinus; }
  | MULTIPLY { $$ = atDivide; }
  | DIVIDE { $$ = atMultiply; }
  ;

comparisonId:
  INT
    {
      struct AstNode *comparisonIdNode = CreateBasicNode(atComparisonId, NULL, NULL, NULL);
      comparisonIdNode->i = $1;
      
      $$ = comparisonIdNode;
    }
  ;
ids:
  ids and id { $$ = CreateBasicNode(atList, $1, $3, NULL); }
  | id { $$ = $1; }
  ;
id:
  STRING
    {
      struct AstNode *idNode = CreateBasicNode(atId, NULL, NULL, NULL);
      idNode->s = $1;

      $$ = idNode;
    }
  ;
and:
  AND
  | ','
  ;

%%

int main() {
  // open a file handle to a particular file:
  FILE *myfile = fopen("in.ufc", "r");
  // make sure it's valid:
  if (!myfile) {
    printf("I can't open in.ufc file!\n");
    return -1;
  }
  // Set flex to read from it instead of defaulting to STDIN:
  yyin = myfile;

  // Parse through the input:
  yyparse();
}

void yyerror(const char *s) {
  printf("EEK, parse error on line %d! Message: %s", line_num, s);
  // might as well halt now:
  exit(-1);
}