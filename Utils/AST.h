#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>

enum AstType 
{
    atRoot,
    atStatementList, atLogicalOr, atLogicalAnd,
    atVariableDef, atFuncDef,
    atTest, atComparisonDeclaration, atComparisonId, atTestIfBranch, atTestElseIfBranch, atTestElseBranch,
    atAssignment, atFuncCall, atFuncCallArgList, atWhileLoop, atWhileCompare, atBreak, atReturn, atContinue,
    atId, atFuncDefArgsList, atFuncDefArg, atConstant, atVoid,
    atAdd, atMinus, atMultiply, atDivide, atPrint, atPrintEndl
};

enum ComparatorType
{
    gtr, str_gtr, neq, eq
};

enum VariableType
{
    integer, floating, characters, noType
};

struct AstNode
{
    enum AstType type;
    enum ComparatorType comparator;
    enum VariableType variableType;

    int stringLength;
    char* s; int i; float f;

    int lineNumInCode;

    struct AstNode *child1;
    struct AstNode *child2;
    struct AstNode *child3;
};

struct AstNode* CreateBasicNode (enum AstType _type, struct AstNode* _child1, struct AstNode* _child2, struct AstNode* _child3, const int lineNum);

struct AstNode* CreateWhileNode (enum ComparatorType _comparator, struct AstNode* _var1, struct AstNode* _var2, struct AstNode* _whileBranch, const int lineNum);

void FreeAST (struct AstNode* ast);

#endif