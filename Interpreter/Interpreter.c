#include "Interpreter.h"

void InterpreterError(char* error_msg)
{
    printf("Error from the interpreter : %s\n", error_msg);
}

int GetValue(struct AstNode* id, struct ValueHolder* outVal)
{
    
}

void InterpreteAST (struct AstNode* ast)
{
    if (ast==NULL)
        return;
    
    switch (ast->type)
    {
        case atRoot:
            //Variables and functions definitions
            InterpreteAST(ast->child1);
            //Main body of the code
            InterpreteAST(ast->child2);

            break;
        case atStatementList:
            InterpreteAST(ast->child1);
            InterpreteAST(ast->child2);

            break;
        case atElemList:
            InterpreteAST(ast->child1);
            if (ast->child2!=NULL)
            {
                fprintf(currentFile, ", ");
                InterpreteAST(ast->child2);
            }

            break;
        case atLogicalOr:
            fprintf(currentFile, "(");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "||");
            InterpreteAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ")");
            break;
        case atLogicalAnd:
            fprintf(currentFile, "(");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "&&");
            InterpreteAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
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
            InterpreteAST(ast->child1, funcFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the name of the function
            fprintf(funcFile, "(");
            InterpreteAST(ast->child2, funcFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the type and name of the arguments
            fprintf(funcFile, ") {\n");
            InterpreteAST(ast->child3, funcFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the body of the function
            fprintf(funcFile, "}\n\n");

            break;
        case atVariableDef:
            /*switch (ast->variableType)
            {
                case integer:
                    fprintf(inMainFile, "int ");
                    InterpreteAST(ast->child1,outMainFile,inMainFile);
                    fprintf(inMainFile, " = %d",ast->i);


                    break;

                case floating:
                    fprintf(inMainFile, "float ");
                    InterpreteAST(ast->child1,outMainFile,inMainFile);
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
                    InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, compDict);
                    //Writes the if/else_if/else statements using the dicionnary
                    InterpreteAST(ast->child2, currentFile, mainFile, funcFile, varFile, compDict);

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
                    InterpreteAST(comparison->value1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
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
                    InterpreteAST(comparison->value2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                    fprintf(currentFile, ")");
                }
                break;
            }
        case atTestIfBranch:
            fprintf(currentFile, "if (");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the boolean test
            fprintf(currentFile, ") {\n");
            InterpreteAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atTestElseIfBranch:
            fprintf(currentFile, "else if (");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict); // Writes the boolean test
            fprintf(currentFile, ") {\n");
            InterpreteAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atTestElseBranch:
            fprintf(currentFile, "else {\n");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atAssignment:
            if (ast->child1->type==atVoid && ast->child2->type==atFuncCall) // then it's a call of a function without catching the return value
            {
                InterpreteAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            }
            else
            {
                InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                fprintf(currentFile, " = ");
                InterpreteAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                
                if (ast->child2->type==atId || ast->child2->type==atConstant) // Beacause atFuncCall already adds a ';' at the end
                    fprintf(currentFile, ";\n");
            }
            break;
        case atFuncCall:
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "(");
            InterpreteAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atWhileLoop:
            fprintf(currentFile, "while(");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ") {\n");
            InterpreteAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, "}\n");
            break;
        case atCompare:
            fprintf(currentFile, "(");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
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
            InterpreteAST(ast->child2, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ")");
            break;
        case atBreak:
            fprintf(currentFile, "break;\n");
            break;
        case atReturn:
            fprintf(currentFile, "return(");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
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
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);

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
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " + ");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atMinus:
            fprintf(currentFile, "(");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " - ");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atMultiply:
            fprintf(currentFile, "(");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " * ");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atDivide:
            fprintf(currentFile, "(");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, " / ");
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
            fprintf(currentFile, ");\n");
            break;
        case atPrint:
            switch (ast->child1->type)
            {
                case atId:
                    switch(ast->variableType)
                    {
                        case integer:
                            printf("%d", );
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

                    InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
                    fprintf(currentFile, ");\n");
                    
                    break;
                case atConstant:
                    fprintf(currentFile, "printf(\"");
                    InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);
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