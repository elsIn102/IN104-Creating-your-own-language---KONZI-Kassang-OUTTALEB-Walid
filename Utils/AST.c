#include <stdlib.h>
#include <stdio.h>
#include "AST.h"

struct AstNode* CreateBasicNode (enum AstType _type, struct AstNode* _child1, struct AstNode* _child2, struct AstNode* _child3, const int lineNum)
{
    struct AstNode* node = (struct AstNode*) malloc(sizeof (struct AstNode));
    if (node==NULL)
    {
        printf("Memory error : cannot allocate memory to a new AST node\n");
        exit(1); //Closes all pointers, opened files, ... and exits the program with error 2
    }

    node->type = _type;
    node->child1 = _child1;
    node->child2 = _child2;
    node->child3 = _child3;

    node->lineNumInCode = lineNum;

    return node;
}

struct AstNode* CreateWhileNode (enum ComparatorType _comparator, struct AstNode* _var1, struct AstNode* _var2, struct AstNode* _whileBranch, const int lineNum)
{
    struct AstNode *conditionNode = CreateBasicNode(atWhileCompare, _var1, _var2, NULL, lineNum);
    conditionNode->comparator = _comparator;

    return CreateBasicNode(atWhileLoop, conditionNode, _whileBranch, NULL, lineNum);
}

void FreeAST (struct AstNode* ast)
{
    if (ast == NULL)
        return;
    
    FreeAST(ast->child1);
    FreeAST(ast->child2);
    FreeAST(ast->child3);

    if (ast->s != NULL)
        free(ast->s);
    
    free(ast);
}