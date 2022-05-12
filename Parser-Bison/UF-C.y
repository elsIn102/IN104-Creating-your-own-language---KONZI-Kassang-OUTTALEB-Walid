%code requires {
  #include "../Utils/AST.h"
}

// For debugging
%define parse.trace

%code {
  #include <stdio.h>
  #include <stdlib.h>


  extern int line_num;
  extern int stringLength;
  extern int char_pos_in_line, current_token_length, previous_token_length;
  extern char* yytext; // Text of the current token

  // stuff from flex that bison needs to know about:
  extern int yylex();
 
  void yyerror(struct AstNode** errorAstPtr, const char *s);
}

//defines a pointer that will be required when calling the parser, allowing the caller to access the AST
%parse-param {struct AstNode** ast}

%union {
  int ival;
  float fval;
  char* sval;

  struct AstNode *nodeVal;
  enum ComparatorType comparatorVal;
  enum VariableType varTypeVal;
}


// define the constant-string tokens:
%token END ENDL
%token DEFINITIONS_END

%token ASSIGN ASSIGN_FUNC AND
%token IQ FANS ANNOUNCES
%token PRINT PRINT_INT PRINT_FLOAT PRINT_STRING

%left ADD MINUS MULTIPLY DIVIDE

%token TEST_GTR TEST_STR_GTR
%token COLON HYPHEN COMMA

%token BEGIN_TEST BEGIN_BRANCH BEGIN_ELSE
%token BEGIN_COMPARISON BEGIN_CONDITION BEGIN_ELSE_BRANCH END_CONDITION

%token BEGIN_ARGS FUNC_DEF_BEGIN_ARGS FUNC_DEF_END_ARGS END_FUNC RETURN
%token FLOAT_FUNC_ARG INT_FUNC_ARG STRING_FUNC_ARG
%token TYPE_FLOAT TYPE_INT TYPE_STRING TYPE_VOID
%token BREAK CONTINUE

%token BEGIN_RETURN_VAR

%token WHILE LOOP_NOTNULL LOOP_GTR LOOP_EQ LOOP_NOT_EQ LOOP_BEGIN_ACTION

// define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the union:
%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING STRING_CONSTANT
%token VOID

%type<varTypeVal> funcReturnType
%type<comparatorVal> comparator
%type<nodeVal> start
%type<nodeVal> definitions body_line body_lines
%type<nodeVal> varDef function_def function_body
%type<nodeVal> while_loop func_call print return assignment assignmentOrFuncCall
%type<nodeVal> id idOrVoid constant exp void args nonVoidArg nonVoidArgs nonVoidFuncArg nonVoidFuncArgs funcArgs
%type<nodeVal> test test_comparisons_declarations test_comparison_declaration disjunctive_normal_form_comparisons andComparisons comparisonId test_if_branch test_elseIf_branch test_elseIf_branchs test_else_branch test_branchs


%%

// The first rule defined is the highest-level rule
UF-C:
  endls start { *ast = $2; }
  | start { *ast = $1; }
  ;
start:
    definitions DEFINITIONS_END endls body_lines footer { $$ = CreateBasicNode(atRoot, $1, $4, NULL); }
  | definitions footer { $$ = CreateBasicNode(atRoot, $1, NULL, NULL); }
  | body_lines footer { $$ = CreateBasicNode(atRoot, NULL, $1, NULL); }
  | footer { $$ = CreateBasicNode(atRoot, NULL, NULL, NULL); }
  ;

definitions:
  varDef endls definitions { $$ = CreateBasicNode(atStatementList, $1, $3, NULL); }
  | function_def endls definitions { $$ = CreateBasicNode(atStatementList, $1, $3, NULL); }
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
  | id IQ FLOAT
    {
      struct AstNode *floatDefNode = CreateBasicNode(atVariableDef, $1, NULL, NULL); 
      floatDefNode->variableType = floating;
      floatDefNode->f = $3;

      $$ = floatDefNode;
    }
  | id ANNOUNCES STRING_CONSTANT
    {
      struct AstNode *stringDefNode = CreateBasicNode(atVariableDef, $1, NULL, NULL); 
      stringDefNode->variableType = characters;
      stringDefNode->stringLength = stringLength;
      stringDefNode->s = $3;

      $$ = stringDefNode;
    }
  ;
function_def:
  id FUNC_DEF_BEGIN_ARGS funcArgs FUNC_DEF_END_ARGS funcReturnType COLON endls function_body
    {
      struct AstNode *funcDefNode = CreateBasicNode(atFuncDef, $1, $3, $8); 
      funcDefNode->variableType = $5;

      $$ = funcDefNode;
    }
  ;
funcArgs:
  nonVoidFuncArgs { $$ = $1; }
  | void { $$ = $1; }
  ;
nonVoidFuncArgs:
  nonVoidFuncArg and nonVoidFuncArgs { $$ = CreateBasicNode(atElemList, $1, $3, NULL); }
  | nonVoidFuncArg { $$ = CreateBasicNode(atElemList, $1, NULL, NULL); }
  ;
nonVoidFuncArg:
  INT_FUNC_ARG id 
    {
      struct AstNode *funcArgNode = CreateBasicNode(atFuncArg, $2, NULL, NULL); 
      funcArgNode->variableType = integer;

      $$ = funcArgNode;
    }
  | FLOAT_FUNC_ARG id
    {
      struct AstNode *funcArgNode = CreateBasicNode(atFuncArg, $2, NULL, NULL); 
      funcArgNode->variableType = floating;

      $$ = funcArgNode;
    }
  | STRING_FUNC_ARG id
    {
      struct AstNode *funcArgNode = CreateBasicNode(atFuncArg, $2, NULL, NULL); 
      funcArgNode->variableType = characters;

      $$ = funcArgNode;
    }
  ;
funcReturnType:
  TYPE_FLOAT { $$ = floating; }
  | TYPE_INT { $$ = integer; }
  | TYPE_STRING { $$ = characters; }
  | TYPE_VOID { $$ = noType; }
  ;
function_body:
  body_lines END_FUNC { $$ = $1; }
  ;


body_lines:
  body_line body_lines { $$ = CreateBasicNode(atStatementList, $1, $2, NULL); }
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
assignment:
  exp and ASSIGN id { $$ = CreateBasicNode(atAssignment, $4, $1, NULL); }
  | nonVoidArg ASSIGN id { $$ = CreateBasicNode(atAssignment, $3, $1, NULL); }
  | id ASSIGN_FUNC id BEGIN_ARGS args 
    { 
      struct AstNode *funcCallNode = CreateBasicNode(atFuncCall, $1, $5, NULL);
      $$ = CreateBasicNode(atAssignment, $3, funcCallNode, NULL); 
    }
  ;
exp:
  exp ADD exp { $$ = CreateBasicNode(atAdd, $1, $3, NULL); }
  | exp MINUS exp { $$ = CreateBasicNode(atMinus, $1, $3, NULL); }
  | exp MULTIPLY exp { $$ = CreateBasicNode(atMultiply, $1, $3, NULL); }
  | exp DIVIDE exp { $$ = CreateBasicNode(atDivide, $1, $3, NULL); }
  | nonVoidArg { $$ = $1; }
  ;

test:
  BEGIN_TEST COLON endls test_comparisons_declarations BEGIN_BRANCH COLON endls test_branchs { $$ = CreateBasicNode(atTest, $4, $8, NULL); }
  ;
test_comparisons_declarations:
  HYPHEN test_comparison_declaration test_comparisons_declarations { $$ = CreateBasicNode(atStatementList, $2, $3, NULL); }
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
  HYPHEN test_if_branch END_CONDITION { $$ = CreateBasicNode(atStatementList, $2, NULL, NULL); }
  | HYPHEN test_if_branch test_elseIf_branchs END_CONDITION { $$ = CreateBasicNode(atStatementList, $2, $3, NULL); }
  | HYPHEN test_if_branch BEGIN_ELSE_BRANCH test_else_branch END_CONDITION
    { 
      struct AstNode *elseStatementNode = CreateBasicNode(atStatementList, $4, NULL, NULL);
      $$ = CreateBasicNode(atStatementList, $2, elseStatementNode, NULL); 
    }
  | HYPHEN test_if_branch test_elseIf_branchs BEGIN_ELSE_BRANCH test_else_branch END_CONDITION
    {
      struct AstNode *elseStatementNode = CreateBasicNode(atStatementList, $5, NULL, NULL);
      
      struct AstNode *temp;
      for (temp = $3; temp->child2!=NULL; temp = temp->child2); // Find the last element of the test_elseIf_branches node list
      temp->child2 = elseStatementNode; // Assign the else statement at the end of the list

      $$ = CreateBasicNode(atStatementList, $2, $3, NULL);
    }
  ;
test_if_branch:
  id BEGIN_CONDITION disjunctive_normal_form_comparisons BEGIN_ARGS args BEGIN_RETURN_VAR idOrVoid endls
    {
      struct AstNode *callFuncNode = CreateBasicNode(atFuncCall, $1, $5, NULL);
      struct AstNode *assignNode = CreateBasicNode(atAssignment, $7, callFuncNode, NULL);

      $$ = CreateBasicNode(atTestIfBranch, $3, assignNode, NULL);
    }
  ;
test_elseIf_branchs:
  HYPHEN test_elseIf_branch test_elseIf_branchs { $$ = CreateBasicNode(atStatementList, $2, $3, NULL); }
  | HYPHEN test_elseIf_branch { $$ = CreateBasicNode(atStatementList, $2, NULL, NULL); }
  ;
test_elseIf_branch:
  id BEGIN_CONDITION disjunctive_normal_form_comparisons BEGIN_ARGS args BEGIN_RETURN_VAR idOrVoid endls
    {
      struct AstNode *callFuncNode = CreateBasicNode(atFuncCall, $1, $5, NULL);
      struct AstNode *assignNode = CreateBasicNode(atAssignment, $7, callFuncNode, NULL);

      $$ = CreateBasicNode(atTestElseIfBranch, $3, assignNode, NULL);
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

      $$ = CreateBasicNode(atTestElseBranch, assignNode, NULL, NULL);
    }
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
func_call:
  id BEGIN_ARGS args { $$ = CreateBasicNode(atFuncCall, $1, $3, NULL); }
  ;
print:
  PRINT constant { $$ = CreateBasicNode(atPrint, $2, NULL, NULL); }
  | PRINT PRINT_INT id
    { 
      struct AstNode *printNode = CreateBasicNode(atPrint, $3, NULL, NULL);
      printNode->variableType = integer;

      $$ = printNode;
    }
  | PRINT PRINT_FLOAT id
    { 
      struct AstNode *printNode = CreateBasicNode(atPrint, $3, NULL, NULL);
      printNode->variableType = floating;

      $$ = printNode;
    }
  | PRINT PRINT_STRING id
    { 
      struct AstNode *printNode = CreateBasicNode(atPrint, $3, NULL, NULL);
      printNode->variableType = characters;

      $$ = printNode;
    }
  ;
return:
  nonVoidArg RETURN { $$ = CreateBasicNode(atReturn, $1, NULL, NULL); }
  ;


footer:
  END
  | END endls
  ;
endls:
  ENDL endls
  | ENDL
  ;
args:
  nonVoidArgs { $$ = $1; }
  | void { $$ = $1; }
  ;
nonVoidArgs:
  nonVoidArg and nonVoidArgs { $$ = CreateBasicNode(atElemList, $1, $3, NULL); }
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
  | STRING_CONSTANT
    {
      struct AstNode *stringNode = CreateBasicNode(atConstant, NULL, NULL, NULL);
      stringNode->variableType = characters;
      stringNode->s = $1;

      $$ = stringNode;
    }
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
  | COMMA
  ;
%%


void yyerror(struct AstNode** errorAstPtr, const char *s) {
  // Bison always reads one token ahead so we need to substract the last 2 tokens length to find the position of the problematic token
  int tokenPos = char_pos_in_line - previous_token_length - current_token_length;
  printf("Parse error on line %d:%d (%s) : %s\n", line_num, tokenPos, yytext, s);
  // might as well halt now:
  if (errorAstPtr!=NULL)
    FreeAST(*errorAstPtr);
  exit(1);
}