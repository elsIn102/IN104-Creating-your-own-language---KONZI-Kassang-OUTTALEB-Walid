#ifndef __COMPARISON_DICTIONNARY_H__
#define __COMPARISON_DICTIONNARY_H__

#include "AST.h"


struct ComparisonValue
{
    enum ComparatorType comparator;

    // The values to compares, of type atId or atConstant
    struct AstNode* value1;
    struct AstNode* value2;
};

struct Comparisons_Dict
{
    int key;
    struct ComparisonValue *value;
    struct Comparisons_Dict *next;
};

// Creates a new dictionnary
// Return 1 if it was created successfully, 0 otherwise
int CreateComparisonsDict(struct Comparisons_Dict** outDict);

// Frees the dictionnary
void FreeComparisonsDict(struct Comparisons_Dict* dict);

// Tries to find the element in the dictionnary and returns 1 if found, 0 otherwise
// with the found value stored at out
int TryFind_ComparisonsDict(struct Comparisons_Dict* dict, int key, struct ComparisonValue **out);

// Adds an element to the dictionnary and returns 1 if added successfully, 0 otherwise
int Add_ComparisonsDict(struct Comparisons_Dict **dict, int key, enum ComparatorType comparator, struct AstNode* value1, struct AstNode* value2);

#endif