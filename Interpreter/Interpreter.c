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

int InterpreteAST (struct AstNode* ast, struct ValueHolder* outVal, struct HashStruct* globalSymbolTable, struct HashStruct* localSymbolTable, struct HashStruct* argsTable, struct ArgList* listOfArgs, struct valueHolder* returnValue) 
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
        case atStatementList:
        {
            int a = InterpreteAST(ast->child1, outVal);

            if (ast->child1->type == atReturn) // Since every call of return can only be in a function (not a loop, nor an if), this effectively ends the flow of the function when a return is met
                return a;
            
            int b = InterpreteAST(ast->child2, outVal);

            return a && b;
            break;
        }
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
            struct ValueHolder* funcIdHolder = malloc(sizeof(struct ValueHolder));
            if (funcIdHolder==NULL) {
                InterpreterError("Can't allocate memory for funcIdHolder in atFuncDef");
                return 1;
            }

            if (InterpreteAST(ast->child1, funcIdHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL)) // get the name of the function
            {
                if (funcIdHolder->variableType != characters)
                {
                    InterpreterError("Not a valid function Id");
                    FreeValueHolder(funcIdHolder);
                    return 1;
                }

                if (TryFind_Hashtable(globalSymbolTable, funcIdHolder->s, NULL)) { //If a function or a variable with this name has already been defined
                    InterpreterError("A function or a variable with this name already exists");
                    FreeValueHolder(funcIdHolder);
                    return 1;
                }

                struct HashStruct* _argsTable = NULL;
                struct ArgList* _listOfArgs = NULL;

                if (ast->child2->type != atVoid) // If the function requires arguments, fill the list and table
                {
                    _argsTable = malloc(sizeof(struct HashStruct));
                    if (funcIdHolder==NULL) {
                        InterpreterError("Can't allocate memory for _argsTable in atFuncDef");
                        FreeValueHolder(funcIdHolder);
                        return 1;
                    }

                    _listOfArgs = malloc(sizeof(struct ArgList));
                    if (_listOfArgs==NULL) {
                        InterpreterError("Can't allocate memory for _listOfArgs in atFuncDef");
                        FreeValueHolder(funcIdHolder);
                        Free_Hashtable(_argsTable);
                        return 1;
                    }

                    if (!InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, _argsTable, _listOfArgs, NULL)) // If all arguments of the function has not been defined successfully
                    {
                        InterpreterError("Error while defining the arguments during the function definition");
                        FreeValueHolder(funcIdHolder);
                        Free_Hashtable(_argsTable);
                        FreeArgList(_listOfArgs);
                        return 1;
                    }
                }

                // Create the structure of the function that will be stored in the global symbol table
                struct VariableStruct* funcStruct = malloc(sizeof(struct VariableStruct));
                if (funcStruct==NULL) {
                    InterpreterError("Can't allocate memory for funcStruct in atFuncDef");
                    FreeValueHolder(funcIdHolder);
                    Free_Hashtable(_argsTable);
                    FreeArgList(_listOfArgs);
                    return 1;
                }

                // Set the parameters of the function
                funcStruct->s = malloc(1 + strlen(funcIdHolder->s));
                strcpy(funcStruct->s, funcIdHolder->s);

                funcStruct->argumentsTable = _argsTable;
                funcStruct->argumentsList = _listOfArgs;
                funcStruct->type = ast->variableType; //Return type of the function
                funcStruct->functionBody = ast->child3;

                // Add the function to the global symbol table
                if (!Add_Hashtable(globalSymbolTable, funcIdHolder->s, funcStruct)) {
                    InterpreterError("Error while adding the function to the global symbol table");
                    FreeValueHolder(funcIdHolder);
                    Free_Hashtable(_argsTable);
                    FreeArgList(_listOfArgs);
                    FreeVariableStruct(funcStruct);
                    return 1;
                }

            }
            else { // If couldn't get the name of the function
                InterpreterError("Can't get the Id of the function in atFuncDef");
                FreeValueHolder(funcIdHolder);
                return 1;
            }

            return 0;
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
                InterpreteAST(ast->child2, valToAssign, globalSymbolTable, localSymbolTable, NULL, NULL);
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
            struct ValueHolder* funcIdHolder = malloc(sizeof(struct ValueHolder));
            if (funcIdHolder==NULL) {
                InterpreterError("Can't allocate memory for funcIdHolder in atFuncCall");
                return 1;
            }

            if (InterpreteAST(ast->child1, funcIdHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL)) //If manages to retrieve the id of the function
            {
                if (funcIdHolder->variableType != characters)
                {
                    InterpreterError("Not a valid function Id");
                    FreeValueHolder(funcIdHolder);
                    return 1;
                }

                struct VariableStruct* funcVarStruct;// = malloc(sizeof(struct VariableStruct));
                /*if (funcVarStruct==NULL) {
                    InterpreterError("Can't allocate memory for funcVarStruct in atFuncCall");
                    FreeValueHolder(funcIdHolder);
                    return 1;
                }*/

                if (TryFind_Hashtable (globalSymbolTable, funcIdHolder->s, &funcVarStruct)) // If the function was defined
                {
                    // Fill the table of local arguments (argsTable) with the values used to call the function
                    if (ast->child2->type != atVoid)
                    {
                        if (!InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, funcVarStruct->argumentsTable, funcVarStruct->argumentsList, NULL))
                        {
                            InterpreterError("Could not assign all the arguments for the call of the function");
                            FreeValueHolder(funcIdHolder);
                            return 1;
                        }
                    }

                    // Call the function and return the output value
                    if (!InterpreteAST(funcVarStruct->functionBody, NULL, globalSymbolTable, funcVarStruct->argumentsTable, NULL, NULL, outVal)) { // If an error occurred while calling the function
                        InterpreterError("Error while calling the function");
                        FreeValueHolder(funcIdHolder);
                        return 1;
                    }
                }
                else {
                    InterpreterError("Call of an undefined function");
                    FreeValueHolder(funcIdHolder);
                    return 1;
                }
            }
            else {
                InterpreterError("Can't get the id of the function in atFuncCall");
                FreeValueHolder(funcIdHolder);
                return 1;
            }

            FreeValueHolder(funcIdHolder);

            return 0;
            break;
        case atFuncCallArgList: // Get through the listOfArgs and assign their value to the localSymbolTable
            // Make sure there is at least one more argument
            if (listOfArgs==NULL)
            {
                InterpreterError("Not enought arguments in the function call");
                return 1;
            }

            // Get a pointer to the argument wich value we need to assign in the local hashtable
            struct VariableStruct* foundArg;// = malloc(sizeof(struct VariableStruct));
            /*if (foundArg==NULL) {
                InterpreterError("Can't allocate memory for foundArg in atFuncCallArgList");
                return 1;
            }*/

            if (TryFind_Hashtable(argsTable, listOfArgs->id, &foundArg))
            {
                // Get the value to assgin to the argument
                struct ValueHolder* argVal = malloc(sizeof(struct ValueHolder));
                if (argVal==NULL) {
                    InterpreterError("Can't allocate memory for argVal in atFuncCallArgList");
                    return 1;
                }

                if (InterpreteAST(ast->child1, argVal, globalSymbolTable, localSymbolTable, NULL, NULL, NULL))
                {
                    if (argVal->variableType != foundArg->type)
                    {
                        InterpreterError("The type of the argument doesn't match the type defined in the function");
                        FreeValueHolder(argVal);
                        return 1;
                    }

                    switch (argVal->variableType)
                    {
                        case integer:
                            foundArg->i = argVal->i;
                            break;
                        case floating:
                            foundArg->f = argVal->f;
                            break;
                        case characters:
                            if (foundArg->s != NULL)
                                free(foundArg->s);

                            foundArg->s = malloc(1 + strlen(argVal->s));
                            strcpy(foundArg->s, argVal->s);
                            break;            
                        default:
                            InterpreterError("Not a valid argument type");
                            FreeValueHolder(argVal);
                            return 1;
                            break;
                    }
                }
                else {
                    InterpreterError("Could not get the value of the argument");
                    FreeValueHolder(argVal);
                    return 1;
                }
            }
            else {
                InterpreterError("Failed to find the function argument in the hashtable : atFuncDef might not define the argumentsTable and the argumentsList properly");
                return 1;
            }

            // Get the rest of the arguments to be added to the localSymbolTable
            if (ast->child2!=NULL)
                return InterpreteAST(ast->child1, NULL, globalSymbolTable, localSymbolTable, argsTable, listOfArgs->next, NULL);

            return 0;

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
        case atReturn: // Assign the return value. The end of the flow is done in atStatementList since it is the only place where a return can exist
            if (returnValue==NULL) {
                InterpreterError("Return called outside of a function");
                return 1;
            }

            if(!InterpreteAST(ast->child1, returnValue, globalSymbolTable, localSymbolTable, NULL, NULL, returnValue)) {
                InterpreterError("Could not get the value to return");
                return 1;
            }

            return 0;

            break;
        case atContinue:
            fprintf(currentFile, "continue;\n");
            break;
        case atId:
            fprintf(currentFile, "%s", ast->s);
            break;
        case atFuncDefArgsList: // Add the arguments to the hashtable argsTable and return the list of arguments in listOfArgs
            // Add the argument to argsTable and fill the value of the head of listOfArgs
            if (InterpreteAST(ast->child1, NULL, globalSymbolTable, localSymbolTable, argsTable, listOfArgs, NULL))
            {
                if (ast->child2!=NULL) { // if there is another argument to define
                    // Create the next element in the list of arguments
                    struct ArgList* newArg = malloc(sizeof(struct ArgList));
                    if (newArg==NULL) {
                        InterpreterError("Can't allocate memory for newArg in atFuncDefArgsList");
                        return 1;
                    }

                    // Fill the tail of the list with the arguments
                    if (!InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, argsTable, newArg, NULL)) {
                        InterpreterError("Can't allocate memory for newArg in atFuncDefArgsList");
                        FreeArgList(newArg);
                        return 1;
                    }
                    // The head points towards the tail
                    listOfArgs->next = newArg;
                }
            }
            else {
                InterpreterError("Error while defining the argument of the function");
                return 1;
            }

            return 0;
            break;
        case atFuncDefArg: // Add the argument to argsTable and set it in listOfArgs
            struct ValueHolder* argId = malloc(sizeof(struct ValueHolder));
            if (argId==NULL) {
                InterpreterError("Can't allocate memory for argId in atFuncDefArg");
                return 1;
            }

            // Get the Id of the argument
            if (InterpreteAST(ast->child1, argId, globalSymbolTable, localSymbolTable, NULL, NULL, NULL))
            {
                // Make sure what we got is really an id
                if (argId->variableType!=characters) {
                    InterpreterError("Error while building the AST : the first child of atFuncDefArg is not an atId");
                    FreeValueHolder(argId);
                    return 1;
                }

                // Add the argument to the argsTable of the function
                struct VariableStruct* argValue = malloc(sizeof(struct VariableStruct));
                if (argValue==NULL) {
                    InterpreterError("Can't allocate memory for argValue in atFuncDefArg");
                    FreeValueHolder(argId);
                    return 1;
                }

                argValue->id = argId->s;
                argValue->type = ast->variableType;

                switch (Add_Hashtable(argsTable, argId, argValue))
                {
                    case 2:
                        InterpreterError("An argument with this name has already been defined");
                        FreeValueHolder(argId);
                        FreeVariableStruct(argValue);
                        return 1;
                        break;
                    case 1:
                        InterpreterError("Could not add the argument to the hashtable");
                        FreeValueHolder(argId);
                        FreeVariableStruct(argValue);
                        return 1;
                        break;
                    case 0:
                        // Set the argument from listOfArgs
                        if (listOfArgs->id!=NULL)
                            free(listOfArgs->id);
                        
                        listOfArgs->id = malloc(1 + strlen(argId->s));
                        strcpy(listOfArgs->id, argId->s);

                        return 0;

                        break;
                    default:
                        InterpreterError("Unknown error while trying to add the argument to the hashtable");
                        FreeValueHolder(argId);
                        FreeVariableStruct(argValue);
                        return 1;
                        break;
                }
            }
            else {
                InterpreterError("Could not get the id of the argument");
                FreeValueHolder(argId);
                return 1;
            }
            
            break;
        case atConstant:
            outVal->variableType = ast->variableType;

            switch (ast->variableType) {
                case integer:
                    outVal->i = ast->i;
                    break;
                case floating:
                    outVal->f = ast->f;
                    break;
                case characters:
                    outVal->s = ast->s; // No need to copy since the value will be used before we free the AST
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
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL))
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
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL))
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
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL))
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
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL))
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
                        if (value1->i % value2->i == 0)
                        {
                            outVal->variableType=integer;
                            outVal->i = value1->i / value2->i;
                        }
                        else
                        {
                            outVal->variableType=floating;
                            outVal->f = value1->i / value2->i;
                        }
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
            // Used to get the value of the variaiable in child1 (the variable to print)
            struct ValueHolder *valueToPrint = malloc(sizeof(struct ValueHolder));
            if (valueToPrint==NULL) {
                InterpreterError("Can't allocate memory for valueToPrint in atPrint");
                return 1;
            }

            if(InterpreteAST(ast->child1, valueToPrint, globalSymbolTable, localSymbolTable, NULL, NULL)) // If managed to retrieve the value
            {
                switch(valueToPrint->variableType) {
                    case integer:
                        printf("%d\n", valueToPrint->i);
                    break;
                    case floating:
                        printf("%f\n", valueToPrint->f);
                    break;
                    case characters:
                        printf("%s\n", valueToPrint->s);
                    break;
                    default:
                        InterpreterError("Not a valid variable type to print");
                        FreeValueHolder(valueToPrint);
                        return 1;
                    break;
                }
            }

            FreeValueHolder(valueToPrint);

            break;
        default:
            InterpreterError("Node not valid");
            return 1;
        break;
    }
}