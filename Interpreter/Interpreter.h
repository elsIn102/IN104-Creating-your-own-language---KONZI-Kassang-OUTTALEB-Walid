#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include "../Utils/AST.h"

struct ValueHolder 
{
    enum VariableType variableType;

    int i;
    float f;
    char* s; 
};

void InterpreteAST (struct AstNode* ast, struct ValueHolder* outVal);

#endif