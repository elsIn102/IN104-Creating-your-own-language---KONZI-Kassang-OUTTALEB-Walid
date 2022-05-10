#include "../Utils/AST.h"
#include "../Parser-Bison/UF-C.tab.h"

#include <stdlib.h>
#include <string.h>

extern int yyparse();
extern FILE *yyin;

void InterpreterError(char* error_msg)
{
    printf("Error from the interpreter : %s\n", error_msg);
}

void TranslateAST (struct AstNode* ast, FILE* outMainFile, FILE* inMainFile)
{
    switch (ast->type)
    {
        case atRoot:
            TranslateAST(ast->child1, outMainFile, inMainFile);
            TranslateAST(ast->child2, outMainFile, inMainFile);

            break;
        case atStatementList:
            TranslateAST(ast->child1, outMainFile, inMainFile);
            TranslateAST(ast->child2, outMainFile, inMainFile);

            break;
        case atElemList:
            TranslateAST(ast->child1, outMainFile, inMainFile);
            TranslateAST(ast->child2, outMainFile, inMainFile);

            break;
        case atLogicalOr:

            break;
        case atLogicalAnd:

            break;
        case atVariableDef:
            switch (ast->variableType)
            {
                case integer:
                    fprintf(inMainFile, "int ");
                    TranslateAST(ast->child1,outMainFile,inMainFile);
                    fprintf(inMainFile, " = %d",ast->i);


                    break;

                case floating:
                    fprintf(inMainFile, "float ");
                    TranslateAST(ast->child1,outMainFile,inMainFile);
                    fprintf(inMainFile, " = %f",ast->f);

                    break;

                case characters:

                    break;

                case noType:

                    break;
            }
            break;
        case atPrint:
            switch (ast->child1->type)
            {
                case atId:
                    //Need to know the variable type of the variable to print (add a new keyword ?)
                    switch(ast->child1->variableType)
                    {
                        case integer:
                            fprintf(inMainFile, "printf(\"%%d\n\",");
                        break;
                        case floating:
                            fprintf(inMainFile, "printf(\"%%f\n\",");
                        break;
                        case characters:
                            fprintf(inMainFile, "printf(\"%%s\n\",");
                        break;
                        default:
                            InterpreterError("Not a valid variable type to print\n");
                        break;
                    }

                    TranslateAST(ast->child1, outMainFile, inMainFile);
                    fprintf(inMainFile, ");\n");
                    
                    break;
                case atConstant:
                    switch(ast->variableType)
                    {
                        case integer:
                            fprintf(inMainFile, "printf(\"%d\");\n", ast->i);
                        break;
                        case floating:
                            fprintf(inMainFile, "printf(\"%f\");\n", ast->f);
                        break;
                        case characters:
                            fprintf(inMainFile, "printf(\"%s\");\n", ast->s);
                        break;
                        default:
                            InterpreterError("Not a valid variable type to print\n");
                        break;
                    }
                break;
                default:
                    InterpreterError("The ring gal can't show this type of data\n");
                break;
            }
            break;
        default:
            InterpreterError("Node not valid");
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
    //We don't need the input file anymore
    fclose(myfile);

    //Now we read the AST and add the function def,... in a temporary file 
    //and the main part of the function into another

    FILE* outMainFile = fopen("outMainTemp", "w");
    if (outMainFile==NULL)
    {
        printf("Can't create the temporary outMain file\n");
        FreeAST(ast);
        return -1;
    }

    FILE* inMainFile = fopen("inMainTemp", "w");
    if (inMainFile==NULL)
    {
        printf("Can't create the temporary inMain file\n");
        fclose(outMainFile);
        FreeAST(ast);
        return -1;
    }

    TranslateAST(ast, outMainFile, inMainFile);
    
    //Now we don't need the AST anymore
    FreeAST(ast);


    //Now we create the output file

    //Defining the name of the output file as inputFileName (removing the extension .ufc)
    char* outFileName;
    if ((outFileName = malloc (strlen(fileName) + 1)) == NULL)
    {
        printf("Can't create the output file name\n");
        fclose(outMainFile);
        fclose(inMainFile);
        return -1;
    }

    strcpy(outFileName, fileName);

    char* extension = strrchr(outFileName, '.');
    if (extension == NULL)
    {
        printf("Can't find any '.' in the name of the input file\n");
        free(outFileName);
        fclose(outMainFile);
        fclose(inMainFile);
        return -1;
    }
    *extension = '\0';

    //Now we can concatenate because ".ufc" is longer than ".c" so no memory problem
    strcat(outFileName, ".c");

    //From here on, outputFileName is the name of the file with the .c extension

    FILE* outFile = fopen(outFileName, "w");
    if (outFile==NULL)
    {
        printf("Can't create the output file\n");
        free(outFileName);
        fclose(outMainFile);
        fclose(inMainFile);
        return -1;
    }
    free(outFileName);

    //Now we merge the outMainFile content and the inMainFile content to create the final .c output file


    fclose(outFile);
    fclose(outMainFile);
    fclose(inMainFile);

    return 0;
}