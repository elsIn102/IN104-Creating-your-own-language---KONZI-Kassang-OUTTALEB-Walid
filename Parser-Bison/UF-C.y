%code requires {
  #include "Utils.h"
}

%code {
  #include <stdio.h>
  #include <stdlib.h>


  extern int line_num;

  // stuff from flex that bison needs to know about:
  extern int yylex();
  extern int yyparse();
  extern FILE *yyin;
 
  void yyerror(struct AstNode** errorAstPtr, const char *s);
}

//defines a pointer that will be required when calling the parser, allowing the caller to access the AST
%parse-param {struct AstNode** ast}

%union {
  int ival;
  float fval;
  char* sval;

  struct AstNode *nodeVal;
  enum AstType typeVal;
  enum ComparatorType comparatorVal;
  enum VariableType varTypeVal;
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
%token TYPE_FLOAT TYPE_INT TYPE_STRING
%token BREAK CONTINUE

%token BEGIN_RETURN_VAR

%token WHILE LOOP_NOTNULL LOOP_GTR LOOP_EQ LOOP_NOT_EQ LOOP_BEGIN_ACTION

// define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the union:
%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING STRING_CONSTANT
%token VOID

%type<sval> string
%type<typeVal> operator
%type<varTypeVal> funcReturnType
%type<comparatorVal> comparator
%type<nodeVal> definitions body_line body_lines
%type<nodeVal> varDef function_def function_body
%type<nodeVal> while_loop func_call print return assignment assignmentOrFuncCall
%type<nodeVal> id ids idOrVoid constant exp args void nonVoidArg nonVoidArgs
%type<nodeVal> test test_comparisons_declarations test_comparison_declaration disjunctive_normal_form_comparisons andComparisons comparisonId test_if_branch test_if_branchs test_else_branch test_branchs

%%

// The first rule defined is the highest-level rule
UF-C:
  endls definitions body_lines footer { *ast = CreateBasicNode(atRoot, $2, $3, NULL); }
  ;


definitions:
  definitions varDef endls { $$ = CreateBasicNode(atStatementList, $1, $2, NULL); }
  | definitions function_def endls { $$ = CreateBasicNode(atStatementList, $1, $2, NULL); }
  | varDef endls { $$ = CreateBasicNode(atStatementList, $1, NULL, NULL); }
  | function_def endls { $$ = CreateBasicNode(atStatementList, $1, NULL, NULL); }
  ;
varDef:
  id FANS INT
    {
      struct AstNode *intDefNode = CreateBasicNode(atVariableDef, $1, NULL, NULL); 
      intDefNode->variableType = integer;
      intDefNode->i = $3;

      $$ = intDefNode;
    }
  | id BONES INT { // Char values
    // if ($3 < 255)
    printf("New defined char \n");
  }
  | id IQ FLOAT
    {
      struct AstNode *floatDefNode = CreateBasicNode(atVariableDef, $1, NULL, NULL); 
      floatDefNode->variableType = floating;
      floatDefNode->f = $3;

      $$ = floatDefNode;
    }
  | id ANNOUNCES string
    {
      struct AstNode *stringDefNode = CreateBasicNode(atVariableDef, $1, NULL, NULL); 
      stringDefNode->variableType = characters;
      stringDefNode->s = $3;

      $$ = stringDefNode;
    }
  ;
function_def:
  id FUNC_DEF_BEGIN_ARGS ids FUNC_DEF_END_ARGS funcReturnType COLON endls function_body
    {
      struct AstNode *funcDefNode = CreateBasicNode(atFuncDef, $1, $3, $8); 
      funcDefNode->variableType = $5;

      $$ = funcDefNode;
    }
  ;
funcReturnType:
  TYPE_FLOAT { $$ = floating; }
  | TYPE_INT { $$ = integer; }
  | TYPE_STRING { $$ = characters; }
  ;
function_body:
  body_lines END_FUNC { $$ = $1; }
  ;


body_lines:
  body_lines body_line { $$ = CreateBasicNode(atStatementList, $1, $2, NULL); }
  | body_line { $$ = CreateBasicNode(atStatementList, $1, NULL, NULL); }
  ;
body_line:
  exp endls { $$ = $1; }
  | assignment endls { $$ = $1; }
  | test endls { $$ = $1; }
  | func_call endls { $$ = $1; }
  | while_loop endls { $$ = $1; }
  | print endls { $$ = $1; }
  | BREAK endls { $$ = CreateBasicNode(atBreak, NULL, NULL, NULL); }
  | CONTINUE endls { $$ = CreateBasicNode(atContinue, NULL, NULL, NULL); }
  | return endls { $$ = $1; }
  ;
exp:
  exp operator exp { $$ = CreateBasicNode($2, $1, $3, NULL); }
  | constant { $$ = $1; }
  | id { $$ = $1; }
  ;
assignment:
  id ASSIGN exp { $$ = CreateBasicNode(atAssignment, $1, $3, NULL); }
  | id ASSIGN func_call { $$ = CreateBasicNode(atAssignment, $1, $3, NULL); }
  ;

test:
  BEGIN_TEST COLON endls test_comparisons_declarations BEGIN_BRANCH endls test_branchs { $$ = CreateBasicNode(atTest, $4, $7, NULL); }
  ;
test_comparisons_declarations:
  test_comparisons_declarations HYPHEN test_comparison_declaration { $$ = CreateBasicNode(atStatementList, $1, $3, NULL); }
  | HYPHEN test_comparison_declaration { $$ = CreateBasicNode(atStatementList, $2, NULL, NULL); }
  ;
test_comparison_declaration:
  BEGIN_COMPARISON INT COLON nonVoidArg comparator nonVoidArg endls
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
  test_if_branchs HYPHEN test_else_branch { $$ = CreateBasicNode(atStatementList, $3, $1, NULL); }
  | test_if_branchs { $$ = CreateBasicNode(atStatementList, $1, NULL, NULL); }
test_if_branchs:
  test_if_branchs HYPHEN test_if_branch { $$ = CreateBasicNode(atStatementList, $3, $1, NULL); }
  | HYPHEN test_if_branch { $$ = CreateBasicNode(atStatementList, $2, NULL, NULL); }
  ;
test_if_branch:
  id BEGIN_CONDITION disjunctive_normal_form_comparisons BEGIN_ARGS args BEGIN_RETURN_VAR idOrVoid endls
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
  id BEGIN_ELSE BEGIN_ARGS args BEGIN_RETURN_VAR idOrVoid
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
  nonVoidArg WHILE nonVoidArg LOOP_NOTNULL endls LOOP_BEGIN_ACTION assignmentOrFuncCall 
    { $$ = CreateWhileNode(neq, $3, 0, $7); }
  | nonVoidArg WHILE nonVoidArg LOOP_GTR endls LOOP_BEGIN_ACTION assignmentOrFuncCall 
    { $$ = CreateWhileNode(gtr, $1, $3, $7); }
  | nonVoidArg WHILE nonVoidArg LOOP_NOT_EQ endls LOOP_BEGIN_ACTION assignmentOrFuncCall 
    { $$ = CreateWhileNode(neq, $1, $3, $7); }
  | nonVoidArg WHILE nonVoidArg LOOP_EQ endls LOOP_BEGIN_ACTION assignmentOrFuncCall 
    { $$ = CreateWhileNode(eq, $1, $3, $7); }
  ;
assignmentOrFuncCall:
  assignment { $$ = $1; }
  | func_call { $$ = $1; }
  ;
print:
  PRINT nonVoidArgs { $$ = CreateBasicNode(atPrint, $2, NULL, NULL); }
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
  nonVoidArgs { $$ = $1; }
  | args and void { $$ = CreateBasicNode(atElemList, $1, $3, NULL); }
  | void { $$ = CreateBasicNode(atElemList, $1, NULL, NULL); }
  ;
nonVoidArgs:
  nonVoidArgs and nonVoidArg { $$ = CreateBasicNode(atElemList, $1, $3, NULL); }
  | nonVoidArg { $$ = CreateBasicNode(atElemList, $1, NULL, NULL); }
  ;
nonVoidArg:
  id { $$ = $1; }
  | constant { $$ = $1; }
  ;
constant:
  INT
    {
      struct AstNode *intNode = CreateBasicNode(atConstant, NULL, NULL, NULL);
      intNode->variableType = integer;
      intNode->i = $1;

      $$ = intNode;
    }
  | FLOAT
    {
      struct AstNode *floatNode = CreateBasicNode(atConstant, NULL, NULL, NULL);
      floatNode->variableType = floating;
      floatNode->f = $1;

      $$ = floatNode;
    }
  | string
    {
      struct AstNode *stringNode = CreateBasicNode(atConstant, NULL, NULL, NULL);
      stringNode->variableType = characters;
      stringNode->s = $1;

      $$ = stringNode;
    }
  ;
string:
  STRING { $$ = $1; }
  | STRING_CONSTANT { $$ = $1; }
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
idOrVoid:
  id { $$ = $1; }
  | void { $$ = $1; }
  ;
ids:
  ids and id { $$ = CreateBasicNode(atElemList, $3, $1, NULL); }
  | id { $$ = CreateBasicNode(atElemList, $1, NULL, NULL); }
  ;
id:
  STRING
    {
      struct AstNode *idNode = CreateBasicNode(atId, NULL, NULL, NULL);
      idNode->s = $1;

      $$ = idNode;
    }
  ;
void:
  VOID { $$ = CreateBasicNode(atVoid, NULL, NULL, NULL); }
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

  struct AstNode* _ast;

  // Parse through the input:
  int error = yyparse(&_ast);
  if (error != 0)
  {
    printf("Error during parsing\n");
    return 1;
  }

  FILE* outFile = fopen("out.ufc", "w");
  if (outFile!=NULL)
  {
    AstToCode(_ast, outFile);
    fclose(outFile);
  }

  FreeAST(_ast);

  return 0;
}

void yyerror(struct AstNode** errorAstPtr, const char *s) {
  printf("Parse error on line %d : %s\n", line_num, s);
  // might as well halt now:
  FreeAST(*errorAstPtr);
  exit(-1);
}