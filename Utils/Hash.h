#ifndef __HASH_H__
#define __HASH_H__

#include "SymbolTableData.h"

#define HASH_TABLE_SIZE 20


// Structure of a hashtable
struct HashStruct {
    unsigned int size;
    struct VariableStruct** table;
};


// Allocate memory for a new hashtable of size HASH_TABLE_SIZE
int Create_Hashtable (struct HashStruct** hashtable);

// Free the memory used by the hashtable
void Free_Hashtable(struct HashStruct* hashtable);

// Tries to find an element with the key in the hashtable
// Returns 1 if an element was found and foundValue points to that element
// Returns 0 otherwise
int TryFind_Hashtable (struct HashStruct* hashtable, char* key, struct VariableStruct** foundValue);

// This function adds a key/value pair to the table if the key doesn't already exist
// It returns 1 if the pair was added, 0 if there was an error and 2 if the key already existed in the hashtable
int Add_Hashtable (struct HashStruct* hashtable, char* key, struct VariableStruct* value);

#endif