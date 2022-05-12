#include <stdlib.h>

#include "Dictionnary.h"

// Creates a new dictionnary
int CreateComparisonsDict(struct Comparisons_Dict** outDict) {
    *outDict = malloc(sizeof(struct Comparisons_Dict));

    if (*outDict==NULL)
    {
        printf("Unable to allocate memory for the dictionnary\n");
        return 1;
    }

    return 0;
}

// Frees the dictionnary
void FreeComparisonsDict(struct Comparisons_Dict *dict) {
    if (dict==NULL)
        return;
    
    FreeComparisonsDict(dict->next);
    
    if (dict->value!=NULL)
        free(dict->value);

    free(dict);
}

// Tries to find the element in the dictionnary and returns 0 if found, 1 otherwise
// with the found value stored at out
int TryFind_ComparisonsDict(struct Comparisons_Dict *dict, int key, struct ComparisonValue **out) 
{
    if (dict==NULL)
    {
        printf("The dictionnary is empty\n");
        return 1;
    }

    struct Comparisons_Dict *ptr;

    for (ptr = dict; ptr != NULL; ptr = ptr->next) {
        if (ptr->key==key) {
            *out = ptr->value;
            return 0;
        }
    }
    
    return 1;
}

// Adds an element to the dictionnary and returns 0 if added successfully, 1 otherwise
int Add_ComparisonsDict(struct Comparisons_Dict **dict, int key, enum ComparatorType comparator, struct AstNode* value1, struct AstNode* value2) 
{
    if (dict==NULL)
    {
        printf("The dictionnary is empty\n");
        return 1;
    }
    
    if (TryFind_ComparisonsDict(*dict, key, NULL))
    {
        printf("An element with this key already exists in the dictionnary\n");
        return 1;
    }

    // Creation of the ComparisonValue element to store in the dictionnary
    struct ComparisonValue* value = malloc(sizeof(struct ComparisonValue));
    if (value==NULL)
    {
        printf("Unable to allocate memory for the comparison structure in the dictionnary\n");
        return 1;
    }

    value->comparator = comparator;
    value->value1 = value1;
    value->value2 = value2;

    // Creation of the entry in the dictionnary
    struct Comparisons_Dict *d = malloc(sizeof(struct Comparisons_Dict));
    d->key = key;
    d->value = value;
    d->next = *dict;
    *dict = d;
}