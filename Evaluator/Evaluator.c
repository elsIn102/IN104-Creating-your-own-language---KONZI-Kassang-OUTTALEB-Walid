#include "../Utils/AST.h"

int main() {
  // open a file handle to a particular file:
  FILE *myfile = fopen("in.ufc", "r");
  // make sure it's valid:
  if (!myfile) {
    printf("I can't open in.ufc file!\n");
    return -1;
  }
  // Set flex to read from it instead of defaulting to STDIN:
  yyin = myfile;

  struct AstNode* _ast;

  // Parse through the input:
  int error = yyparse(&_ast);
  if (error != 0)
  {
    printf("Error during parsing\n");
    return 1;
  }

  FILE* outFile = fopen("out.ufc", "w");
  if (outFile!=NULL)
  {
    AstToCode(_ast, outFile);
    fclose(outFile);
  }

  FreeAST(_ast);

  return 0;
}