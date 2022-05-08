#include <stdlib.h>
#include <stdio.h>
#include "Utils.h"

struct AstNode* CreateBasicNode (enum AstType _type, struct AstNode* _child1, struct AstNode* _child2, struct AstNode* _child3)
{
    struct AstNode* node = (struct AstNode*) malloc(sizeof (struct AstNode));
    if (node==NULL)
    {
        printf("Memory error\n");
        exit(2); //Closes all pointers, opened files, ... and exits the program with error 2
    }

    node->type = _type;
    node->child1 = _child1;
    node->child2 = _child2;
    node->child3 = _child3;

    return node;
}

struct AstNode* CreateWhileNode (enum ComparatorType _comparator, struct AstNode* _var1, struct AstNode* _var2, struct AstNode* _whileBranch)
{
    struct AstNode *conditionNode = CreateBasicNode(atCompare, _var1, _var2, NULL);
    conditionNode->comparator = _comparator;

    return CreateBasicNode(atWhileLoop, conditionNode, _whileBranch, NULL);
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

void AstToCode (struct AstNode* ast, FILE* file)
{
    switch (ast->type)
    {
        case atStatementList:
            AstToCode(ast->child1, file);
            AstToCode(ast->child2, file);

        break;
        case atLogicalAnd:
            
        break;
        case atLogicalOr:
            
        break;
    }
}