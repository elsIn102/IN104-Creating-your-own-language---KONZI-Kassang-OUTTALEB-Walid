#ifndef __DICTIONNARY_H__
#define __DICTIONNARY_H__

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

int CreateComparisonsDict(struct Comparisons_Dict** outDict);

void FreeComparisonsDict(struct Comparisons_Dict* dict);

int TryFind_ComparisonsDict(struct Comparisons_Dict* dict, int key, struct ComparisonValue **out);

int Add_ComparisonsDict(struct Comparisons_Dict **dict, int key, enum ComparatorType comparator, struct AstNode* value1, struct AstNode* value2);

#endif