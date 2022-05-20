#ifndef __SYMBOL_TABLE_DATA_H__
#define __SYMBOL_TABLE_DATA_H__

#include "AST.h"

struct VariableStruct {
    char* id;

    enum VariableType type;
    int i;
    float f;
    char* s;

    /******Used to define or call a function******/

    // Pointer to the hashtable of the local variables used as arguments of the function
    struct HashStruct* argumentsTable;
    // Ordered list of the arguments of the function
    struct ArgList* argumentsList;
    // Pointer to the body of the function in the AST
    struct AstNode* functionBody;

    /*********************************************/

    struct VariableStruct* nextInHash;
};

struct ArgList {
    char* id;

    struct ArgList* next;
};


// Free the memory used by a VariableStruct
void FreeVariableStruct (struct VariableStruct* varStruct);

void FreeArgList(struct ArgList* argList);

// This function steps through the list of VariableStruct to try and find one with an id matching the key
// If it found one, *ouVal will point to it and the function will return 1
// Otherwise the function returns 0
int TryFind_VariableStruct (struct VariableStruct* varStruct, char* key, struct VariableStruct** outVal);

#endif