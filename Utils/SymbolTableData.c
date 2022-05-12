#include <stdlib.h>
#include "SymbolTableData.h"

// Free the memory used by a VariableStruct
void FreeVariableStruct (struct VariableStruct* varStruct) {
    if (varStruct==NULL)
        return;
    
    FreeVariableStruct(varStruct->nextInHash);

    if (varStruct->s!=NULL)
        free(varStruct->s);
    
    if (varStruct->id!=NULL)
        free(varStruct->id);

    free(varStruct);
}

// This function steps through the list of VariableStruct to try and find one with an id matching the key
// If it found one, *ouVal will point to it and the function will return 0
// Otherwise the function returns 1
int TryFind_VariableStruct (struct VariableStruct* varStruct, char* key, struct VariableStruct** outVal) {
    if (varStruct==NULL)
        return 1;
    
    if (strcmp(varStruct->id, key)) { // If we found the right element
        if (*outVal!=NULL)
            *outVal = varStruct;

        return 0;
    }

    return TryFind_VariableStruct(varStruct->nextInHash, key, outVal);

}

/*
// Copies the data of toCopy to value (not the id and the nextInHash)
void CopyVariableStructData (struct VariableStruct* value, struct VariableStruct* toCopy) {
    value->i = toCopy->i;
    value->f = toCopy->f;
    value->s = strdup(toCopy->s);
}
*/