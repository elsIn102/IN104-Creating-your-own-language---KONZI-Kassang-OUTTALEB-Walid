#include <stdlib.h>
#include <string.h>

#include "SymbolTableData.h"
#include "Hash.h"

int CreateArgList (struct ArgList** argList)
{
    if (argList==NULL) {
        printf("Error : need a pointer to store the created ArgList\n");
        return 0;
    }

    struct ArgList* _argList = malloc(sizeof(struct ArgList));
    if (_argList == NULL) {
        printf("Could not allocate memory for _argList in CreateArgList\n");
        return 0;
    }
    _argList->id = NULL;
    _argList->next = NULL;

    *argList = _argList;

    return 1;
}

int CreateVariableStruct (struct VariableStruct** varStruct)
{
    if (varStruct==NULL) {
        printf("Error : need a pointer to store the created VariableStruct\n");
        return 0;
    }

    struct VariableStruct* _varStruct = malloc(sizeof(struct VariableStruct));
    if (_varStruct == NULL) {
        printf("Could not allocate memory for _varStruct in CreateVariableStruct\n");
        return 0;
    }
    _varStruct->id = NULL;
    _varStruct->s = NULL;
    _varStruct->argumentsTable = NULL;
    _varStruct->argumentsList = NULL;
    _varStruct->functionBody = NULL;
    _varStruct->nextInHash = NULL;

    *varStruct = _varStruct;

    return 1;
}

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

    if (varStruct->id==NULL || varStruct->id[0]=='\0') // If there is no id
    {
        printf("Error : this VariableStruct has no id (TryFind_VariableStruct)\n");
        return 0;
    }
    
    if (strcmp(varStruct->id, key)) { // If we found the right element
        if (*outVal!=NULL)
            *outVal = varStruct;

        return 1;
    }

    return TryFind_VariableStruct(varStruct->nextInHash, key, outVal);
}