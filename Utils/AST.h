#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>

enum AstType 
{
    atRoot,
    atStatementList, atElemList, atLogicalOr, atLogicalAnd,
    atVariableDef, atFuncDef,
    atTest, atComparisonDeclaration, atComparisonId, atTestIfBranch, atTestElseBranch,
    atAssignment, atFuncCall, atWhileLoop, atCompare, atBreak, atReturn, atContinue,
    atId, atConstant, atVoid,
    atAdd, atMinus, atMultiply, atDivide, atPrint
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

    char* s; int i; float f;

    struct AstNode *child1;
    struct AstNode *child2;
    struct AstNode *child3;
};

struct AstNode* CreateBasicNode (enum AstType _type, struct AstNode* _child1, struct AstNode* _child2, struct AstNode* _child3);

struct AstNode* CreateWhileNode (enum ComparatorType _comparator, struct AstNode* _var1, struct AstNode* _var2, struct AstNode* _whileBranch);

void FreeAST (struct AstNode* ast);

#endif