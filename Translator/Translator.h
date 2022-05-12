#ifndef __TRANSLATOR_H__
#define __TRANSLATOR_H__

#define VAR_TEMP_NAME "_varTemp"
#define FUNC_TEMP_NAME "_funcTemp"
#define MAIN_TEMP_NAME "_mainTemp"

#include <stdio.h>
#include "../Utils/AST.h"

int TranslateAST (struct AstNode* ast, FILE* outFile);

#endif