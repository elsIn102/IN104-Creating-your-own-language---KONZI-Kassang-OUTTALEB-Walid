#include <stdlib.h>

#include "Hash.h"
#include "SymbolTableData.h"

// Hash function for char* named djb2
unsigned long djb2_hash (char *str) {
    if (str == NULL)
        printf("djb2 algorithme requires a char*\n");
        
    unsigned long hash = 5381;
    int c;

    while ((c = *str++) != 0)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}


// Allocate memory for a new hashtable of size HASH_TABLE_SIZE
int Create_Hashtable (struct HashStruct** hashtable) {
    struct HashStruct* hash = malloc(sizeof(struct HashStruct));

	if (hash==NULL) {
        printf("Unable to allocate memory for the hash struct\n");
		return 0;
    }

    hash->table = malloc(sizeof(struct VariableStruct*) * HASH_TABLE_SIZE);

	if(hash->table== NULL) {
        printf("Unable to allocate memory for the hashtable\n");
		return 0;
	}

	for(int i = 0; i<HASH_TABLE_SIZE; i++)
		hash->table[i] = NULL;

	hash->size = HASH_TABLE_SIZE;

    *hashtable = hash;
	return 1;
}

// Free the memory used by the hashtable
void Free_Hashtable (struct HashStruct* hashtable) {
    if (hashtable==NULL)
        return;
    
    for (int i = 0; i<hashtable->size; i++)
        FreeVariableStruct(hashtable->table[i]);
    
    free(hashtable->table);

    free(hashtable);
}

// Tries to find an element with the key in the hashtable
// Returns 1 if an element was found and foundValue points to that element
// Returns 0 otherwise
int TryFind_Hashtable (struct HashStruct* hashtable, char* key, struct VariableStruct** foundValue) {
    if (hashtable==NULL || hashtable->table == NULL) {
        printf("Can't find a key in a null hashtable\n");
        return 0;
    }

    if (key==NULL)
    {
        printf("A key value is needed to find in the hashtable\n");
        return 0;
    }

    unsigned long pos = djb2_hash(key) % hashtable->size;

    return TryFind_VariableStruct(hashtable->table[pos], key, foundValue);
}

// This function adds a key/value pair to the table if the key doesn't already exist
// It returns 1 if the pair was added, 0 if there was an error and 2 if the key already existed in the hashtable
int Add_Hashtable (struct HashStruct* hashtable, char* key, struct VariableStruct* value) {
    if (hashtable==NULL || hashtable->table==NULL) {
        printf("Can't add a key/value pair to a null hashtable\n");
        return 0;
    }

    if (key==NULL || key[0]=='\0')
    {
        printf("A key value is needed to add to the hashtable\n");
        return 0;
    }

    unsigned long pos = djb2_hash(key) % hashtable->size;
    struct VariableStruct* foundValue;

    // If an element with this key already exists in the hashtable
    if (TryFind_VariableStruct(hashtable->table[pos], key, &foundValue)) {
        printf("An element with the key %s already exists in the table\n", key);
        return 2;
    }

    value->nextInHash = hashtable->table[pos];
    hashtable->table[pos] = value;

    
    return 1;
}
