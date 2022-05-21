#include <stdlib.h>
#include <string.h>

#include "Interpreter.h"
#include "../Utils/Hash.h"
#include "../Utils/ComparisonDictionnary.h"
#include "../Utils/SymbolTableData.h"

#define InterpreterError(msg) InterpreterError_Expand(msg, __LINE__)

void InterpreterError_Expand(char* error_msg, const int line)
{
    printf("Error from the interpreter at line %d : %s\n", line, error_msg);
}

// Copy a char* from source to dest
// If dest is not NULL, free it first
// Return 0 if there was an error, 1 otherwise
int StrFreeAndCopy (char** dest, char* source) {
    if (source==NULL || source[0]=='\0') {
        printf("The source cannot be NULL or an empty string (StrFreeAndCopy)\n");
        return 0;
    }

    if (*dest!=NULL)
        free(dest);

    char* _dest = malloc(1 + strlen(source));
    if (_dest==NULL) {
        printf("Error while allocating memory for the destination (StrFreeAndCopy)\n");
        return 0;
    }

    strcpy(_dest, source);

    *dest = _dest;

    return 1;
}

int CreateValueHolder (struct ValueHolder** valHolder) {
    if (valHolder==NULL) {
        printf("Error : need a pointer to store the created ValueHolder\n");
        return 0;
    }

    struct ValueHolder* _valHolder = malloc(sizeof(struct ValueHolder));
    if (_valHolder == NULL) {
        printf("Could not allocate memory for _valHolder in CreateValueHolder\n");
        return 0;
    }
    _valHolder->s = NULL;

    *valHolder = _valHolder;

    return 1;
}

void FreeValueHolder (struct ValueHolder* value) {
    if (value==NULL)
        return;

    if (value->s!=NULL)
        free(value->s);
    
    free(value);
}

// Copy all the values of the symbol with symbolName key in SymbolTable into outVal
// Return 0 of an error was met, 1 otherwise
int GetSymbolValue (char* symbolId, struct ValueHolder** outVal, struct HashStruct* globalSymbolTable, struct HashStruct* localSymbolTable) {
    // Get a pointer to the variable
    struct VariableStruct* varStruct;
    // Using the lazy evaluation to first look at local variables
    if (!(localSymbolTable!=NULL && TryFind_Hashtable(localSymbolTable, symbolId, &varStruct)) && !TryFind_Hashtable(globalSymbolTable, symbolId, &varStruct)) {
        printf("No defined variable with this name (GetSymbolValue)\n");
        return 0;
    }

    (*outVal)->variableType = varStruct->type;
    (*outVal)->f = varStruct->f;
    (*outVal)->i = varStruct->i;

    if (varStruct->s!=NULL && varStruct->s[0]!='\0' && !StrFreeAndCopy(&((*outVal)->s), varStruct->s)) {
        printf("Error while copying varStruct->s into outVal->s in GetSymbolValue\n");
        return 0;
    }

    return 1;
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

int InterpreteAST (struct AstNode* ast, struct ValueHolder* outVal, struct HashStruct* globalSymbolTable, struct HashStruct* localSymbolTable, struct HashStruct* argsTable, struct ArgList* listOfArgs, struct ValueHolder* returnValue, struct Comparisons_Dict* comparisonDict) 
{
    if (ast==NULL)
        return 0;
    
    switch (ast->type)
    {
        case atRoot:
        {
            struct HashStruct* _globalSymbolTable = NULL;
            if (!Create_Hashtable(&globalSymbolTable)) {
                InterpreterError("Error while creating the global symbol table in atRoot");
                return 0;
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
                struct ValueHolder* stopEvaluationsHolder;
                if (!CreateValueHolder(&stopEvaluationsHolder)) {
                    InterpreterError("Error while creating the ValueHolder for stopEvaluationsHolder in atStatementList");
                    return 0;
                }

                stopEvaluationsHolder->variableType = integer;
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
        case atLogicalOr: // Set outVal->i to 1 if the statement is true, 0 otherwise
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the result of the OR evaluation : outVal is null in atLogicalOr");
                return 0;
            }

            struct ValueHolder* booleanAndHolder1;
            if (!CreateValueHolder(&booleanAndHolder1)) {
                InterpreterError("Error while creating the ValueHolder for booleanAndHolder1 in atLogicalOr");
                return 0;
            }

            struct ValueHolder* booleanAndHolder2;
            if (!CreateValueHolder(&booleanAndHolder2)) {
                InterpreterError("Error while creating the ValueHolder for booleanAndHolder2 in atLogicalOr");
                FreeValueHolder(booleanAndHolder1);
                return 0;
            }

            // Evaluate the boolean value of the left and right expressions
            if (!(InterpreteAST(ast->child1, booleanAndHolder1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, comparisonDict)
                    && InterpreteAST(ast->child2, booleanAndHolder2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, comparisonDict))) {
                InterpreterError("Error while evaluating the left or right expression of the OR comparison");
                FreeValueHolder(booleanAndHolder1);
                FreeValueHolder(booleanAndHolder2);
                return 0;
            }

            outVal->i = booleanAndHolder1->i || booleanAndHolder2->i;

            return 1;
            break;
        }
        case atLogicalAnd: // Set outVal->i to 1 if the statement is true, 0 otherwise
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the result of the AND evaluation : outVal is null in atLogicalAnd");
                return 0;
            }

            struct ValueHolder* booleanAndHolder1;
            if (!CreateValueHolder(&booleanAndHolder1)) {
                InterpreterError("Error while creating the ValueHolder for booleanAndHolder1 in atLogicalAnd");
                return 0;
            }

            struct ValueHolder* booleanAndHolder2;
            if (!CreateValueHolder(&booleanAndHolder2)) {
                InterpreterError("Error while creating the ValueHolder for booleanAndHolder2 in atLogicalAnd");
                FreeValueHolder(booleanAndHolder1);
                return 0;
            }

            // Evaluate the boolean value of the left and right expressions
            if (!(InterpreteAST(ast->child1, booleanAndHolder1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, comparisonDict)
                    && InterpreteAST(ast->child2, booleanAndHolder2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, comparisonDict))) {
                InterpreterError("Error while evaluating the left or right expression of the AND comparison");
                FreeValueHolder(booleanAndHolder1);
                FreeValueHolder(booleanAndHolder2);
                return 0;
            }

            outVal->i = booleanAndHolder1->i && booleanAndHolder2->i;

            return 1;
            break;
        }
        case atVariableDef:
        {
            struct ValueHolder* varIdHolder;
            if (!CreateValueHolder(&varIdHolder)) {
                InterpreterError("CError while creating the ValueHolder for varIdHolder in atVariableDef");
                return 0;
            }

            if (InterpreteAST(ast->child1, varIdHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) { // get the name of the variable
                struct VariableStruct* varValue;
                if (!CreateVariableStruct(&varValue)) {
                    InterpreterError("Error while creating the VariableStruct for varValue in atVariableDef");
                    FreeValueHolder(varIdHolder);
                    return 0;
                }

                // Fills the fields of varValue
                if (!StrFreeAndCopy(&varValue->id, varIdHolder->s)) {
                    InterpreterError("Error while copying varIdHolder->id into varValue->id in atVariableDef");
                    FreeValueHolder(varIdHolder);
                    FreeVariableStruct(varValue);
                    return 0;
                }
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
                        if (!StrFreeAndCopy(&varValue->s, ast->s)) {
                            InterpreterError("Error while copying ast->s into varValue->s in atVariableDef");
                            FreeValueHolder(varIdHolder);
                            FreeVariableStruct(varValue);
                            return 0;
                        }

                        return 1;
                        break;
                    default:
                        InterpreterError("Cannot define a variable with this type");
                        FreeValueHolder(varIdHolder);
                        FreeVariableStruct(varValue);
                        return 0;
                        break;
                }

                // Add varValue to the local hashtable if it exists, to the global one otherwise
                if ((localSymbolTable!=NULL && !Add_Hashtable(localSymbolTable, varValue->id, varValue))
                    || !Add_Hashtable(globalSymbolTable, varValue->id, varValue))
                {
                    InterpreterError("Error when adding the variable to the hashtable (atVariableDef)");
                    FreeValueHolder(varIdHolder);
                    FreeVariableStruct(varValue);
                    return 0;
                }
            }
            else {
                InterpreterError("Cannot get the Id of the variable in arVariableDef");
                FreeValueHolder(varIdHolder);
                return 0;
            }
            
            FreeValueHolder(varIdHolder);
            return 1;
            break;
        }
        case atFuncDef:
        {
            struct ValueHolder* funcIdHolder;
            if (!CreateValueHolder(&funcIdHolder)) {
                InterpreterError("Error while creating the ValueHolder for funcIdHolder in atFuncDef");
                return 0;
            }
            funcIdHolder->s = NULL;

            if (InterpreteAST(ast->child1, funcIdHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) // get the name of the function
            {
                if (funcIdHolder->variableType != characters)
                {
                    InterpreterError("Not a valid function Id");
                    FreeValueHolder(funcIdHolder);
                    return 0;
                }

                if (TryFind_Hashtable(globalSymbolTable, funcIdHolder->s, NULL)) { //If a function or a variable with this name has already been defined
                    InterpreterError("A function or a variable with this name already exists");
                    FreeValueHolder(funcIdHolder);
                    return 0;
                }

                struct HashStruct* _argsTable = NULL;
                struct ArgList* _listOfArgs = NULL;

                if (ast->child2->type != atVoid) // If the function requires arguments, fill the list and table
                {
                    
                    if (!Create_Hashtable(&_argsTable)) {
                        InterpreterError("Error while creating the _argsTable hashtable");
                        FreeValueHolder(funcIdHolder);
                        return 0;
                    }

                    if (!CreateArgList(&_listOfArgs)) {
                        InterpreterError("Error while creating the ArgList for _listOfArgs in atFuncDef");
                        FreeValueHolder(funcIdHolder);
                        Free_Hashtable(_argsTable);
                        return 0;
                    }

                    if (!InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, _argsTable, _listOfArgs, NULL, NULL)) // If all arguments of the function has not been defined successfully
                    {
                        InterpreterError("Error while defining the arguments during the function definition");
                        FreeValueHolder(funcIdHolder);
                        Free_Hashtable(_argsTable);
                        FreeArgList(_listOfArgs);
                        return 0;
                    }
                }

                // Create the structure of the function that will be stored in the global symbol table
                struct VariableStruct* funcStruct;
                if (!CreateVariableStruct(&funcStruct)) {
                    InterpreterError("Error while creating the VariableStruct for funcStruct in atFuncDef");
                    FreeValueHolder(funcIdHolder);
                    Free_Hashtable(_argsTable);
                    FreeArgList(_listOfArgs);
                    return 0;
                }

                // Set the parameters of the function
                if (!StrFreeAndCopy(&funcStruct->id, funcIdHolder->s)) {
                    InterpreterError("Error while copying funcIdHolder->s into funcStruct->id in atFuncDef");
                    FreeValueHolder(funcIdHolder);
                    Free_Hashtable(_argsTable);
                    FreeArgList(_listOfArgs);
                    return 0;
                }

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
                    return 0;
                }

            }
            else { // If couldn't get the name of the function
                InterpreterError("Can't get the Id of the function in atFuncDef");
                FreeValueHolder(funcIdHolder);
                return 0;
            }

            return 1;
            break;
        }
        case atTest:
            {
                struct Comparisons_Dict* compDict;
                if (!CreateComparisonsDict(&compDict)) { //If it failed to create the dictionnary
                    InterpreterError("Unable to create the dictionnary for the tests (atTest)");
                    return 0;
                }

                // Fills the dictionnary with all the comparisons
                if (!InterpreteAST(ast->child1, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, compDict)) {
                    InterpreterError("Error while adding the comparisons to the dictionnary");
                    FreeComparisonsDict(compDict);
                    return 0;
                }

                //Interpretes the if/else_if/else statements using the dicionnary
                if (!InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, compDict)) {
                    InterpreterError("Error in the if/else if/else statement (atTest)");
                    FreeComparisonsDict(compDict);
                    return 0;
                }

                FreeComparisonsDict(compDict);
                return 1;
                break;
            }
        case atComparisonDeclaration: // Adds the comparison to the dictionnary
        {
            if (ast->child1->type == integer || ast->child1->type == floating)
            {
                if (ast->child2->type != integer || ast->child2->type != floating)
                {
                    InterpreterError("Impossible to compare these variables (atComparisonDefinition): Incompatible variable types");
                    return 0;
                }
            }
            else if (ast->child1->type == characters)
            {
                if (ast->child2->type != characters)
                {
                    InterpreterError("Impossible to compare these variables (atComparisonDefinition): Incompatible variable types");
                    return 0;
                }
            }
            else {
                InterpreterError("Impossible to use a variable with no type in a comparison (atComparisonDeclaration)");
                return 0;
            }

            if (!Add_ComparisonsDict(&comparisonDict, ast->i, ast->comparator, ast->child1, ast->child2)) {
                InterpreterError("Could not add the comparison to the dictionnary");
                return 0;
            }

            return 1;
            break;
        }
        case atComparisonId: // Set outVal->i to 1 if the comparison is true, 0 otherwise
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the result of the comparison : outVal is null in atComparisonId");
                return 0;
            }

            struct ComparisonValue *comparison;
            if (!TryFind_ComparisonsDict(comparisonDict, ast->i, &comparison)) {
                char* msg;
                sprintf(msg, "Unable to find the comparison (match %d) in this dictionnary", ast->i);
                InterpreterError(msg);
                free(msg);
                return 0;
            }


            struct ValueHolder* var1Holder;
            if (!CreateValueHolder(&var1Holder)) {
                InterpreterError("Error while creating the ValueHolder for var1Holder in atComparisonId");
                return 0;
            }
            struct ValueHolder* var2Holder;
            if (!CreateValueHolder(&var2Holder)) {
                InterpreterError("Error while creating the ValueHolder for var2Holder in atComparisonId");
                FreeValueHolder(var1Holder);
                return 0;
            }
            // Get the value of the 2 members of the comparison
            if (!InterpreteAST(comparison->value1, var1Holder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)
                || !InterpreteAST(comparison->value2, var2Holder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) 
            {
                InterpreterError("Error while getting the value or id of the variables to compare in atComparisonId");
                FreeValueHolder(var1Holder);
                FreeValueHolder(var2Holder);
                return 0;
            }

            // If var1Holder is a variable, get its value and put it into var1Holder
            if (comparison->value1->type == atId && !GetSymbolValue(var1Holder->s, &var1Holder, globalSymbolTable, localSymbolTable)) {
                InterpreterError("Error while getting the value of var1Holder in atComparisonId");
                FreeValueHolder(var1Holder);
                FreeValueHolder(var2Holder);
                return 0;
            }

            // If var2Holder is a variable, get its value and put it into var2Holder
            if (comparison->value2->type == atId && !GetSymbolValue(var2Holder->s, &var2Holder, globalSymbolTable, localSymbolTable)) {
                InterpreterError("Error while getting the value of var2Holder in atComparisonId");
                FreeValueHolder(var1Holder);
                FreeValueHolder(var2Holder);
                return 0;
            }

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
                                return 0;
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
                                return 0;
                                break;
                        }
                    }
                    else {
                        InterpreterError("Impossible to compare these types of value");
                        FreeValueHolder(var1Holder);
                        FreeValueHolder(var2Holder);
                        return 0;
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
                                return 0;
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
                                return 0;
                                break;
                        }
                    }
                    else {
                        InterpreterError("Impossible to compare these types of value");
                        FreeValueHolder(var1Holder);
                        FreeValueHolder(var2Holder);
                        return 0;
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
                                return 0;
                                break;
                        }
                    }
                    else {
                        InterpreterError("Impossible to compare these types of value");
                        FreeValueHolder(var1Holder);
                        FreeValueHolder(var2Holder);
                        return 0;
                    }
                break;
                default:
                    InterpreterError("Imopssible to compare these types of value");
                    FreeValueHolder(var1Holder);
                    FreeValueHolder(var2Holder);
                    return 0;
                break;
            }

            FreeValueHolder(var1Holder);
            FreeValueHolder(var2Holder);
            return 1;
            break;
        }
        case atTestIfBranch: // If the condition is true, launches the atAssignment and set outVal->i to 1
        {
            struct ValueHolder* booleanValueHolder;
            if (!CreateValueHolder(&booleanValueHolder)) {
                InterpreterError("Error while creating the ValueHolder for booleanValueHolder in atTestIfBranch");
                return 0;
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
                return 0;
            }

            FreeValueHolder(booleanValueHolder);
            return 1;
            break;
        }
        case atTestElseIfBranch: // If the condition is true, launches the atAssignment and set outVal->i to 1
        {
            struct ValueHolder* booleanValueHolder;
            if (!CreateValueHolder(&booleanValueHolder)) {
                InterpreterError("Error while creating the ValueHolder for booleanValueHolder in atTestElseIfBranch");
                return 0;
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
                return 0;
            }

            FreeValueHolder(booleanValueHolder);
            return 1;
            break;
        }
        case atTestElseBranch:
        {
            InterpreteAST(ast->child1, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL);
            return 1;
            break;
        }
        case atAssignment:
        {
            if (ast->child1->type==atVoid && ast->child2->type==atFuncCall) { // then it's a call of a function without catching the return value
                InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL);
            }
            else
            {
                struct ValueHolder* varIdHolder;
                if (!CreateValueHolder(&varIdHolder)) {
                    InterpreterError("Error while creating the ValueHolder for varIdHolder in atAssignment");
                    return 0;
                }
                
                // Get the id of the variable to assign to
                if (InterpreteAST(ast->child1, varIdHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) 
                {
                    if (varIdHolder->variableType != characters) {
                        InterpreterError("Not a valid variable Id to assign to in atAssignment");
                        FreeValueHolder(varIdHolder);
                        return 0;
                    }

                    // Get a pointer to the variable to assign to in the hashtable
                    struct VariableStruct* varStruct;
                    // Using the lazy evaluation to first look at local variables
                    if (!(localSymbolTable!=NULL && TryFind_Hashtable(localSymbolTable, varIdHolder->s, &varStruct)) && !TryFind_Hashtable(globalSymbolTable, varIdHolder->s, &varStruct)) {
                        InterpreterError("No defined variable with this name (atAssignment)");
                        FreeValueHolder(varIdHolder);
                        return 0;
                    }

                    // The variable to get the value from
                    struct ValueHolder* valToAssign;
                    if (!CreateValueHolder(&valToAssign)) {
                        InterpreterError("Error while creating the ValueHolder for valToAssign in atAssignment");
                        FreeValueHolder(varIdHolder);
                        return 0;
                    }

                    // Get the value to assign
                    if (InterpreteAST(ast->child2, valToAssign, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) 
                    {
                        // If valToAssign is a variable, get its value and put it into valToAssign
                        if (ast->child2->type == atId && !GetSymbolValue(valToAssign->s, &valToAssign, globalSymbolTable, localSymbolTable)) {
                            InterpreterError("Error while getting the value of valToAssign in atAssignment");
                            FreeValueHolder(varIdHolder);
                            FreeValueHolder(valToAssign);
                            return 0;
                        }

                        // Check that the type of the variable and the valToAssign is matching
                        if (varStruct->type != valToAssign->variableType) {
                            InterpreterError("Type of the variable not matching the type of the other hand of the assignment");
                            FreeValueHolder(varIdHolder);
                            FreeValueHolder(valToAssign);
                            return 0;
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
                                if (!StrFreeAndCopy(&varStruct->s, valToAssign->s)) {
                                    InterpreterError("Error while copying valToAssign->s into varStruct->s in atAssignment");
                                    FreeValueHolder(varIdHolder);
                                    FreeValueHolder(valToAssign);
                                    return 0;
                                }

                                break;
                            default:
                                InterpreterError("Impossible to assign this type of variable (atAssignment)");
                                FreeValueHolder(varIdHolder);
                                FreeValueHolder(valToAssign);
                                return 0;
                                break;
                        }

                        FreeValueHolder(varIdHolder);
                        FreeValueHolder(valToAssign);
                        return 1;
                    }
                    else {
                        InterpreterError("Error while evaluating the right side of the assignment : Cannot get the value to assign in atAssignment");
                        FreeValueHolder(varIdHolder);
                        FreeValueHolder(valToAssign);
                        return 0;
                    }
                }
                else { 
                    InterpreterError("Can't get the id of the variable to assign to in atAssignment");
                    FreeValueHolder(varIdHolder);
                    return 0;
                }
            }

            return 1;
            break;
        }
        case atFuncCall:
        {
            struct ValueHolder* funcIdHolder;
            if (!CreateValueHolder(&funcIdHolder)) {
                InterpreterError("Error while creating the ValueHolder for funcIdHolder in atFuncCall");
                return 0;
            }

            if (InterpreteAST(ast->child1, funcIdHolder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) //If manages to retrieve the id of the function
            {
                if (funcIdHolder->variableType != characters) {
                    InterpreterError("Not a valid function Id");
                    FreeValueHolder(funcIdHolder);
                    return 0;
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
                            return 0;
                        }
                    }

                    // Call the function and return the output value
                    if (!InterpreteAST(funcVarStruct->functionBody, NULL, globalSymbolTable, funcVarStruct->argumentsTable, NULL, NULL, outVal, NULL)) { // If an error occurred while calling the function
                        InterpreterError("Error while calling the function");
                        FreeValueHolder(funcIdHolder);
                        return 0;
                    }
                }
                else {
                    InterpreterError("Call of an undefined function");
                    FreeValueHolder(funcIdHolder);
                    return 0;
                }
            }
            else {
                InterpreterError("Can't get the id of the function in atFuncCall");
                FreeValueHolder(funcIdHolder);
                return 0;
            }

            FreeValueHolder(funcIdHolder);

            return 1;
            break;
        }
        case atFuncCallArgList: // Get through the listOfArgs and assign their value to the localSymbolTable
        {
            // Make sure there is at least one more argument
            if (listOfArgs==NULL)
            {
                InterpreterError("Not enought arguments in the function call");
                return 0;
            }

            // Get a pointer to the argument wich value we need to assign in the local hashtable
            struct VariableStruct* foundArg;
            if (TryFind_Hashtable(argsTable, listOfArgs->id, &foundArg))
            {
                // Get the value to assign to the argument
                struct ValueHolder* argVal;
                if (!CreateValueHolder(&argVal)) {
                    InterpreterError("Error while creating the ValueHolder for argValIdHolder in atFuncCallArgList");
                    return 0;
                }

                if (InterpreteAST(ast->child1, argVal, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
                {
                    // If argVal is a variable, get its value and put it into argVal
                    if (ast->child1->type == atId && !GetSymbolValue(argVal->s, &argVal, globalSymbolTable, localSymbolTable)) {
                        InterpreterError("Error while getting the value of argVal in atFuncCallArgList");
                        FreeValueHolder(argVal);
                        return 0;
                    }

                    // Check that the types are matching
                    if (argVal->variableType != foundArg->type) {
                        InterpreterError("The type of the argument doesn't match the type defined in the function");
                        FreeValueHolder(argVal);
                        return 0;
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
                            if (!StrFreeAndCopy(&foundArg->s, argVal->s)) {
                                InterpreterError("Error while copying argVal->s into foundArg->s in atFuncCallArgList");
                                FreeValueHolder(argVal);
                                return 0;
                            }
                            break;            
                        default:
                            InterpreterError("Not a valid argument type");
                            FreeValueHolder(argVal);
                            return 0;
                            break;
                    }
                }
                else {
                    InterpreterError("Could not get the value of the argument");
                    FreeValueHolder(argVal);
                    return 0;
                }
            }
            else {
                InterpreterError("Failed to find the function argument in the hashtable : atFuncDef might not define the argumentsTable and the argumentsList properly");
                return 0;
            }

            // Get the rest of the arguments to be added to the localSymbolTable
            if (ast->child2!=NULL)
                return InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, argsTable, listOfArgs->next, NULL, NULL);

            return 1;
            break;
        }
        case atWhileLoop:
        {
            struct ValueHolder* comparisonResult;
            if (!CreateValueHolder(&comparisonResult)) {
                InterpreterError("Error while creating the ValueHolder for comparisonResult in atWhileLoop");
                return 0;
            }

            // Run the loop as long as comparisonResult->i == 0 ie as long as the condition is true
            for (InterpreteAST(ast->child1, comparisonResult, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL); comparisonResult->i; InterpreteAST(ast->child1, comparisonResult, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
            {
                // Run the body of the loop
                InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL);
            }

            FreeValueHolder(comparisonResult);
            return 1;
            break;
        }
        case atWhileCompare: // returns 1 in outVal->i if the comparison is true, 0 otherwise
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the comparison : outVal is null in atCompare");
                return 0;
            }

            outVal->variableType = integer;

            struct ValueHolder* var1Holder;
            if (!CreateValueHolder(&var1Holder)) {
                InterpreterError("Error while creating the ValueHolder for var1Holder in atCompare");
                return 0;
            }

            struct ValueHolder* var2Holder;
            if (!CreateValueHolder(&var2Holder)) {
                InterpreterError("Error while creating the ValueHolder for var2Holder in atCompare");
                FreeValueHolder(var1Holder);
                return 0;
            }

            if (InterpreteAST(ast->child1, var1Holder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL) && InterpreteAST(ast->child2, var2Holder, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) 
            {
                // If var1Holder is a variable, get its value and put it into var1Holder
                if (ast->child1->type == atId && !GetSymbolValue(var1Holder->s, &var1Holder, globalSymbolTable, localSymbolTable)) {
                    InterpreterError("Error while getting the value of var1Holder in atWhileCompare");
                    FreeValueHolder(var1Holder);
                    FreeValueHolder(var2Holder);
                    return 0;
                }

                // If var2Holder is a variable, get its value and put it into var2Holder
                if (ast->child2->type == atId && !GetSymbolValue(var2Holder->s, &var2Holder, globalSymbolTable, localSymbolTable)) {
                    InterpreterError("Error while getting the value of var2Holder in atWhileCompare");
                    FreeValueHolder(var1Holder);
                    FreeValueHolder(var2Holder);
                    return 0;
                }

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
                                    return 0;
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
                                    return 0;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 0;
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
                                    return 0;
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
                                    return 0;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 0;
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
                                    return 0;
                                    break;
                            }
                        }
                        else {
                            InterpreterError("Impossible to compare these types of value");
                            FreeValueHolder(var1Holder);
                            FreeValueHolder(var2Holder);
                            return 0;
                        }
                    break;
                    default:
                        InterpreterError("Imopssible to compare these types of value");
                        FreeValueHolder(var1Holder);
                        FreeValueHolder(var2Holder);
                        return 0;
                    break;
                }
            }
            else {
                InterpreterError("Could not get the value of the variables to compare (atWhileCompare)");
                FreeValueHolder(var1Holder);
                FreeValueHolder(var2Holder);
                return 0;
            }
            
            FreeValueHolder(var1Holder);
            FreeValueHolder(var2Holder);

            return 1;
            break;
        }
        case atBreak:
            // Need to implement
            return 1;
            break;
        case atReturn: // Assign the return value. The end of the flow is done in atStatementList since it is the only place where a return can exist
        {
            if(!InterpreteAST(ast->child1, returnValue, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) { // If could not evaluate the id or constant to return
                InterpreterError("Could not get the value to return");
                return 0;
            }

            // If returnValue is a variable, get its value and put it into returnValue
            if (ast->child1->type == atId && !GetSymbolValue(returnValue->s, &returnValue, globalSymbolTable, localSymbolTable)) {
                InterpreterError("Error while getting the value of returnValue in atReturn");
                return 0;
            }

            return 1;
            break;
        }
        case atContinue:
            // Need to implement
            return 1;
            break;
        case atId: // assigns outVal->s with the name of the variable or function
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the Id : outVal is null in atId");
                return 0;
            }

            if (!StrFreeAndCopy(&outVal->s, ast->s)) {
                InterpreterError("Error while copying ast->s into outVal->s in atId");
                return 0;
            }

            outVal->variableType = characters;

            return 1;
            break;
        }
        case atFuncDefArgsList: // Add the arguments to the hashtable argsTable and return the list of arguments in listOfArgs
        {
            // Add the argument to argsTable and fill the value of the head of listOfArgs
            if (InterpreteAST(ast->child1, NULL, globalSymbolTable, localSymbolTable, argsTable, listOfArgs, NULL, NULL))
            {
                if (ast->child2!=NULL) { // if there is another argument to define
                    // Create the next element in the list of arguments
                    struct ArgList* newArg;
                    if (!CreateArgList(&newArg)) {
                        InterpreterError("Error while creating the ArgList for newArg in atFuncDefArgsList");
                        return 0;
                    }

                    // Fill the tail of the list with the arguments
                    if (!InterpreteAST(ast->child2, NULL, globalSymbolTable, localSymbolTable, argsTable, newArg, NULL, NULL)) {
                        InterpreterError("Can't allocate memory for newArg in atFuncDefArgsList");
                        FreeArgList(newArg);
                        return 0;
                    }
                    // The head points towards the tail
                    listOfArgs->next = newArg;
                }
            }
            else {
                InterpreterError("Error while defining the argument of the function");
                return 0;
            }

            return 1;
            break;
        }
        case atFuncDefArg: // Add the argument to argsTable and set it in listOfArgs
        {
            struct ValueHolder* argId;
            if (!CreateValueHolder(&argId)) {
                InterpreterError("Error while creating the ValueHolder for argId in atFuncDefArg");
                return 0;
            }

            // Get the Id of the argument
            if (InterpreteAST(ast->child1, argId, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
            {
                // Make sure what we got is really an id
                if (argId->variableType!=characters) {
                    InterpreterError("Error while building the AST : the first child of atFuncDefArg is not an atId");
                    FreeValueHolder(argId);
                    return 0;
                }

                // Create and set up the argument
                struct VariableStruct* argValue;
                if (!CreateVariableStruct(&argValue)) {
                    InterpreterError("Error while creating the VariableStruct for argValue in atFuncDefArg");
                    FreeValueHolder(argId);
                    return 0;
                }

                if (!StrFreeAndCopy(&argValue->id, argId->s)) {
                    InterpreterError("Error while copying argId->s into argValue->id in atFuncDefArg");
                    FreeValueHolder(argId);
                    FreeVariableStruct(argValue);
                    return 0;
                }
                
                argValue->type = ast->variableType;

                // Add the argument to the argsTable hashtable
                switch (Add_Hashtable(argsTable, argId->s, argValue))
                {
                    case 2:
                        InterpreterError("An argument with this name has already been defined");
                        FreeValueHolder(argId);
                        FreeVariableStruct(argValue);
                        return 0;
                        break;
                    case 0:
                        InterpreterError("Could not add the argument to the hashtable");
                        FreeValueHolder(argId);
                        FreeVariableStruct(argValue);
                        return 0;
                        break;
                    case 1:
                        // Set the argument from listOfArgs
                        if (!StrFreeAndCopy(&listOfArgs->id, argId->s)) {
                            InterpreterError("Error while copying argId->s into listOfArgs->id in atFuncDefArg");
                            FreeValueHolder(argId);
                            return 0;
                        }

                        FreeValueHolder(argId);

                        return 1;
                        break;
                    default:
                        InterpreterError("Unknown error while trying to add the argument to the hashtable");
                        FreeValueHolder(argId);
                        FreeVariableStruct(argValue);
                        return 0;
                        break;
                }
            }
            else {
                InterpreterError("Could not get the id of the argument");
                FreeValueHolder(argId);
                return 0;
            }
            
            return 1;
            break;
        }
        case atConstant:
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the constant : outVal is null in atConstant");
                return 0;
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
                    if (!StrFreeAndCopy(&outVal->s, ast->s)) {
                        InterpreterError("Error while copying ast->s into outVal->id in atConstant");
                        return 0;
                    }
                    
                    break;
                default:
                    InterpreterError("Not a valid constant type");
                    return 0;
                    break;
            }

            return 1;
            break;
        }
        case atVoid:
        {
            if (outVal!=NULL)
                outVal->variableType = noType;

            return 1;
            break;
        }
        case atAdd:
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the addition : outVal is null in atAdd");
                return 0;
            }

            struct ValueHolder *value1;
            if (!CreateValueHolder(&value1)) {
                InterpreterError("Error while creating the ValueHolder for value1 in atAdd");
                return 0;
            }

            struct ValueHolder *value2;
            if (!CreateValueHolder(&value2)) {
                InterpreterError("Error while creating the ValueHolder for value2 in atAdd");
                FreeValueHolder(value1);
                return 0;
            }

            // If managed to get the value or id of both members of the operation
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
            {
                // If value1 is a variable, get its value and put it into value1
                if (ast->child1->type == atId && !GetSymbolValue(value1->s, &value1, globalSymbolTable, localSymbolTable)) {
                    InterpreterError("Error while getting the value of value1 in atAdd");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }

                // If value2 is a variable, get its value and put it into value2
                if (ast->child2->type == atId && !GetSymbolValue(value2->s, &value2, globalSymbolTable, localSymbolTable)) {
                    InterpreterError("Error while getting the value of value2 in atAdd");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }

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
                        return 0;
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
                        return 0;
                    }
                }
                else if(value1->variableType==characters && value2->variableType==characters)
                {
                    outVal->variableType=characters;

                    if (outVal->s!=NULL)
                        free(outVal->s);

                    outVal->s = malloc(1 + strlen(value1->s) + strlen(value2->s));
                    if (outVal->s == NULL) {
                        InterpreterError("Could not allocate memory for outVal->s in atAdd");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 0;
                    }

                    strcpy(outVal->s, value1->s);
                    strcat(outVal->s, value2->s);
                }
                else {
                    InterpreterError("Can't add these types of data");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }
            }
            else {
                InterpreterError("Could not get the value of the two members of the addition");
                FreeValueHolder(value1);
                FreeValueHolder(value2);
                return 0;
            }
            
            FreeValueHolder(value1);
            FreeValueHolder(value2);

            return 1;

            break;
        }
        case atMinus:
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the substraction : outVal is null in atMinus");
                return 0;
            }

            struct ValueHolder *value1;
            if (!CreateValueHolder(&value1)) {
                InterpreterError("Error while creating the ValueHolder for value1 in atMinus");
                return 0;
            }

            struct ValueHolder *value2;
            if (!CreateValueHolder(&value2)) {
                InterpreterError("Error while creating the ValueHolder for value2 in atMinus");
                FreeValueHolder(value1);
                return 0;
            }

            // If managed to get the value of both members of the operation
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
            {
                // If value1 is a variable, get its value and put it into value1
                if (ast->child1->type == atId && !GetSymbolValue(value1->s, &value1, globalSymbolTable, localSymbolTable)) {
                    InterpreterError("Error while getting the value of value1 in atMinus");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }

                // If value2 is a variable, get its value and put it into value2
                if (ast->child2->type == atId && !GetSymbolValue(value2->s, &value2, globalSymbolTable, localSymbolTable)) {
                    InterpreterError("Error while getting the value of value2 in atMinus");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }

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
                        return 0;
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
                        return 0;
                    }
                }
                else {
                    InterpreterError("Can't substract these types of data");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }
            }
            else {
                InterpreterError("Could not get the value of the two members of the substraction");
                FreeValueHolder(value1);
                FreeValueHolder(value2);
                return 0;
            }
            
            FreeValueHolder(value1);
            FreeValueHolder(value2);

            return 1;
            break;
        }
        case atMultiply:
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the multiplication : outVal is null in atMultiply");
                return 0;
            }

            struct ValueHolder *value1;
            if (!CreateValueHolder(&value1)) {
                InterpreterError("Error while creating the ValueHolder for value1 in atMultiply");
                return 0;
            }

            struct ValueHolder *value2;
            if (!CreateValueHolder(&value2)) {
                InterpreterError("Error while creating the ValueHolder for value2 in atMultiply");
                FreeValueHolder(value1);
                return 0;
            }

            // If managed to get the value of both members of the operation
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
            {
                // If value1 is a variable, get its value and put it into value1
                if (ast->child1->type == atId && !GetSymbolValue(value1->s, &value1, globalSymbolTable, localSymbolTable)) {
                    InterpreterError("Error while getting the value of value1 in atMultiply");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }

                // If value2 is a variable, get its value and put it into value2
                if (ast->child2->type == atId && !GetSymbolValue(value2->s, &value2, globalSymbolTable, localSymbolTable)) {
                    InterpreterError("Error while getting the value of value2 in atMultiply");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }

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
                        return 0;
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
                        return 0;
                    }
                }
                else {
                    InterpreterError("Can't multiply these types of data");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }
            }
            else {
                InterpreterError("Could not get the value of the two members of the multiplication");
                FreeValueHolder(value1);
                FreeValueHolder(value2);
                return 0;
            }
            
            FreeValueHolder(value1);
            FreeValueHolder(value2);

            return 1;
            break;
        }
        case atDivide:
        {
            if (outVal==NULL) {
                InterpreterError("No pointer to hold the value of the division : outVal is null in atDivide");
                return 0;
            }

            struct ValueHolder *value1;
            if (!CreateValueHolder(&value1)) {
                InterpreterError("Error while creating the ValueHolder for value1 in atDivide");
                return 0;
            }

            struct ValueHolder *value2;
            if (!CreateValueHolder(&value2)) {
                InterpreterError("Error while creating the ValueHolder for value2 in atDivide");
                FreeValueHolder(value1);
                return 0;
            }

            // If managed to get the value of both members of the operation
            if(InterpreteAST(ast->child1, value1, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL) 
                && InterpreteAST(ast->child2, value2, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL))
            {
                // If value1 is a variable, get its value and put it into value1
                if (ast->child1->type == atId && !GetSymbolValue(value1->s, &value1, globalSymbolTable, localSymbolTable)) {
                    InterpreterError("Error while getting the value of value1 in atDivide");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }

                // If value2 is a variable, get its value and put it into value2
                if (ast->child2->type == atId && !GetSymbolValue(value2->s, &value2, globalSymbolTable, localSymbolTable)) {
                    InterpreterError("Error while getting the value of value2 in atDivide");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }

                if(value2->variableType==integer)
                {
                    if (value2->i==0) {
                        InterpreterError("Division by 0");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 0;
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
                        return 0;
                    }
                }
                else if(value2->variableType==floating)
                {
                    outVal->variableType=floating;

                    if (value2->f==0) {
                        InterpreterError("Division by 0");
                        FreeValueHolder(value1);
                        FreeValueHolder(value2);
                        return 0;
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
                        return 0;
                    }
                }
                else {
                    InterpreterError("Can't divide these types of data");
                    FreeValueHolder(value1);
                    FreeValueHolder(value2);
                    return 0;
                }
            }
            else {
                InterpreterError("Could not get the value of the two members of the division");
                FreeValueHolder(value1);
                FreeValueHolder(value2);
                return 0;
            }
            
            FreeValueHolder(value1);
            FreeValueHolder(value2);

            return 1;
            break;
        }
        case atPrint:
        {
            // Used to get the value of the variaiable in child1 (the variable to print)
            struct ValueHolder *valueToPrint;
            if (!CreateValueHolder(&valueToPrint)) {
                InterpreterError("Error while creating the ValueHolder for valueToPrint in atPrint");
                return 0;
            }

            if(InterpreteAST(ast->child1, valueToPrint, globalSymbolTable, localSymbolTable, NULL, NULL, NULL, NULL)) // If managed to retrieve the value
            {
                // If valueToPrint is a variable, get its value and put it into valueToPrint
                if (ast->child1->type == atId && !GetSymbolValue(valueToPrint->s, &valueToPrint, globalSymbolTable, localSymbolTable)) {
                    InterpreterError("Error while getting the value of valueToPrint in atPrint");
                    FreeValueHolder(valueToPrint);
                    return 0;
                }
                
                switch(valueToPrint->variableType) {
                    case integer:
                        printf("%d", valueToPrint->i);
                    break;
                    case floating:
                        printf("%f", valueToPrint->f);
                    break;
                    case characters:
                        printf("%s", valueToPrint->s);
                    break;
                    default:
                        InterpreterError("Not a valid variable type to print");
                        FreeValueHolder(valueToPrint);
                        return 0;
                    break;
                }
            }
            else {
                InterpreterError("Could not get the value to print");
                FreeValueHolder(valueToPrint);
                return 0;
            }

            FreeValueHolder(valueToPrint);
            return 1;
            break;
        }
        default:
            InterpreterError("Node not valid");
            return 0;
        break;
    }
}