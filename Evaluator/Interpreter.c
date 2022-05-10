#include "../Utils/AST.h"
#include "../Parser-Bison/UF-C.tab.h"

extern int yyparse();
extern FILE *yyin;

void TranslateAST (struct AstNode* ast)
{
    switch (ast->type)
    {
        case atRoot:
            TranslateAST(ast->child1);
            TranslateAST(ast->child2);
        break;
        case atStatementList:
            TranslateAST(ast->child1);
            TranslateAST(ast->child2);
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

int main(int argc, char* argv[]) 
{
    if (argc == 1)
    {
        printf("Error : Not enough arguments\n");
        return 1;
    }
    else if (argc > 2)
    {
        printf("Error : Too many arguments\n");
        return 1;
    }

    char* fileName = argv[1];

    // open the input file
    FILE *myfile = fopen(fileName, "r");
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

    //EvaluateAST(ast); 

    FreeAST(ast);
    fclose(myfile);

    return 0;
}