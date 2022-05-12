#include <stdlib.h>
#include <string.h>

#include "../Utils/AST.h"
#include "../Parser-Bison/UF-C.tab.h"
#include "../Utils/ComparisonDictionnary.h"
#include "../Translator/Translator.h"

// Flex: file to read from
extern FILE *yyin;
// Bison: parsing function
extern int yyparse();


int main(int argc, char* argv[]) 
{
    if (argc == 1)
    {
        printf("Error : Not enough arguments\n");
        return 1;
    }
    else if (argc > 2)
    {
        printf("Error : Too many arguments\n");
        return 1;
    }


    /**************** Opening the code file ***********************/

    char* fileName = argv[1];

    // open the input file
    FILE *myfile = fopen(fileName, "r");
    if (!myfile) 
    {
        printf("Cannot open in.ufc file\n");
        return 1;
    }


    /******************* Creating the AST ***********************/

    struct AstNode* ast;

    // Set flex to read from it instead of defaulting to STDIN:
    yyin = myfile;
    // Not zero to trace Bison states (debug)
    extern int yydebug;
    yydebug = 0;

    // Parse through the input and get the AST
    int error = yyparse(&ast);
    if (error != 0)
    {
        printf("Error during parsing\n");
        FreeAST(ast);
        fclose(myfile);
        return error;
    }
    // We don't need the input file anymore
    fclose(myfile);


    /**************** Creating the output '.c' file ********************/

    // Getting the name of the output file (removing the extension .ufc)
    char* outFileName;
    if ((outFileName = malloc (strlen(fileName) + 1)) == NULL)
    {
        printf("Can't create the output file name\n");
        return 1;
    }
    // Copy the name of the input file in outFileName
    strcpy(outFileName, fileName);

    // Find the position in memory of the '.'
    char* extension = strrchr(outFileName, '.');
    if (extension == NULL)
    {
        printf("Can't find any '.' in the name of the input file\n");
        free(outFileName);
        return 1;
    }
    // Assign 'end of char*' character at that position
    *extension = '\0';

    // Now we can concatenate because ".ufc" is longer than ".c" so no memory problem
    strcat(outFileName, ".c");


    // From here on, outputFileName is the name of the file with the .c extension
    // Creating the output file
    FILE* outFile = fopen(outFileName, "w");
    if (outFile==NULL)
    {
        printf("Can't create the output file\n");
        free(outFileName);
        return 1;
    }
    // Name of the output file useless now so we can free it
    free(outFileName);


    /************* Translating the AST into the output file ***************/

    if (TranslateAST (ast, outFile))
        printf("Error while translating the AST\n");
    
    // We don't need the AST anymore
    FreeAST(ast);


    /********************* Closing the outpur file **********************/

    fclose(outFile);

    return 0;
}