#ifndef __SYMBOL_TABLE_DATA_H__
#define __SYMBOL_TABLE_DATA_H__

#include "AST.h"

struct VariableStruct {
    char* id;

    int i;
    float f;
    char* s;

    // Pointer to the body of the function in the AST
    struct AstNode* functionBody;

    struct VariableStruct* nextInHash;
};


// Free the memory used by a VariableStruct
void FreeVariableStruct (struct VariableStruct* varStruct);

// This function steps through the list of VariableStruct to try and find one with an id matching the key
// If it found one, *ouVal will point to it and the function will return 0
// Otherwise the function returns 1
int TryFind_VariableStruct (struct VariableStruct* varStruct, char* key, struct VariableStruct** outVal);

#endif