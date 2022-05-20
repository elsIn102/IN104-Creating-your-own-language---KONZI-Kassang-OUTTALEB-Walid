#include <stdlib.h>
#include "SymbolTableData.h"

void FreeArgList(struct ArgList* argList) {
    if (argList==NULL)
        return;

    if (argList->id!=NULL)
        free(argList->id);

    FreeArgList(argList->next);

    free(argList);
}

// Free the memory used by a VariableStruct
void FreeVariableStruct (struct VariableStruct* varStruct) {
    if (varStruct==NULL)
        return;
    
    FreeVariableStruct(varStruct->nextInHash);

    if (varStruct->s!=NULL)
        free(varStruct->s);
    
    if (varStruct->id!=NULL)
        free(varStruct->id);
    
    FreeArgList(varStruct->argumentsList);
    Free_Hashtable(varStruct->argumentsTable);

    // functionBody will be freed with the ast (to avoid a double free)

    free(varStruct);
}

// This function steps through the list of VariableStruct to try and find one with an id matching the key
// If it found one, *ouVal will point to it and the function will return 1
// Otherwise the function returns 0
int TryFind_VariableStruct (struct VariableStruct* varStruct, char* key, struct VariableStruct** outVal) {
    if (varStruct==NULL)
        return 0;
    
    if (strcmp(varStruct->id, key)) { // If we found the right element
        if (*outVal!=NULL)
            *outVal = varStruct;

        return 1;
    }

    return TryFind_VariableStruct(varStruct->nextInHash, key, outVal);
}