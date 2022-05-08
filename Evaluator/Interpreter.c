#include "../Utils/AST.h"
#include "../Parser-Bison/UF-C.tab.h"

extern int yyparse();
extern FILE *yyin;

void EvaluateAST (struct AstNode* ast)
{
    switch (ast->type)
    {
        case atRoot:
            EvaluateAST(ast->child1);
            EvaluateAST(ast->child2);
        break;
        case atStatementList:
            EvaluateAST(ast->child1);
            EvaluateAST(ast->child2);
        break;
        case atVariableDef:

        break;
        case atLogicalAnd:
            
        break;
        case atLogicalOr:
            
        break;
        default:
            printf("Error during evaluation : Node not valid\n");
            return;
        break;
    }
}

int main() {
    // open the input file
    FILE *myfile = fopen("in.ufc", "r");
    if (!myfile) 
    {
        printf("Cannot open in.ufc file\n");
        return -1;
    }

    // Set flex to read from it instead of defaulting to STDIN:
    yyin = myfile;

    struct AstNode* ast;

    // Parse through the input:
    int error = yyparse(&ast);
    if (error != 0)
    {
        printf("Error during parsing\n");
        FreeAST(ast);
        fclose(myfile);
        return error;
    }

    EvaluateAST(ast);

    FreeAST(ast);
    fclose(myfile);

    return 0;
}