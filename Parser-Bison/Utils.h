#ifndef __UTILS_H__
#define __UTILS_H__

enum AstType 
{
    atList, atLogicalOr, atLogicalAnd,
    atIntDef, atFloatDef, atStringDef, atFuncDef,
    atTest, atComparisonDeclaration, atComparisonId, atTestIfBranch, atTestElseBranch,
    atAssignment, atFuncCall, atWhileLoop, atCompare, atBreak, atReturn, atContinue,
    atId, atInt, atFloat, atVoid,
    atAdd, atMinus, atMultiply, atDivide, atPrint
};

enum ComparatorType
{
    gtr, str_gtr, neq, eq
};

struct AstNode
{
    enum AstType type;
    enum ComparatorType comparator;

    char* s; int i; float f;

    struct AstNode *child1;
    struct AstNode *child2;
    struct AstNode *child3;
};

struct AstNode* CreateBasicNode (enum AstType _type, struct AstNode* _child1, struct AstNode* _child2, struct AstNode* _child3);

struct AstNode* CreateWhileNode (enum ComparatorType _comparator, struct AstNode* _var1, struct AstNode* _var2, struct AstNode* _whileBranch);

void FreeAST (struct AstNode* ast);

#endif