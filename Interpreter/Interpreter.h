#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include "../Utils/AST.h"
#include "../Utils/ComparisonDictionnary.h"
#include "../Utils/Hash.h"
#include "../Utils/SymbolTableData.h"

struct ValueHolder 
{
    enum VariableType variableType;

    int i;
    float f;
    char* s;
};

int InterpreteAST (struct AstNode* ast, struct ValueHolder* outVal, struct HashStruct* globalSymbolTable, struct HashStruct* localSymbolTable, struct HashStruct* argsTable, struct ArgList* listOfArgs, struct valueHolder* returnValue, struct Comparisons_Dict* comparisonDict);

#endif