#include <stdlib.h>
#include <string.h>

#include "Interpreter.h"
#include "../Utils/Hash.h"

void InterpreterError(char* error_msg)
{
    printf("Error from the interpreter : %s\n", error_msg);
}

void FreeValueHolder (struct ValueHolder* value) {
    if (value==NULL)
        return;

    if (value->s!=NULL)
        free(value->s);
    
    free(value);
}

int InterpreteAST (struct AstNode* ast, struct ValueHolder* outVal) 
{
    if (ast==NULL)
        return 1;
    
    switch (ast->type)
    {
        case atRoot:
        {
            //Variables and functions definitions
            int a = InterpreteAST(ast->child1, outVal);
            //Main body of the code
            int b = InterpreteAST(ast->child2, outVal);

            return a && b;
            break;
        }
        case atStatementList: ;
            int a = InterpreteAST(ast->child1, outVal);
            int b = InterpreteAST(ast->child2, outVal);

            return a && b;
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
                InterpreterError("Not a valid argument type");
                break;
            }
            // Writes the id of the argument
            InterpreteAST(ast->child1, currentFile, mainFile, funcFile, varFile, comparisonsDict);

            break;
        case atConstant:
            outVal->variableType = ast->variableType;

            switch (ast->variableType)
            {
            case integer:
                outVal->i = ast->i;
                break;
            case floating:
                outVal->f = ast->f;
                break;
            case characters:
                outVal->s = ast->s;
                break;            
            default:
                InterpreterError("Not a valid constant type");
                return 1;
                break;
            }

            return 0;
            break;
        case atVoid:
            outVal->variableType = noType;

            return 0;
            break;
        case atAdd:
        {
            struct ValueHolder *value1 = malloc(sizeof(struct ValueHolder));
            if (value1==NULL) {
                InterpreterError("Can't allocate memory for value1 in atAdd");
                return 1;
            }

            struct ValueHolder *value2 = malloc(sizeof(struct ValueHolder));
            if (value2==NULL) {
                FreeValueHolder(value1);
                InterpreterError("Can't allocate memory for value2 in atAdd");
                return 1;
            }

            // If managed to get the value of both members of the operation
            if(InterpreteAST(ast->child1, value1) && InterpreteAST(ast->child2, value2))
            {
                if(value2->variableType==integer)
                {
                    if (value1->variableType==integer)
                    {
                        outVal->variableType=integer;
                        outVal->i = value1->i + value2->i;
                    }
                    else if (value1->variableType==floating)
                    {
                        outVal->variableType=floating;
                        outVal->f = value1->f + value2->i;
                    }
                    else {
                        InterpreterError("Incompatible variable types");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 1;
                    }
                }
                else if(value2->variableType==floating)
                {
                    outVal->variableType=floating;

                    if (value1->variableType==integer) {
                        outVal->f = value1->i + value2->f;
                    }
                    else if (value1->variableType==floating) {
                        outVal->f = value1->f + value2->f;
                    }
                    else {
                        InterpreterError("Incompatible variable types");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 1;
                    }
                }
                else if(value2->variableType==characters && value2->variableType==characters)
                {
                    outVal->variableType=characters;

                    if (outVal->s!=NULL)
                        free(outVal->s);

                    outVal->s = malloc(1 + strlen(value1->s) + strlen(value2->s));
                    strcpy(outVal->s, value1->s);
                    strcat(outVal->s, value2->s);
                }
                else {
                    InterpreterError("Can't add these types of data");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 1;
                }
            }
            else {
                InterpreterError("Could not get the value of the two members of the addition");
                FreeValueHolder(value1);
                FreeValueHolder(value2);
                return 1;
            }
            
            FreeValueHolder(value1);
            FreeValueHolder(value2);

            return 0;

            break;
        }
        case atMinus:
        {
            struct ValueHolder *value1 = malloc(sizeof(struct ValueHolder));
            if (value1==NULL) {
                InterpreterError("Can't allocate memory for value1 in atMinus");
                return 1;
            }

            struct ValueHolder *value2 = malloc(sizeof(struct ValueHolder));
            if (value2==NULL) {
                FreeValueHolder(value1);
                InterpreterError("Can't allocate memory for value2 in atMinus");
                return 1;
            }

            // If managed to get the value of both members of the operation
            if(InterpreteAST(ast->child1, value1) && InterpreteAST(ast->child2, value2))
            {
                if(value2->variableType==integer)
                {
                    if (value1->variableType==integer)
                    {
                        outVal->variableType=integer;
                        outVal->i = value1->i - value2->i;
                    }
                    else if (value1->variableType==floating)
                    {
                        outVal->variableType=floating;
                        outVal->f = value1->f - value2->i;
                    }
                    else {
                        InterpreterError("Incompatible variable types");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 1;
                    }
                }
                else if(value2->variableType==floating)
                {
                    outVal->variableType=floating;

                    if (value1->variableType==integer) {
                        outVal->f = value1->i - value2->f;
                    }
                    else if (value1->variableType==floating) {
                        outVal->f = value1->f - value2->f;
                    }
                    else {
                        InterpreterError("Incompatible variable types");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 1;
                    }
                }
                else {
                    InterpreterError("Can't substract these types of data");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 1;
                }
            }
            else {
                InterpreterError("Could not get the value of the two members of the substraction");
                FreeValueHolder(value1);
                FreeValueHolder(value2);
                return 1;
            }
            
            FreeValueHolder(value1);
            FreeValueHolder(value2);

            return 0;
            break;
        }
        case atMultiply:
        {
            struct ValueHolder *value1 = malloc(sizeof(struct ValueHolder));
            if (value1==NULL) {
                InterpreterError("Can't allocate memory for value1 in atMultiply");
                return 1;
            }

            struct ValueHolder *value2 = malloc(sizeof(struct ValueHolder));
            if (value2==NULL) {
                FreeValueHolder(value1);
                InterpreterError("Can't allocate memory for value2 in atMultiply");
                return 1;
            }

            // If managed to get the value of both members of the operation
            if(InterpreteAST(ast->child1, value1) && InterpreteAST(ast->child2, value2))
            {
                if(value2->variableType==integer)
                {
                    if (value1->variableType==integer)
                    {
                        outVal->variableType=integer;
                        outVal->i = value1->i * value2->i;
                    }
                    else if (value1->variableType==floating)
                    {
                        outVal->variableType=floating;
                        outVal->f = value1->f * value2->i;
                    }
                    else {
                        InterpreterError("Incompatible variable types");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 1;
                    }
                }
                else if(value2->variableType==floating)
                {
                    outVal->variableType=floating;

                    if (value1->variableType==integer) {
                        outVal->f = value1->i * value2->f;
                    }
                    else if (value1->variableType==floating) {
                        outVal->f = value1->f * value2->f;
                    }
                    else {
                        InterpreterError("Incompatible variable types");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 1;
                    }
                }
                else {
                    InterpreterError("Can't multiply these types of data");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 1;
                }
            }
            else {
                InterpreterError("Could not get the value of the two members of the multiplication");
                FreeValueHolder(value1);
                FreeValueHolder(value2);
                return 1;
            }
            
            FreeValueHolder(value1);
            FreeValueHolder(value2);

            return 0;
            break;
        }
        case atDivide:
        {
            struct ValueHolder *value1 = malloc(sizeof(struct ValueHolder));
            if (value1==NULL) {
                InterpreterError("Can't allocate memory for value1 in atDivide");
                return 1;
            }

            struct ValueHolder *value2 = malloc(sizeof(struct ValueHolder));
            if (value2==NULL) {
                FreeValueHolder(value1);
                InterpreterError("Can't allocate memory for value2 in atDivide");
                return 1;
            }

            // If managed to get the value of both members of the operation
            if(InterpreteAST(ast->child1, value1) && InterpreteAST(ast->child2, value2))
            {
                if(value2->variableType==integer)
                {
                    if (value2->i==0) {
                        InterpreterError("Division by 0");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 1;
                    }
                    else if (value1->variableType==integer)
                    {
                        outVal->variableType=integer;
                        outVal->i = value1->i / value2->i;
                    }
                    else if (value1->variableType==floating)
                    {
                        outVal->variableType=floating;
                        outVal->f = value1->f / value2->i;
                    }
                    else {
                        InterpreterError("Incompatible variable types");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 1;
                    }
                }
                else if(value2->variableType==floating)
                {
                    outVal->variableType=floating;

                    if (value2->f==0) {
                        InterpreterError("Division by 0");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 1;
                    }
                    else if (value1->variableType==integer) {
                        outVal->f = value1->i / value2->f;
                    }
                    else if (value1->variableType==floating) {
                        outVal->f = value1->f / value2->f;
                    }
                    else {
                        InterpreterError("Incompatible variable types");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 1;
                    }
                }
                else {
                    InterpreterError("Can't divide these types of data");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 1;
                }
            }
            else {
                InterpreterError("Could not get the value of the two members of the division");
                FreeValueHolder(value1);
                FreeValueHolder(value2);
                return 1;
            }
            
            FreeValueHolder(value1);
            FreeValueHolder(value2);

            return 0;
            break;
        }
        case atPrint:
            switch (ast->child1->type)
            {
                case atId: 
                {
                    // Used to get the value of the variaiable in child1 (the variable to print)
                    struct ValueHolder *value1 = malloc(sizeof(struct ValueHolder));
                    if (value1==NULL) {
                        InterpreterError("Can't allocate memory for the ValueHolder in atPrint");
                        return 1;
                    }

                    switch(ast->variableType) {
                        case integer:
                            if(InterpreteAST(ast->child1, value1)) // If managed to retrieve the value
                                printf("%d", value1->i );

                            break;
                        case floating:
                            if(InterpreteAST(ast->child1, value1))
                                printf("%f", value1->f);

                            break;
                        case characters:
                            if(InterpreteAST(ast->child1, value1))
                                printf("%s", value1->s);

                            break;
                        default:
                            InterpreterError("Not a valid variable type to print");
                            FreeValueHolder(value1);
                            return 1;
                        break;
                    }

                    FreeValueHolder(value1);

                    break;
                }
                case atConstant:
                    switch(ast->child1->variableType) {
                        case integer:
                            printf("%d\n", ast->child1->i);
                        break;
                        case floating:
                            printf("%f\n", ast->child1->f);
                        break;
                        case characters:
                            printf("%s\n", ast->child1->s);
                        break;
                        default:
                            InterpreterError("Not a valid constant type to print");
                            return 1;
                        break;
                    }

                    break;
                default:
                    InterpreterError("The ring gal can't show this type of data");
                    return 1;
                break;
            }
            break;
        default:
            InterpreterError("Node not valid");
            return 1;
        break;
    }
}