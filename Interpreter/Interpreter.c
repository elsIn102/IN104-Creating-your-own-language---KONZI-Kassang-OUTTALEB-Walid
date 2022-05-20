#include <stdlib.h>
#include <string.h>

#include "Interpreter.h"
#include "../Utils/Hash.h"
#include "../Utils/ComparisonDictionnary.h"

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

/*

ast = the node of the AST to interpret

outVal = will hold the return value of the interpreted node

globalSymbolTable = a symbol table with all the global variables and functions

localSymbolTable = a symbol table for all the variables defined inside a function (local variables)

argsTable = symbol table holding the arguments of a function. 
    It is filled with the arguments name and type at func def and their value is assigned in atFuncCall
    But the function is called with this table as the localSymbolTable : it is only a temporary table deleted after the function call
    It is only used in atFuncDefArg types, to fill the values of the arguments with the values of the local or global variables

listOfArgs = a list to memorize the order of the arguments in the function definition, in order to keep this order during function call
    Used in atFuncDefArgsList, atFuncDefArg and atFuncCallArgsList 

returnValue = holds a pointer to the value that needs to be assigned by the next call of return (typically during an assignment of a function call).
    Is only modified by atReturn

comparisonDict = dictionnary of the comparisons declared in an if statement
    Set in atComparisonDeclaration and used in atlogicalAnd
    Must be passed in atStatementList, atTestBranch and atLogicalOr

*/

int InterpreteAST (struct AstNode* ast, struct ValueHolder* outVal, struct HashStruct* globalSymbolTable, struct HashStruct* localSymbolTable, struct HashStruct* argsTable, struct ArgList* listOfArgs, struct valueHolder* returnValue, struct Comparisons_Dict* comparisonDict) 
{
    if (ast==NULL)
        return 1;
    
    switch (ast->type)
    {
        case atRoot:
        {
            struct HashStruct* _globalSymbolTable = NULL;
            if (!Create_Hashtable(&globalSymbolTable)) {
                InterpreterError("Error while creating the global symbol table in atRoot");
                return 1;
            }

            //Variables and functions definitions
            int a = InterpreteAST(ast->child1, NULL, globalSymbolTable, NULL, NULL, NULL, NULL, NULL);
            //Main body of the code
            int b = InterpreteAST(ast->child2, NULL, globalSymbolTable, NULL, NULL, NULL, NULL, NULL);

            Free_Hashtable(_globalSymbolTable);

            return a && b;
            break;
        }
        case atStatementList:
        {
            if (ast->child1->type == atTestIfBranch || ast->child1->type == atTestElseIfBranch) 
            {
                struct ValueHolder* stopEvaluationsHolder = malloc(sizeof(struct ValueHolder));
                if (stopEvaluationsHolder==NULL) {
                    InterpreterError("Can't allocate memory for stopEvaluationsHolder in atStatementList");
                    return 1;
                }

                stopEvaluationsHolder->i = 0; // Don't stop by default

                int a = InterpreteAST(ast->child1, stopEvaluationsHolder, globalSymbolTable, localSymbolTable, NULL, NULL, returnValue, comparisonDict);

                if (!stopEvaluationsHolder->i) // If the if/else if/else statement has not been realized
                    return a && InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, returnValue, comparisonDict);
                
                return a;
            }
            else if (ast->child1->type == atReturn) { // Since every call of return can only be in a function (not a loop, nor an if), not calling the second child effectively ends the flow of the function when a return is met
                return InterpreteAST(ast->child1, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, returnValue, comparisonDict);
            }
            else {
                int a = InterpreteAST(ast->child1, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, returnValue, comparisonDict);                
                int b = InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, returnValue, comparisonDict);

                return a && b;
            }
            break;
        }
        case atLogicalOr: // Set outVal->i to 0 if the statement is true, 1 otherwise
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the result of the OR evaluation : outVal is null in atLogicalOr");
                return 1;
            }

            struct ValueHolder* booleanAndHolder1 = malloc(sizeof(struct ValueHolder));
            if (booleanAndHolder1==NULL) {
                InterpreterError("Can't allocate memory for booleanAndHolder1 in atLogicalOr");
                return 1;
            }

            struct ValueHolder* booleanAndHolder2 = malloc(sizeof(struct ValueHolder));
            if (booleanAndHolder2==NULL) {
                InterpreterError("Can't allocate memory for booleanAndHolder2 in atLogicalOr");
                FreeValueHolder(booleanAndHolder1);
                return 1;
            }

            // Evaluate the boolean value of the left and right expressions
            if (!(InterpreteAST(ast->child1, booleanAndHolder1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, comparisonDict)
                    && InterpreteAST(ast->child2, booleanAndHolder2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, comparisonDict))) {
                InterpreterError("Error while evaluating the left or right expression of the OR comparison");
                FreeValueHolder(booleanAndHolder1);
                FreeValueHolder(booleanAndHolder2);
                return 1;
            }

            outVal->i = booleanAndHolder1->i || booleanAndHolder2->i;

            return 0;
            break;
        }
        case atLogicalAnd: // Set outVal->i to 0 if the statement is true, 1 otherwise
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the result of the AND evaluation : outVal is null in atLogicalAnd");
                return 1;
            }

            struct ValueHolder* booleanAndHolder1 = malloc(sizeof(struct ValueHolder));
            if (booleanAndHolder1==NULL) {
                InterpreterError("Can't allocate memory for booleanAndHolder1 in atLogicalAnd");
                return 1;
            }

            struct ValueHolder* booleanAndHolder2 = malloc(sizeof(struct ValueHolder));
            if (booleanAndHolder2==NULL) {
                InterpreterError("Can't allocate memory for booleanAndHolder2 in atLogicalAnd");
                FreeValueHolder(booleanAndHolder1);
                return 1;
            }

            // Evaluate the boolean value of the left and right expressions
            if (!(InterpreteAST(ast->child1, booleanAndHolder1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, comparisonDict)
                    && InterpreteAST(ast->child2, booleanAndHolder2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, comparisonDict))) {
                InterpreterError("Error while evaluating the left or right expression of the AND comparison");
                FreeValueHolder(booleanAndHolder1);
                FreeValueHolder(booleanAndHolder2);
                return 1;
            }

            outVal->i = booleanAndHolder1->i && booleanAndHolder2->i;

            return 0;
            break;
        }
        case atVariableDef:
        {
            struct ValueHolder* varIdHolder = malloc(sizeof(struct ValueHolder));
            if (varIdHolder==NULL) {
                InterpreterError("Can't allocate memory for varIdHolder in atVariableDef");
                return 1;
            }

            if (InterpreteAST(ast->child1, varIdHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) { // get the name of the variable
                struct VariableStruct* varValue = malloc(sizeof(struct VariableStruct));
                if (varValue==NULL) {
                    InterpreterError("Can't allocate memory for varValue in atVariableDef");
                    FreeValueHolder(varIdHolder);
                    return 1;
                }

                // Fills the fields of varValue
                varValue->id = malloc(1 + strlen(varIdHolder->s));
                strcpy(varValue->id, varIdHolder->s);

                varValue->type = ast->variableType;

                switch (ast->variableType)
                {
                    case integer:
                        varValue->i = ast->i;
                        break;
                    case floating:
                        varValue->f = ast->f;
                        break;
                    case characters:
                        varValue->s = malloc(1 + strlen(ast->s));
                        strcpy(varValue->s, ast->s);
                        break;
                    default:
                        InterpreterError("Cannot define a variable with this type");
                        FreeValueHolder(varIdHolder);
                        FreeVariableStruct(varValue);
                        return 1;
                        break;
                }

                // Add varValue to the local hashtable if it exists, to the global one otherwise
                if (!((localSymbolTable!=NULL && Add_Hashtable(localSymbolTable, varValue->s, varValue))
                    || Add_Hashtable(globalSymbolTable, varValue->s, varValue)))
                {
                    InterpreterError("Error when adding the variable to the hashtable (atVariableDef)");
                    FreeValueHolder(varIdHolder);
                    FreeVariableStruct(varValue);
                    return 1;
                }
            }
            else {
                InterpreterError("Cannot get the Id of the variable in arVariableDef");
                FreeValueHolder(varIdHolder);
                return 1;
            }
            
            FreeValueHolder(varIdHolder);
            return 0;
            break;
        }
        case atFuncDef:
        {
            struct ValueHolder* funcIdHolder = malloc(sizeof(struct ValueHolder));
            if (funcIdHolder==NULL) {
                InterpreterError("Can't allocate memory for funcIdHolder in atFuncDef");
                return 1;
            }

            if (InterpreteAST(ast->child1, funcIdHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) // get the name of the function
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

                    if (!InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, _argsTable, _listOfArgs, NULL, NULL)) // If all arguments of the function has not been defined successfully
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
        }
        case atTest:
            {
                struct Comparisons_Dict* compDict;
                if (!CreateComparisonsDict(&compDict)) { //If it failed to create the dictionnary
                    InterpreterError("Unable to create the dictionnary for the tests (atTest)");
                    return 1;
                }

                // Fills the dictionnary with all the comparisons
                if (!InterpreteAST(ast->child1, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, compDict)) {
                    InterpreterError("Error while adding the comparisons to the dictionnary");
                    FreeComparisonsDict(compDict);
                    return 1;
                }

                //Interpretes the if/else_if/else statements using the dicionnary
                if (!InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, compDict)) {
                    InterpreterError("Error in the if/else if/else statement (atTest)");
                    FreeComparisonsDict(compDict);
                    return 1;
                }

                FreeComparisonsDict(compDict);
                return 0;
                break;
            }
        case atComparisonDeclaration: // Adds the comparison to the dictionnary
        {
            if (ast->child1->type == integer || ast->child1->type == floating)
            {
                if (ast->child2->type != integer || ast->child2->type != floating)
                {
                    InterpreterError("Impossible to compare these variables (atComparisonDefinition): Incompatible variable types");
                    return 1;
                }
            }
            else if (ast->child1->type == characters)
            {
                if (ast->child2->type != characters)
                {
                    InterpreterError("Impossible to compare these variables (atComparisonDefinition): Incompatible variable types");
                    return 1;
                }
            }
            else {
                InterpreterError("Impossible to use a variable with no type in a comparison (atComparisonDeclaration)");
                return 1;
            }

            if (!Add_ComparisonsDict(&comparisonDict, ast->i, ast->comparator, ast->child1, ast->child2)) {
                InterpreterError("Could not add the comparison to the dictionnary");
                return 1;
            }

            return 0;
            break;
        }
        case atComparisonId: // Set outVal->i to 0 if the comparison is true, 1 otherwise
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the result of the comparison : outVal is null in atComparisonId");
                return 1;
            }

            struct ComparisonValue *comparison;
            if (!TryFind_ComparisonsDict(comparisonDict, ast->i, &comparison)) {
                char* msg;
                sprintf(msg, "Unable to find the comparison (match %d) in this dictionnary", ast->i);
                InterpreterError(msg);
                free(msg);
                return 1;
            }

            struct ValueHolder* var1Holder = malloc(sizeof(struct ValueHolder));
            if (var1Holder==NULL) {
                InterpreterError("Can't allocate memory for var1Holder in atComparisonId");
                return 1;
            }

            struct ValueHolder* var2Holder = malloc(sizeof(struct ValueHolder));
            if (var2Holder==NULL) {
                InterpreterError("Can't allocate memory for var2Holder in atComparisonId");
                FreeValueHolder(var1Holder);
                return 1;
            }
            
            if (!InterpreteAST(comparison->value1, var1Holder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)
                || !InterpreteAST(comparison->value2, var2Holder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) 
            {
                InterpreterError("Error while getting the value of the variables to compara in atComparisonId");
                FreeValueHolder(var1Holder);
                FreeValueHolder(var2Holder);
                return 1;
            }
            
            if (comparison->value1->type == atConstant && comparison->value1->type == atConstant)
            {
                switch(var1Holder->variableType) {
                    case integer:
                        if (var2Holder->variableType == integer) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Holder->i >= var2Holder->i;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Holder->i > var2Holder->i;
                                    break;
                                case neq:
                                    outVal->i = var1Holder->i != var2Holder->i;
                                    break;
                                case eq:
                                    outVal->i = var1Holder->i == var2Holder->i;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else if (var2Holder->variableType == floating) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Holder->i >= var2Holder->f;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Holder->i > var2Holder->f;
                                    break;
                                case neq:
                                    outVal->i = var1Holder->i != var2Holder->f;
                                    break;
                                case eq:
                                    outVal->i = var1Holder->i == var2Holder->f;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    case floating:
                        if (var2Holder->variableType == integer) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Holder->f >= var2Holder->i;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Holder->f > var2Holder->i;
                                    break;
                                case neq:
                                    outVal->i = var1Holder->f != var2Holder->i;
                                    break;
                                case eq:
                                    outVal->i = var1Holder->f == var2Holder->i;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else if (var2Holder->variableType == floating) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Holder->f >= var2Holder->f;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Holder->f > var2Holder->f;
                                    break;
                                case neq:
                                    outVal->i = var1Holder->f != var2Holder->f;
                                    break;
                                case eq:
                                    outVal->i = var1Holder->f == var2Holder->f;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    case characters:
                        if (var2Holder->variableType == characters) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = strcmp(var1Holder->s, var2Holder->s) >= 0;
                                    break;
                                case str_gtr:
                                    outVal->i = strcmp(var1Holder->s, var2Holder->s) > 0;
                                    break;
                                case neq:
                                    outVal->i = strcmp(var1Holder->s, var2Holder->s) != 0;
                                    break;
                                case eq:
                                    outVal->i = strcmp(var1Holder->s, var2Holder->s) == 0;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    default:
                        InterpreterError("Imopssible to compare these types of value");
                        FreeValueHolder(var1Holder);
                        FreeValueHolder(var2Holder);
                        return 1;
                    break;
                }
            }
            else if (ast->child1->type == atId && ast->child2->type == atId)
            {
                // Get the values of the variables
                struct VariableStruct *var1Struct, *var2Struct;
                // Using the lazy evaluation to first look at local variables
                if ((!TryFind_Hashtable(localSymbolTable, var1Holder->s, var1Struct) && !TryFind_Hashtable(globalSymbolTable, var1Holder->s, var1Struct))
                        || (!TryFind_Hashtable(localSymbolTable, var2Holder->s, var2Struct) && !TryFind_Hashtable(globalSymbolTable, var2Holder->s, var2Struct))) 
                {
                    InterpreterError("No defined variable with this name (atComparisonId)");
                    FreeValueHolder(var1Holder);
                    FreeValueHolder(var2Holder);
                    return 1;
                }

                switch(var1Struct->type) {
                    case integer:
                        if (var2Struct->type == integer) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Struct->i >= var2Struct->i;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Struct->i > var2Struct->i;
                                    break;
                                case neq:
                                    outVal->i = var1Struct->i != var2Struct->i;
                                    break;
                                case eq:
                                    outVal->i = var1Struct->i == var2Struct->i;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else if (var2Struct->type == floating) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Struct->i >= var2Struct->f;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Struct->i > var2Struct->f;
                                    break;
                                case neq:
                                    outVal->i = var1Struct->i != var2Struct->f;
                                    break;
                                case eq:
                                    outVal->i = var1Struct->i == var2Struct->f;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    case floating:
                        if (var2Struct->type == integer) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Struct->f >= var2Struct->i;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Struct->f > var2Struct->i;
                                    break;
                                case neq:
                                    outVal->i = var1Struct->f != var2Struct->i;
                                    break;
                                case eq:
                                    outVal->i = var1Struct->f == var2Struct->i;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else if (var2Struct->type == floating) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Struct->f >= var2Struct->f;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Struct->f > var2Struct->f;
                                    break;
                                case neq:
                                    outVal->i = var1Struct->f != var2Struct->f;
                                    break;
                                case eq:
                                    outVal->i = var1Struct->f == var2Struct->f;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    case characters:
                        if (var2Struct->type == characters) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = strcmp(var1Struct->s, var2Struct->s) >= 0;
                                    break;
                                case str_gtr:
                                    outVal->i = strcmp(var1Struct->s, var2Struct->s) > 0;
                                    break;
                                case neq:
                                    outVal->i = strcmp(var1Struct->s, var2Struct->s) != 0;
                                    break;
                                case eq:
                                    outVal->i = strcmp(var1Struct->s, var2Struct->s) == 0;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    default:
                        InterpreterError("Imopssible to compare these types of value");
                        FreeValueHolder(var1Holder);
                        FreeValueHolder(var2Holder);
                        return 1;
                    break;
                }
            }
            else if (ast->child1->type == atConstant && ast->child2->type == atId) // If the first one is a constant, the second a variable
            {
                // Get the value of the variable
                struct VariableStruct* varStruct;
                // Using the lazy evaluation to first look at local variables
                if (!TryFind_Hashtable(localSymbolTable, var2Holder->s, varStruct) && !TryFind_Hashtable(globalSymbolTable, var2Holder->s, varStruct)) {
                    InterpreterError("No defined variable with this name (atComparisonId)");
                    FreeValueHolder(var1Holder);
                    FreeValueHolder(var2Holder);
                    return 1;
                }

                switch(var1Holder->variableType) {
                    case integer:
                        if (varStruct->type == integer) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Holder->i >= varStruct->i;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Holder->i > varStruct->i;
                                    break;
                                case neq:
                                    outVal->i = var1Holder->i != varStruct->i;
                                    break;
                                case eq:
                                    outVal->i = var1Holder->i == varStruct->i;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else if (varStruct->type == floating) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Holder->i >= varStruct->f;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Holder->i > varStruct->f;
                                    break;
                                case neq:
                                    outVal->i = var1Holder->i != varStruct->f;
                                    break;
                                case eq:
                                    outVal->i = var1Holder->i == varStruct->f;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    case floating:
                        if (varStruct->type == integer) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Holder->f >= varStruct->i;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Holder->f > varStruct->i;
                                    break;
                                case neq:
                                    outVal->i = var1Holder->f != varStruct->i;
                                    break;
                                case eq:
                                    outVal->i = var1Holder->f == varStruct->i;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else if (varStruct->type == floating) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = var1Holder->f >= varStruct->f;
                                    break;
                                case str_gtr:
                                    outVal->i = var1Holder->f > varStruct->f;
                                    break;
                                case neq:
                                    outVal->i = var1Holder->f != varStruct->f;
                                    break;
                                case eq:
                                    outVal->i = var1Holder->f == varStruct->f;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    case characters:
                        if (varStruct->type == characters) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = strcmp(var1Holder->s, varStruct->s) >= 0;
                                    break;
                                case str_gtr:
                                    outVal->i = strcmp(var1Holder->s, varStruct->s) > 0;
                                    break;
                                case neq:
                                    outVal->i = strcmp(var1Holder->s, varStruct->s) != 0;
                                    break;
                                case eq:
                                    outVal->i = strcmp(var1Holder->s, varStruct->s) == 0;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    default:
                        InterpreterError("Imopssible to compare these types of value");
                        FreeValueHolder(var1Holder);
                        FreeValueHolder(var2Holder);
                        return 1;
                    break;
                }
            }
            else if (ast->child1->type == atId && ast->child2->type == atConstant) // If the first one is a variable, the second a constant
            {
                // Get the value of the variable
                struct VariableStruct* varStruct;
                // Using the lazy evaluation to first look at local variables
                if (!TryFind_Hashtable(localSymbolTable, var1Holder->s, varStruct) && !TryFind_Hashtable(globalSymbolTable, var1Holder->s, varStruct)) {
                    InterpreterError("No defined variable with this name (atComparisonId)");
                    FreeValueHolder(var1Holder);
                    FreeValueHolder(var2Holder);
                    return 1;
                }

                switch(var2Holder->variableType) {
                    case integer:
                        if (varStruct->type == integer) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = varStruct->i >= var2Holder->i;
                                    break;
                                case str_gtr:
                                    outVal->i = varStruct->i > var2Holder->i;
                                    break;
                                case neq:
                                    outVal->i = varStruct->i != var2Holder->i;
                                    break;
                                case eq:
                                    outVal->i = varStruct->i == var2Holder->i;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else if (varStruct->type == floating) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = varStruct->f >= var2Holder->i;
                                    break;
                                case str_gtr:
                                    outVal->i = varStruct->f > var2Holder->i;
                                    break;
                                case neq:
                                    outVal->i = varStruct->f != var2Holder->i;
                                    break;
                                case eq:
                                    outVal->i = varStruct->f == var2Holder->i;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    case floating:
                        if (varStruct->type == integer) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = varStruct->i >= var2Holder->f;
                                    break;
                                case str_gtr:
                                    outVal->i = varStruct->i > var2Holder->f;
                                    break;
                                case neq:
                                    outVal->i = varStruct->i != var2Holder->f;
                                    break;
                                case eq:
                                    outVal->i = varStruct->i == var2Holder->f;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else if (varStruct->type == floating) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = varStruct->f >= var2Holder->f;
                                    break;
                                case str_gtr:
                                    outVal->i = varStruct->f > var2Holder->f;
                                    break;
                                case neq:
                                    outVal->i = varStruct->f != var2Holder->f;
                                    break;
                                case eq:
                                    outVal->i = varStruct->f == var2Holder->f;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    case characters:
                        if (varStruct->type == characters) {
                            switch (comparison->comparator) {
                                case gtr:
                                    outVal->i = strcmp(varStruct->s, var2Holder->s) >= 0;
                                    break;
                                case str_gtr:
                                    outVal->i = strcmp(varStruct->s, var2Holder->s) > 0;
                                    break;
                                case neq:
                                    outVal->i = strcmp(varStruct->s, var2Holder->s) != 0;
                                    break;
                                case eq:
                                    outVal->i = strcmp(varStruct->s, var2Holder->s) == 0;
                                    break;
                                default:
                                    InterpreterError("Not a valid comparator");
                                    FreeValueHolder(var1Holder);
                                    FreeValueHolder(var2Holder);
                                    return 1;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        }
                    break;
                    default:
                        InterpreterError("Imopssible to compare these types of value");
                        FreeValueHolder(var1Holder);
                        FreeValueHolder(var2Holder);
                        return 1;
                    break;
                }
            }
            else {
                InterpreterError("Invalid child type in atComparisonId");
                FreeValueHolder(var1Holder);
                FreeValueHolder(var2Holder);
                return 1;
            }

            FreeValueHolder(var1Holder);
            FreeValueHolder(var2Holder);
            return 0;
            break;
        }
        case atTestIfBranch: // If the condition is true, launches the atAssignment and set outVal->i to 1
        {
            struct ValueHolder* booleanValueHolder = malloc(sizeof(struct ValueHolder));
            if (booleanValueHolder==NULL) {
                InterpreterError("Can't allocate memory for booleanValueHolder in atTestIfBranch");
                return 1;
            }

            // Evaluate the condition and put the result in booleanValueHolder->i (0 = true, 1 = false)
            if (InterpreteAST(ast->child1, booleanValueHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, comparisonDict)) {
                if (booleanValueHolder->i) { // If the comparison is true, interprete the branch
                    InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL);
                    outVal->i = 1;
                }
            }
            else {
                InterpreterError("Error while evaluating the boolean expression (atTestIfBranch)");
                FreeValueHolder(booleanValueHolder);
                return 1;
            }

            FreeValueHolder(booleanValueHolder);
            return 0;
            break;
        }
        case atTestElseIfBranch: // If the condition is true, launches the atAssignment and set outVal->i to 1
        {
            struct ValueHolder* booleanValueHolder = malloc(sizeof(struct ValueHolder));
            if (booleanValueHolder==NULL) {
                InterpreterError("Can't allocate memory for booleanValueHolder in atTestElseIfBranch");
                return 1;
            }

            // Evaluate the condition and put the result in booleanValueHolder->i (0 = true, 1 = false)
            if (InterpreteAST(ast->child1, booleanValueHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, comparisonDict)) {
                if (booleanValueHolder->i) { // If the comparison is true, interprete the branch
                    InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL);
                    outVal->i = 1;
                }
            }
            else {
                InterpreterError("Error while evaluating the boolean expression (atTestElseIfBranch)");
                FreeValueHolder(booleanValueHolder);
                return 1;
            }

            FreeValueHolder(booleanValueHolder);
            return 0;
            break;
        }
        case atTestElseBranch:
        {
            InterpreteAST(ast->child1, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL);
            return 0;
            break;
        }
        case atAssignment:
        {
            if (ast->child1->type==atVoid && ast->child2->type==atFuncCall) { // then it's a call of a function without catching the return value
                InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL);
            }
            else
            {
                struct ValueHolder* varIdHolder = malloc(sizeof(struct ValueHolder));
                if (varIdHolder==NULL) {
                    InterpreterError("Can't allocate memory for varIdHolder in atAssignment");
                    return 1;
                }
                
                // Get the id of the variable to assign to
                if (InterpreteAST(ast->child1, varIdHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) 
                {
                    if (varIdHolder->variableType != characters) {
                        InterpreterError("Not a valid variable Id in atAssignment");
                        FreeValueHolder(varIdHolder);
                        return 1;
                    }

                    // Get a pointer to the variable to assign to in the hashtable
                    struct VariableStruct* varStruct;
                    // Using the lazy evaluation to first look at local variables
                    if (!TryFind_Hashtable(localSymbolTable, varIdHolder->s, varStruct) && !TryFind_Hashtable(globalSymbolTable, varIdHolder->s, varStruct)) {
                        InterpreterError("No defined variable with this name (atAssignment)");
                        FreeValueHolder(varIdHolder);
                        return 1;
                    }

                    // If it's an assignation from another variable
                    if (ast->child2->type == atId) {
                        struct ValueHolder* var2IdHolder = malloc(sizeof(struct ValueHolder));
                        if (var2IdHolder==NULL) {
                            InterpreterError("Can't allocate memory for var2IdHolder in atAssignment");
                            FreeValueHolder(varIdHolder);
                            return 1;
                        }

                        // Get the id of the variable
                        if (InterpreteAST(ast->child2, var2IdHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) {
                            if (var2IdHolder->variableType != characters) {
                                InterpreterError("Not a valid variable Id (2) in atAssignment");
                                FreeValueHolder(varIdHolder);
                                FreeValueHolder(var2IdHolder);
                                return 1;
                            }

                            // Get a pointer to the variable
                            struct VariableStruct* var2Struct;
                            // Using the lazy evaluation to first look at local variables
                            if (!TryFind_Hashtable(localSymbolTable, var2IdHolder->s, var2Struct) && !TryFind_Hashtable(globalSymbolTable, var2IdHolder->s, var2Struct)) {
                                InterpreterError("No defined variable with this name (2) (atAssignment)");
                                FreeValueHolder(varIdHolder);
                                FreeValueHolder(var2IdHolder);
                                return 1;
                            }

                            // Check that the type of the 2 variables is matching
                            if (varStruct->type != var2Struct->type) {
                                InterpreterError("These variables don't have the same type : impossible to assign");
                                FreeValueHolder(varIdHolder);
                                FreeValueHolder(var2IdHolder);
                                return 1;
                            }

                            // Change the value of the variable
                            switch (varStruct->type) {
                                case integer:
                                    varStruct->i = var2Struct->i;
                                    break;
                                case floating:
                                    varStruct->f = var2Struct->f;
                                    break;
                                case characters:
                                    if (varStruct->s != NULL)
                                        free(varStruct->s);
                                    
                                    varStruct->s = malloc(1 + strlen(var2Struct->s));
                                    strcpy(varStruct->s, var2Struct->s);
                                    break;
                                default:
                                    InterpreterError("Impossible to assign this type of variable (atAssignment)");
                                    FreeValueHolder(varIdHolder);
                                    FreeValueHolder(var2IdHolder);
                                    return 1;
                                    break;
                            }

                            FreeValueHolder(varIdHolder);
                            FreeValueHolder(var2IdHolder);
                            return 0;
                        }
                        else { 
                            InterpreterError("Can't get the id of the variable to assign in atAssignment");
                            FreeValueHolder(varIdHolder);
                            FreeValueHolder(var2IdHolder);
                            return 1;
                        }
                    }
                    else { // if it's not an assignment from another variable
                        struct ValueHolder* valToAssign = malloc(sizeof(struct ValueHolder));
                        if (valToAssign==NULL) {
                            InterpreterError("Can't allocate memory for valToAssign in atAssignment");
                            FreeValueHolder(varIdHolder);
                            return 1;
                        }

                        // Get the value to assign
                        if (InterpreteAST(ast->child2, valToAssign, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) {
                            // Check that the type of the variable and the valToAssign is matching
                            if (varStruct->type != valToAssign->variableType) {
                                InterpreterError("Type of the variable not matching the type of the other hand of the assignment");
                                FreeValueHolder(varIdHolder);
                                FreeValueHolder(valToAssign);
                                return 1;
                            }

                            // Change the value of the variable
                            switch (varStruct->type) {
                                case integer:
                                    varStruct->i = valToAssign->i;
                                    break;
                                case floating:
                                    varStruct->f = valToAssign->f;
                                    break;
                                case characters:
                                    if (varStruct->s != NULL)
                                        free(varStruct->s);
                                    
                                    varStruct->s = malloc(1 + strlen(valToAssign->s));
                                    strcpy(varStruct->s, valToAssign->s);
                                    break;
                                default:
                                    InterpreterError("Impossible to assign this type of variable (atAssignment)");
                                    FreeValueHolder(varIdHolder);
                                    FreeValueHolder(valToAssign);
                                    return 1;
                                    break;
                            }

                            FreeValueHolder(varIdHolder);
                            FreeValueHolder(valToAssign);
                            return 0;
                        }
                        else {
                            InterpreterError("Error while evaluating the left side of the assignment");
                            FreeValueHolder(varIdHolder);
                            FreeValueHolder(valToAssign);
                            return 1;
                        }
                    }
                }
                else { 
                    InterpreterError("Can't get the id of the variable to assign to in atAssignment");
                    FreeValueHolder(varIdHolder);
                    return 1;
                }
            }

            return 0;
            break;
        }
        case atFuncCall:
        {
            struct ValueHolder* funcIdHolder = malloc(sizeof(struct ValueHolder));
            if (funcIdHolder==NULL) {
                InterpreterError("Can't allocate memory for funcIdHolder in atFuncCall");
                return 1;
            }

            if (InterpreteAST(ast->child1, funcIdHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) //If manages to retrieve the id of the function
            {
                if (funcIdHolder->variableType != characters) {
                    InterpreterError("Not a valid function Id");
                    FreeValueHolder(funcIdHolder);
                    return 1;
                }

                struct VariableStruct* funcVarStruct;
                if (TryFind_Hashtable (globalSymbolTable, funcIdHolder->s, &funcVarStruct)) // If the function was defined
                {
                    // Fill the table of local arguments (argsTable) with the values used to call the function
                    if (ast->child2->type != atVoid)
                    {
                        if (!InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, funcVarStruct->argumentsTable, funcVarStruct->argumentsList, NULL, NULL))
                        {
                            InterpreterError("Could not assign all the arguments for the call of the function");
                            FreeValueHolder(funcIdHolder);
                            return 1;
                        }
                    }

                    // Call the function and return the output value
                    if (!InterpreteAST(funcVarStruct->functionBody, NULL, globalSymbolTable, funcVarStruct->argumentsTable, NULL, NULL, outVal, NULL)) { // If an error occurred while calling the function
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
        }
        case atFuncCallArgList: // Get through the listOfArgs and assign their value to the localSymbolTable
        {
            // Make sure there is at least one more argument
            if (listOfArgs==NULL)
            {
                InterpreterError("Not enought arguments in the function call");
                return 1;
            }

            // Get a pointer to the argument wich value we need to assign in the local hashtable
            struct VariableStruct* foundArg;
            if (TryFind_Hashtable(argsTable, listOfArgs->id, &foundArg))
            {
                // Get the value to assgin to the argument
                struct ValueHolder* argVal = malloc(sizeof(struct ValueHolder));
                if (argVal==NULL) {
                    InterpreterError("Can't allocate memory for argVal in atFuncCallArgList");
                    return 1;
                }

                if (InterpreteAST(ast->child1, argVal, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
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
                return InterpreteAST(ast->child1, NULL, globalSymbolTable, localSymbolTable, argsTable, listOfArgs->next, NULL, NULL);

            return 0;
            break;
        }
        case atWhileLoop:
        {
            struct ValueHolder* comparisonResult = malloc(sizeof(struct ValueHolder));
            if (comparisonResult==NULL) {
                InterpreterError("Can't allocate memory for comparisonResult in atWhileLoop");
                return 1;
            }

            // Run the loop as long as comparisonResult->i == 0 ie as long as the condition is true
            for (InterpreteAST(ast->child1, comparisonResult, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL); comparisonResult->i; InterpreteAST(ast->child1, comparisonResult, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
            {
                // Run the body of the loop
                InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL);
            }

            FreeValueHolder(comparisonResult);
            return 0;
            break;
        }
        case atWhileCompare: // returns 0 in outVal if the comparison is true, 1 otherwise
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the comparison : outVal is null in atCompare");
                return 1;
            }

            outVal->variableType = integer;

            struct ValueHolder* var1Holder = malloc(sizeof(struct ValueHolder));
            if (var1Holder==NULL) {
                InterpreterError("Can't allocate memory for var1Holder in atCompare");
                return 1;
            }

            struct ValueHolder* var2Holder = malloc(sizeof(struct ValueHolder));
            if (var2Holder==NULL) {
                InterpreterError("Can't allocate memory for var2Holder in atCompare");
                FreeValueHolder(var1Holder);
                return 1;
            }

            if (InterpreteAST(ast->child1, var1Holder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL) && InterpreteAST(ast->child2, var2Holder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) 
            {
                if (ast->child1->type == atConstant && ast->child2->type == atConstant)
                {
                    switch(var1Holder->variableType) {
                        case integer:
                            if (var2Holder->variableType == integer) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Holder->i >= var2Holder->i;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Holder->i > var2Holder->i;
                                        break;
                                    case neq:
                                        outVal->i = var1Holder->i != var2Holder->i;
                                        break;
                                    case eq:
                                        outVal->i = var1Holder->i == var2Holder->i;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else if (var2Holder->variableType == floating) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Holder->i >= var2Holder->f;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Holder->i > var2Holder->f;
                                        break;
                                    case neq:
                                        outVal->i = var1Holder->i != var2Holder->f;
                                        break;
                                    case eq:
                                        outVal->i = var1Holder->i == var2Holder->f;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        case floating:
                            if (var2Holder->variableType == integer) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Holder->f >= var2Holder->i;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Holder->f > var2Holder->i;
                                        break;
                                    case neq:
                                        outVal->i = var1Holder->f != var2Holder->i;
                                        break;
                                    case eq:
                                        outVal->i = var1Holder->f == var2Holder->i;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else if (var2Holder->variableType == floating) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Holder->f >= var2Holder->f;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Holder->f > var2Holder->f;
                                        break;
                                    case neq:
                                        outVal->i = var1Holder->f != var2Holder->f;
                                        break;
                                    case eq:
                                        outVal->i = var1Holder->f == var2Holder->f;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        case characters:
                            if (var2Holder->variableType == characters) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = strcmp(var1Holder->s, var2Holder->s) >= 0;
                                        break;
                                    case str_gtr:
                                        outVal->i = strcmp(var1Holder->s, var2Holder->s) > 0;
                                        break;
                                    case neq:
                                        outVal->i = strcmp(var1Holder->s, var2Holder->s) != 0;
                                        break;
                                    case eq:
                                        outVal->i = strcmp(var1Holder->s, var2Holder->s) == 0;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        default:
                            InterpreterError("Imopssible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        break;
                    }
                }
                else if (ast->child1->type == atId && ast->child2->type == atId)
                {
                    // Get the values of the variables
                    struct VariableStruct *var1Struct, *var2Struct;
                    // Using the lazy evaluation to first look at local variables
                    if ((!TryFind_Hashtable(localSymbolTable, var1Holder->s, var1Struct) && !TryFind_Hashtable(globalSymbolTable, var1Holder->s, var1Struct))
                            || (!TryFind_Hashtable(localSymbolTable, var2Holder->s, var2Struct) && !TryFind_Hashtable(globalSymbolTable, var2Holder->s, var2Struct))) 
                    {
                        InterpreterError("No defined variable with this name (atWhileCompare)");
                        FreeValueHolder(var1Holder);
                        FreeValueHolder(var2Holder);
                        return 1;
                    }

                    switch(var1Struct->type) {
                        case integer:
                            if (var2Struct->type == integer) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Struct->i >= var2Struct->i;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Struct->i > var2Struct->i;
                                        break;
                                    case neq:
                                        outVal->i = var1Struct->i != var2Struct->i;
                                        break;
                                    case eq:
                                        outVal->i = var1Struct->i == var2Struct->i;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else if (var2Struct->type == floating) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Struct->i >= var2Struct->f;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Struct->i > var2Struct->f;
                                        break;
                                    case neq:
                                        outVal->i = var1Struct->i != var2Struct->f;
                                        break;
                                    case eq:
                                        outVal->i = var1Struct->i == var2Struct->f;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        case floating:
                            if (var2Struct->type == integer) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Struct->f >= var2Struct->i;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Struct->f > var2Struct->i;
                                        break;
                                    case neq:
                                        outVal->i = var1Struct->f != var2Struct->i;
                                        break;
                                    case eq:
                                        outVal->i = var1Struct->f == var2Struct->i;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else if (var2Struct->type == floating) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Struct->f >= var2Struct->f;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Struct->f > var2Struct->f;
                                        break;
                                    case neq:
                                        outVal->i = var1Struct->f != var2Struct->f;
                                        break;
                                    case eq:
                                        outVal->i = var1Struct->f == var2Struct->f;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        case characters:
                            if (var2Struct->type == characters) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = strcmp(var1Struct->s, var2Struct->s) >= 0;
                                        break;
                                    case str_gtr:
                                        outVal->i = strcmp(var1Struct->s, var2Struct->s) > 0;
                                        break;
                                    case neq:
                                        outVal->i = strcmp(var1Struct->s, var2Struct->s) != 0;
                                        break;
                                    case eq:
                                        outVal->i = strcmp(var1Struct->s, var2Struct->s) == 0;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        default:
                            InterpreterError("Imopssible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        break;
                    }
                }
                else if (ast->child1->type == atConstant) //Then the first one is a constant, the second one a variable
                {
                    // Get the value of the variable
                    struct VariableStruct* varStruct;
                    // Using the lazy evaluation to first look at local variables
                    if (!TryFind_Hashtable(localSymbolTable, var2Holder->s, varStruct) && !TryFind_Hashtable(globalSymbolTable, var2Holder->s, varStruct)) {
                        InterpreterError("No defined variable with this name (atWhileCompare)");
                        FreeValueHolder(var1Holder);
                        FreeValueHolder(var2Holder);
                        return 1;
                    }

                    switch(var1Holder->variableType) {
                        case integer:
                            if (varStruct->type == integer) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Holder->i >= varStruct->i;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Holder->i > varStruct->i;
                                        break;
                                    case neq:
                                        outVal->i = var1Holder->i != varStruct->i;
                                        break;
                                    case eq:
                                        outVal->i = var1Holder->i == varStruct->i;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else if (varStruct->type == floating) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Holder->i >= varStruct->f;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Holder->i > varStruct->f;
                                        break;
                                    case neq:
                                        outVal->i = var1Holder->i != varStruct->f;
                                        break;
                                    case eq:
                                        outVal->i = var1Holder->i == varStruct->f;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        case floating:
                            if (varStruct->type == integer) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Holder->f >= varStruct->i;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Holder->f > varStruct->i;
                                        break;
                                    case neq:
                                        outVal->i = var1Holder->f != varStruct->i;
                                        break;
                                    case eq:
                                        outVal->i = var1Holder->f == varStruct->i;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else if (varStruct->type == floating) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = var1Holder->f >= varStruct->f;
                                        break;
                                    case str_gtr:
                                        outVal->i = var1Holder->f > varStruct->f;
                                        break;
                                    case neq:
                                        outVal->i = var1Holder->f != varStruct->f;
                                        break;
                                    case eq:
                                        outVal->i = var1Holder->f == varStruct->f;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        case characters:
                            if (varStruct->type == characters) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = strcmp(var1Holder->s, varStruct->s) >= 0;
                                        break;
                                    case str_gtr:
                                        outVal->i = strcmp(var1Holder->s, varStruct->s) > 0;
                                        break;
                                    case neq:
                                        outVal->i = strcmp(var1Holder->s, varStruct->s) != 0;
                                        break;
                                    case eq:
                                        outVal->i = strcmp(var1Holder->s, varStruct->s) == 0;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        default:
                            InterpreterError("Imopssible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        break;
                    }
                }
                else // If the first one is a variable and the second one a constant
                {
                    // Get the value of the variable
                    struct VariableStruct* varStruct;
                    // Using the lazy evaluation to first look at local variables
                    if (!TryFind_Hashtable(localSymbolTable, var1Holder->s, varStruct) && !TryFind_Hashtable(globalSymbolTable, var1Holder->s, varStruct)) {
                        InterpreterError("No defined variable with this name (atWhileCompare)");
                        FreeValueHolder(var1Holder);
                        FreeValueHolder(var2Holder);
                        return 1;
                    }

                    switch(var2Holder->variableType) {
                        case integer:
                            if (varStruct->type == integer) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = varStruct->i >= var2Holder->i;
                                        break;
                                    case str_gtr:
                                        outVal->i = varStruct->i > var2Holder->i;
                                        break;
                                    case neq:
                                        outVal->i = varStruct->i != var2Holder->i;
                                        break;
                                    case eq:
                                        outVal->i = varStruct->i == var2Holder->i;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else if (varStruct->type == floating) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = varStruct->f >= var2Holder->i;
                                        break;
                                    case str_gtr:
                                        outVal->i = varStruct->f > var2Holder->i;
                                        break;
                                    case neq:
                                        outVal->i = varStruct->f != var2Holder->i;
                                        break;
                                    case eq:
                                        outVal->i = varStruct->f == var2Holder->i;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        case floating:
                            if (varStruct->type == integer) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = varStruct->i >= var2Holder->f;
                                        break;
                                    case str_gtr:
                                        outVal->i = varStruct->i > var2Holder->f;
                                        break;
                                    case neq:
                                        outVal->i = varStruct->i != var2Holder->f;
                                        break;
                                    case eq:
                                        outVal->i = varStruct->i == var2Holder->f;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else if (varStruct->type == floating) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = varStruct->f >= var2Holder->f;
                                        break;
                                    case str_gtr:
                                        outVal->i = varStruct->f > var2Holder->f;
                                        break;
                                    case neq:
                                        outVal->i = varStruct->f != var2Holder->f;
                                        break;
                                    case eq:
                                        outVal->i = varStruct->f == var2Holder->f;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        case characters:
                            if (varStruct->type == characters) {
                                switch (ast->comparator) {
                                    case gtr:
                                        outVal->i = strcmp(varStruct->s, var2Holder->s) >= 0;
                                        break;
                                    case str_gtr:
                                        outVal->i = strcmp(varStruct->s, var2Holder->s) > 0;
                                        break;
                                    case neq:
                                        outVal->i = strcmp(varStruct->s, var2Holder->s) != 0;
                                        break;
                                    case eq:
                                        outVal->i = strcmp(varStruct->s, var2Holder->s) == 0;
                                        break;
                                    default:
                                        InterpreterError("Not a valid comparator");
                                        FreeValueHolder(var1Holder);
                                        FreeValueHolder(var2Holder);
                                        return 1;
                                        break;
                                }
                            }
                            else {
                                InterpreterError("Impossible to compare these types of value");
                                FreeValueHolder(var1Holder);
                                FreeValueHolder(var2Holder);
                                return 1;
                            }
                        break;
                        default:
                            InterpreterError("Imopssible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 1;
                        break;
                    }
                }
            }
            else {
                InterpreterError("Could not get the value of the variables to compare (atWhileCompare)");
                FreeValueHolder(var1Holder);
                FreeValueHolder(var2Holder);
                return 1;
            }
            
            FreeValueHolder(var1Holder);
            FreeValueHolder(var2Holder);

            return 0;
            break;
        }
        case atBreak:
            // Need to implement
            return 0;
            break;
        case atReturn: // Assign the return value. The end of the flow is done in atStatementList since it is the only place where a return can exist
        {
            if(!InterpreteAST(ast->child1, returnValue, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) { // If could not evaluate the id or constant to return
                InterpreterError("Could not get the value to return");
                return 1;
            }

            return 0;
            break;
        }
        case atContinue:
            // Need to implement
            return 0;
            break;
        case atId: // assigns outVal with the name of the variable or function
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the Id : outVal is null in atId");
                return 1;
            }

            if (outVal->s!=NULL)
                free(outVal->s);
            
            outVal->s = malloc(1 + strlen(ast->s));
            strcpy(outVal->s, ast->s);

            return 0;
            break;
        }
        case atFuncDefArgsList: // Add the arguments to the hashtable argsTable and return the list of arguments in listOfArgs
        {
            // Add the argument to argsTable and fill the value of the head of listOfArgs
            if (InterpreteAST(ast->child1, NULL, globalSymbolTable, localSymbolTable, argsTable, listOfArgs, NULL, NULL))
            {
                if (ast->child2!=NULL) { // if there is another argument to define
                    // Create the next element in the list of arguments
                    struct ArgList* newArg = malloc(sizeof(struct ArgList));
                    if (newArg==NULL) {
                        InterpreterError("Can't allocate memory for newArg in atFuncDefArgsList");
                        return 1;
                    }

                    // Fill the tail of the list with the arguments
                    if (!InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, argsTable, newArg, NULL, NULL)) {
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
        }
        case atFuncDefArg: // Add the argument to argsTable and set it in listOfArgs
        {
            struct ValueHolder* argId = malloc(sizeof(struct ValueHolder));
            if (argId==NULL) {
                InterpreterError("Can't allocate memory for argId in atFuncDefArg");
                return 1;
            }

            // Get the Id of the argument
            if (InterpreteAST(ast->child1, argId, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
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
        }
        case atConstant:
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the constant : outVal is null in atConstant");
                return 1;
            }

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
        }
        case atVoid:
        {
            if (!outVal==NULL) 
                outVal->variableType = noType;

            return 0;
            break;
        }
        case atAdd:
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the addition : outVal is null in atAdd");
                return 1;
            }

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
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
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
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the substraction : outVal is null in atMinus");
                return 1;
            }

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
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
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
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the multiplication : outVal is null in atMultiply");
                return 1;
            }

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
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
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
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the division : outVal is null in atDivide");
                return 1;
            }

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
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
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
        {
            // Used to get the value of the variaiable in child1 (the variable to print)
            struct ValueHolder *valueToPrint = malloc(sizeof(struct ValueHolder));
            if (valueToPrint==NULL) {
                InterpreterError("Can't allocate memory for valueToPrint in atPrint");
                return 1;
            }

            if(InterpreteAST(ast->child1, valueToPrint, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) { // If managed to retrieve the value
                if (ast->child1->type == atConstant) {
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
                else { //Then it is a variable
                    struct VariableStruct* varStruct;
                    // Using the lazy evaluation to first look at local variables
                    if (!TryFind_Hashtable(localSymbolTable, valueToPrint->s, varStruct) && !TryFind_Hashtable(globalSymbolTable, valueToPrint->s, varStruct)) {
                        InterpreterError("No defined variable with this name (atPrint)");
                        FreeValueHolder(valueToPrint);
                        return 1;
                    }

                    switch(varStruct->type) {
                        case integer:
                            printf("%d\n", varStruct->i);
                        break;
                        case floating:
                            printf("%f\n", varStruct->f);
                        break;
                        case characters:
                            printf("%s\n", varStruct->s);
                        break;
                        default:
                            InterpreterError("Not a valid variable type to print");
                            FreeValueHolder(valueToPrint);
                            return 1;
                        break;
                    }
                }
            }
            else {
                InterpreterError("Could not get the value to print");
                FreeValueHolder(valueToPrint);
                return 1;
            }

            FreeValueHolder(valueToPrint);
            return 0;
            break;
        }
        default:
            InterpreterError("Node not valid");
            return 1;
        break;
    }
}