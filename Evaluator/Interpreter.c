#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../Utils/AST.h"
#include "../Parser-Bison/UF-C.tab.h"
#include "../Utils/Dictionnary.h"

#define VAR_TEMP_NAME "_varTemp"
#define FUNC_TEMP_NAME "_funcTemp"
#define MAIN_TEMP_NAME "_mainTemp"

extern int yyparse();
extern FILE *yyin;

void InterpreterError(char* error_msg)
{
    printf("Error from the interpreter : %s\n", error_msg);
}

void TranslateAST (struct AstNode* ast, FILE* currentFile, FILE* mainFile, FILE* funcFile, FILE* varFile, struct Comparisons_Dict* comparisonsDict)
{
    if (ast==NULL)
        return;
    
    switch (ast->type)
    {
        case atRoot:
            //Variables and functions definitions
            TranslateAST(ast->child1, varFile, mainFile, funcFile, varFile, comparisonsDict);
            //Main body of the code
            TranslateAST(ast->child2, mainFile, mainFile, funcFile, varFile, comparisonsDict);

            break;
        case atStatementList:
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);

            break;
        case atElemList:
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            if (ast->child2!=NULL)
            {
                fprintf(currentFile, ", ");
                TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            }

            break;
        case atLogicalOr:
            fprintf(currentFile, "(");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "||");
            TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ")");
            break;
        case atLogicalAnd:
            fprintf(currentFile, "(");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "&&");
            TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ")");
            break;
        case atFuncDef:
            switch (ast->variableType)
            {
            case integer:
                fprintf(funcFile, "int ");
                break;
            case floating:
                fprintf(funcFile, "float ");
                break;
            case characters:
                fprintf(funcFile, "char* ");
                break;      
            case noType:
                fprintf(funcFile, "void ");
                break;
            default:
                InterpreterError("Not a valid function return type");
                break;
            }
            TranslateAST(ast->child1, funcFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the name of the function
            fprintf(funcFile, "(");
            TranslateAST(ast->child2, funcFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the type and name of the arguments
            fprintf(funcFile, ") {\n");
            TranslateAST(ast->child3, funcFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the body of the function
            fprintf(funcFile, "}\n\n");

            break;
        case atVariableDef:
            /*switch (ast->variableType)
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
            }*/
            break;
        
        case atTest:
            {
                struct Comparisons_Dict* compDict;
                if (!CreateComparisonsDict(&compDict)) //If it failed to create the dictionnary
                {
                    InterpreterError("Unable to create the dictionnary");
                }
                else
                {
                    // Fills the dictionnary with all the comparisons
                    TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, compDict);
                    //Writes the if/else_if/else statements using the dicionnary
                    TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, compDict);

                    FreeComparisonsDict(compDict);
                }
            }
            break;
        case atComparisonDeclaration:
            Add_ComparisonsDict(&comparisonsDict, ast->i, ast->comparator, ast->child1, ast->child2);
            break;
        case atComparisonId: // Writes the comparison of the two values corresponding to the ID
            {
                struct ComparisonValue *comparison;
                if (!TryFind_ComparisonsDict(comparisonsDict, ast->i, &comparison))
                {
                    char* msg;
                    sprintf(msg, "Unable to find the comparison (match %d) in this dictionnary", ast->i);
                    InterpreterError(msg);
                }
                else
                {
                    fprintf(currentFile, "(");
                    TranslateAST(comparison->value1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                    switch (ast->comparator)
                    {
                        case gtr:
                            fprintf(currentFile, ">=");
                            break;
                        case str_gtr:
                            fprintf(currentFile, ">");
                            break;
                        case neq:
                            fprintf(currentFile, "!=");
                            break;
                        case eq:
                            fprintf(currentFile, "==");
                            break;
                        default:
                            InterpreterError("Not a valid comparator");
                            break;
                    }
                    TranslateAST(comparison->value2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                    fprintf(currentFile, ")");
                }
                break;
            }
        case atTestIfBranch:
            fprintf(currentFile, "if (");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the boolean test
            fprintf(currentFile, ") {\n");
            TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atTestElseIfBranch:
            fprintf(currentFile, "else if (");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the boolean test
            fprintf(currentFile, ") {\n");
            TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atTestElseBranch:
            fprintf(currentFile, "else {\n");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atAssignment:
            if (ast->child1->type==atVoid && ast->child2->type==atFuncCall) // then it's a call of a function without catching the return value
            {
                TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            }
            else
            {
                TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                fprintf(currentFile, " = ");
                TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                
                if (ast->child2->type==atId || ast->child2->type==atConstant) // Beacause atFuncCall already adds a ';' at the end
                    fprintf(currentFile, ";\n");
            }
            break;
        case atFuncCall:
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "(");
            TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atWhileLoop:
            fprintf(currentFile, "while(");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ") {\n");
            TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atCompare:
            fprintf(currentFile, "(");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            switch (ast->comparator)
            {
            case gtr:
                fprintf(currentFile, ">=");
                break;
            case str_gtr:
                fprintf(currentFile, ">");
                break;
            case neq:
                fprintf(currentFile, "!=");
                break;
            case eq:
                fprintf(currentFile, "==");
                break;
            default:
                InterpreterError("Not a valid comparator");
                break;
            }
            TranslateAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ")");
            break;
        case atBreak:
            fprintf(currentFile, "break;\n");
            break;
        case atReturn:
            fprintf(currentFile, "return(");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atContinue:
            fprintf(currentFile, "continue;\n");
            break;
        case atId:
            fprintf(currentFile, "%s", ast->s);
            break;
        case atFuncArg:
            switch (ast->variableType)
            {
            case integer:
                fprintf(currentFile, "int ");
                break;
            case floating:
                fprintf(currentFile, "float ");
                break;
            case characters:
                fprintf(currentFile, "char* ");
                break;            
            default:
                InterpreterError("Not a valid argument type");
                break;
            }
            // Writes the id of the argument
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);

            break;
        case atConstant:
            switch (ast->variableType)
            {
            case integer:
                fprintf(currentFile, "%d", ast->i);
                break;
            case floating:
                fprintf(currentFile, "%f", ast->f);
                break;
            case characters:
                fprintf(currentFile, "%s", ast->s);
                break;            
            default:
                InterpreterError("Not a valid constant type");
                break;
            }
            break;
        case atVoid:
             // Called when specifing no argument to a function declaration
             // Nothing to write in C
            break;
        case atAdd:
            fprintf(currentFile, "(");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " + ");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atMinus:
            fprintf(currentFile, "(");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " - ");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atMultiply:
            fprintf(currentFile, "(");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " * ");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atDivide:
            fprintf(currentFile, "(");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " / ");
            TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atPrint:
            switch (ast->child1->type)
            {
                case atId:
                    switch(ast->variableType)
                    {
                        case integer:
                            fprintf(currentFile, "printf(\"%%d\n\",");
                        break;
                        case floating:
                            fprintf(currentFile, "printf(\"%%f\\n\",");
                        break;
                        case characters:
                            fprintf(currentFile, "printf(\"%%s\n\",");
                        break;
                        default:
                            InterpreterError("Not a valid variable type to print");
                        break;
                    }

                    TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                    fprintf(currentFile, ");\n");
                    
                    break;
                case atConstant:
                    fprintf(currentFile, "printf(\"");
                    TranslateAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                    fprintf(currentFile, "\");\n");
                    break;
                default:
                    InterpreterError("The ring gal can't show this type of data");
                break;
            }
            break;
        default:
            InterpreterError("Node not valid");
            return;
        break;
    }
}

void MergeFiles(FILE* outFile, FILE* mainFile, FILE* funcFile, FILE* varFile)
{
    int a;

    // Adding the includes
    //fprintf(outFile, "");

    // Adding the variables definitions
    fseek(varFile, 0, SEEK_SET);
    while((a = fgetc(varFile)) != EOF)
      fputc(a, outFile);
    
    // Adding the function definitions
    fseek(funcFile, 0, SEEK_SET);
    while((a = fgetc(funcFile)) != EOF)
      fputc(a, outFile);
    
    //Adding the main function
    fprintf(outFile, "\nint main(int argc, char* argv[]) {\n");

    // Adding the body of the code
    fseek(mainFile, 0, SEEK_SET);
    while((a = fgetc(mainFile)) != EOF)
      fputc(a, outFile);

    // Ending the main function
    fprintf(outFile, "\nreturn 0;\n}");
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


    /**************** Opening the code file ***********************/

    char* fileName = argv[1];

    // open the input file
    FILE *myfile = fopen(fileName, "r");
    if (!myfile) 
    {
        printf("Cannot open in.ufc file\n");
        return 1;
    }


    /******************* Creating the AST ***********************/

    struct AstNode* ast;

    // Set flex to read from it instead of defaulting to STDIN:
    yyin = myfile;
    // Not zero to trace Bison states (debug)
    extern int yydebug;
    yydebug = 0;

    // Parse through the input and get the AST
    int error = yyparse(&ast);
    if (error != 0)
    {
        printf("Error during parsing\n");
        FreeAST(ast);
        fclose(myfile);
        return error;
    }
    // We don't need the input file anymore
    fclose(myfile);



    /***************** Interpreting the AST ******************/


    // Now we read the AST and add the functions definitions into a temporary funcFile, 
    // the variables definitions into varFile and the rest of the code into mainFile

    FILE* mainFile = fopen(MAIN_TEMP_NAME, "w+");
    if (mainFile==NULL)
    {
        printf("Can't create the temporary main file\n");
        FreeAST(ast);
        return 1;
    }

    FILE* funcFile = fopen(FUNC_TEMP_NAME, "w+");
    if (funcFile==NULL)
    {
        printf("Can't create the temporary function file\n");
        fclose(mainFile);
        FreeAST(ast);
        return 1;
    }

    FILE* varFile = fopen(VAR_TEMP_NAME, "w+");
    if (varFile==NULL)
    {
        printf("Can't create the temporary variable file\n");
        fclose(mainFile);
        fclose(funcFile);
        FreeAST(ast);
        return 1;
    }

    TranslateAST(ast, NULL, mainFile, funcFile, varFile, NULL);
    
    // We don't need the AST anymore
    FreeAST(ast);


    /**************** Creating the output '.c' file ********************/

    // Getting the name of the output file (removing the extension .ufc)
    char* outFileName;
    if ((outFileName = malloc (strlen(fileName) + 1)) == NULL)
    {
        printf("Can't create the output file name\n");
        fclose(mainFile);
        fclose(funcFile);
        fclose(varFile);
        return 1;
    }
    // Copy the name of the input file in outFileName
    strcpy(outFileName, fileName);

    // Find the position in memory of the '.'
    char* extension = strrchr(outFileName, '.');
    if (extension == NULL)
    {
        printf("Can't find any '.' in the name of the input file\n");
        free(outFileName);
        fclose(mainFile);
        fclose(funcFile);
        fclose(varFile);
        return 1;
    }
    // Assign 'end of char*' character at that position
    *extension = '\0';

    // Now we can concatenate because ".ufc" is longer than ".c" so no memory problem
    strcat(outFileName, ".c");


    // From here on, outputFileName is the name of the file with the .c extension
    // Creating the output file
    FILE* outFile = fopen(outFileName, "w");
    if (outFile==NULL)
    {
        printf("Can't create the output file\n");
        free(outFileName);
        fclose(mainFile);
        fclose(funcFile);
        fclose(varFile);
        return 1;
    }
    // Name of the output file useless now so we can free it
    free(outFileName);


    /******************* Writing the output file ***********************/

    // Now we merge the varFile, funcFile and mainFile to create the final .c output file
    MergeFiles(outFile, mainFile, funcFile, varFile);


    /*********** Closing all opened files and deleting them ***********/

    // Closing all files
    fclose(varFile);
    fclose(funcFile);
    fclose(mainFile);
    fclose(outFile);

    // Deleting all temporary files
    /*
    if (!remove(VAR_TEMP_NAME))
        printf("Can't delete the variable temporary file\n");
    if (!remove(FUNC_TEMP_NAME))
        printf("Can't delete the variable temporary file\n");
    if (!remove(MAIN_TEMP_NAME))
        printf("Can't delete the variable temporary file\n");
    */

    return 0;
}