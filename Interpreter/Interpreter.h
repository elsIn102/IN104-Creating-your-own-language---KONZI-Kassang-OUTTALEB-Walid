#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include "../Utils/AST.h"

struct ValueHolder 
{
    int i;
    float f;
    char* s;
};

void InterpreteAST (struct AstNode* ast);

#endif