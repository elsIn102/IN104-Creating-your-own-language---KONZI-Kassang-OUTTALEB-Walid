#include "../Utils/ComparisonDictionnary.h"
#include "Translator.h"

void TranslatorError(char* error_msg)
{
    printf("Error from the translator : %s\n", error_msg);
}

void TranslateASTToFiles (struct AstNode* ast, FILE* currentFile, FILE* mainFile, FILE* funcFile, FILE* varFile, struct Comparisons_Dict* comparisonsDict)
{
    if (ast==NULL)
        return;
    
    switch (ast->type)
    {
        case atRoot:
            //Variables and functions definitions
            TranslateASTToFiles(ast->child1, varFile, mainFile, funcFile, varFile, comparisonsDict);
            //Main body of the code
            TranslateASTToFiles(ast->child2, mainFile, mainFile, funcFile, varFile, comparisonsDict);

            break;
        case atStatementList:
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);

            break;
        case atLogicalOr:
            fprintf(currentFile, "(");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "||");
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ")");
            break;
        case atLogicalAnd:
            fprintf(currentFile, "(");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "&&");
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ")");
            break;
        case atVariableDef:
            switch (ast->variableType) {
                case integer:
                    fprintf(varFile, "int ");
                    break;
                case floating:
                    fprintf(varFile, "float ");
                    break;
                case characters:
                    fprintf(varFile, "char* ");
                    break;
                default:
                    TranslatorError("Cannot define a variable with this type");
                    break;
            }
            TranslateASTToFiles(ast->child1, varFile, mainFile, funcFile, varFile, comparisonsDict);
            switch (ast->variableType) {
                case integer:
                    fprintf(varFile, " = %d;\n", ast->i);
                    break;
                case floating:
                    fprintf(varFile, " = %f;\n", ast->f);
                    break;
                case characters:
                    fprintf(varFile, " = malloc( 1 + %d);\n", ast->stringLength);
                    fprintf(varFile, "sprintf(");
                    TranslateASTToFiles(ast->child1, varFile, mainFile, funcFile, varFile, comparisonsDict);
                    fprintf(varFile, ", %s);\n", ast->s);
                    break;
                default:
                    TranslatorError("Cannot define a variable with this type");
                    break;
            }
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
                TranslatorError("Not a valid function return type");
                break;
            }
            TranslateASTToFiles(ast->child1, funcFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the name of the function
            fprintf(funcFile, "(");
            TranslateASTToFiles(ast->child2, funcFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the type and name of the arguments
            fprintf(funcFile, ") {\n");
            TranslateASTToFiles(ast->child3, funcFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the body of the function
            fprintf(funcFile, "}\n\n");

            break;        
        case atTest:
            {
                struct Comparisons_Dict* compDict;
                if (!CreateComparisonsDict(&compDict)) //If it failed to create the dictionnary
                {
                    TranslatorError("Unable to create the dictionnary");
                }
                else
                {
                    // Fills the dictionnary with all the comparisons
                    TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, compDict);
                    //Writes the if/else_if/else statements using the dicionnary
                    TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, compDict);

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
                    TranslatorError(msg);
                }
                else
                {
                    fprintf(currentFile, "(");
                    TranslateASTToFiles(comparison->value1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
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
                            TranslatorError("Not a valid comparator");
                            break;
                    }
                    TranslateASTToFiles(comparison->value2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                    fprintf(currentFile, ")");
                }
                break;
            }
        case atTestIfBranch:
            fprintf(currentFile, "if (");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the boolean test
            fprintf(currentFile, ") {\n");
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atTestElseIfBranch:
            fprintf(currentFile, "else if (");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the boolean test
            fprintf(currentFile, ") {\n");
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atTestElseBranch:
            fprintf(currentFile, "else {\n");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atAssignment:
            if (ast->child1->type==atVoid && ast->child2->type==atFuncCall) // then it's a call of a function without catching the return value
            {
                TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            }
            else
            {
                TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                fprintf(currentFile, " = ");
                TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                
                if (ast->child2->type==atId || ast->child2->type==atConstant) // Beacause atFuncCall already adds a ';' at the end
                    fprintf(currentFile, ";\n");
            }
            break;
        case atFuncCall:
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "(");
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atFuncCallArgList:
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            if (ast->child2 != NULL) {
                fprintf(currentFile, ", ");
                TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            }
            break;
        case atWhileLoop:
            fprintf(currentFile, "while(");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ") {\n");
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atWhileCompare:
            fprintf(currentFile, "(");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
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
                TranslatorError("Not a valid comparator");
                break;
            }
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ")");
            break;
        case atBreak:
            fprintf(currentFile, "break;\n");
            break;
        case atReturn:
            fprintf(currentFile, "return(");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atContinue:
            fprintf(currentFile, "continue;\n");
            break;
        case atId:
            fprintf(currentFile, "%s", ast->s);
            break;
        case atFuncDefArgsList:
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            if (ast->child2!=NULL) {
                fprintf(currentFile, ",");
                TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            }
            break;
        case atFuncDefArg:
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
                TranslatorError("Not a valid argument type");
                break;
            }
            // Writes the id of the argument
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);

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
                TranslatorError("Not a valid constant type");
                break;
            }
            break;
        case atVoid:
             // Called when specifing no argument to a function declaration
             // Nothing to write in C
            break;
        case atAdd:
            fprintf(currentFile, "(");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " + ");
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atMinus:
            fprintf(currentFile, "(");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " - ");
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atMultiply:
            fprintf(currentFile, "(");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " * ");
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atDivide:
            fprintf(currentFile, "(");
            TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " / ");
            TranslateASTToFiles(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atPrint:
            switch (ast->child1->type)
            {
                case atId:
                    switch(ast->variableType)
                    {
                        case integer:
                            fprintf(currentFile, "printf(\"%%d\",");
                        break;
                        case floating:
                            fprintf(currentFile, "printf(\"%%f\",");
                        break;
                        case characters:
                            fprintf(currentFile, "printf(\"%%s\",");
                        break;
                        default:
                            TranslatorError("Not a valid variable type to print");
                        break;
                    }

                    TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                    fprintf(currentFile, ");\n");
                    
                    break;
                case atConstant:
                    fprintf(currentFile, "printf(\"");
                    TranslateASTToFiles(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                    fprintf(currentFile, "\");\n");
                    break;
                default:
                    TranslatorError("The ring gal can't show this type of data");
                break;
            }
            break;
        case atPrintEndl:
            fprintf(currentFile, "printf(\"\\n\");\n");
            break;
        default:
            TranslatorError("Node not valid");
            return;
        break;
    }
}


void MergeFiles(FILE* outFile, FILE* mainFile, FILE* funcFile, FILE* varFile)
{
    // Temporary value to store the character read
    int tempChar;

    // Adding the includes
    fprintf(outFile, "#include <stdio.h>\n");
    fprintf(outFile, "#include <stdlib.h>\n");
    fprintf(outFile, "#include <string.h>\n\n");

    // Adding the variables definitions
    fseek(varFile, 0, SEEK_SET);
    while((tempChar = fgetc(varFile)) != EOF)
      fputc(tempChar, outFile);
    
    fprintf(outFile, "\n");
    // Adding the function definitions
    fseek(funcFile, 0, SEEK_SET);
    while((tempChar = fgetc(funcFile)) != EOF)
      fputc(tempChar, outFile);
    
    //Adding the main function
    fprintf(outFile, "\nint main(int argc, char* argv[]) {\n");

    // Adding the body of the code
    fseek(mainFile, 0, SEEK_SET);
    while((tempChar = fgetc(mainFile)) != EOF)
      fputc(tempChar, outFile);

    // Ending the main function
    fprintf(outFile, "\nreturn 0;\n}");
}

int TranslateAST (struct AstNode* ast, FILE* outFile)
{
    // We read the AST and add the functions definitions into a temporary funcFile, 
    // the variables definitions into varFile and the rest of the code into mainFile

    FILE* mainFile = fopen(MAIN_TEMP_NAME, "w+");
    if (mainFile==NULL)
    {
        printf("Can't create the temporary main file\n");
        FreeAST(ast);
        return 0;
    }

    FILE* funcFile = fopen(FUNC_TEMP_NAME, "w+");
    if (funcFile==NULL)
    {
        printf("Can't create the temporary function file\n");
        fclose(mainFile);
        FreeAST(ast);
        return 0;
    }

    FILE* varFile = fopen(VAR_TEMP_NAME, "w+");
    if (varFile==NULL)
    {
        printf("Can't create the temporary variable file\n");
        fclose(mainFile);
        fclose(funcFile);
        FreeAST(ast);
        return 0;
    }

    // Fill the mainFile, funcFile and varFile according to the AST
    TranslateASTToFiles(ast, NULL, mainFile, funcFile, varFile, NULL);

    // Merge these 3 files into the output file with the correct syntax
    MergeFiles(outFile, mainFile, funcFile, varFile);

    // Closing all files
    fclose(varFile);
    fclose(funcFile);
    fclose(mainFile);

    // Deleting all temporary files
    if (remove(VAR_TEMP_NAME))
        printf("Can't delete the variable temporary file\n");
    if (remove(FUNC_TEMP_NAME))
        printf("Can't delete the function temporary file\n");
    if (remove(MAIN_TEMP_NAME))
        printf("Can't delete the main temporary file\n");
    
   
    return 1;
    
}