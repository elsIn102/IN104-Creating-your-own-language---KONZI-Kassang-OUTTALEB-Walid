#include <stdlib.h>
#include "Hash.h"

// Hash function for char* named djb2
unsigned long djb2_hash (char *str) {
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}


// Allocate memory for a new hashtable of size HASH_TABLE_SIZE
int Create_Hashtable (struct HashStruct** hashtable) {
    struct HashStruct* hash = malloc(sizeof(struct HashStruct));

	if (hash==NULL) {
        printf("Unable to allocate memory for the hash struct\n");
		return 1;
    }

    hash->table = malloc(sizeof(struct VariableStruct*) * HASH_TABLE_SIZE);

	if(hash->table== NULL) {
        printf("Unable to allocate memory for the hashtable\n");
		return 1;
	}

	for(int i = 0; i<HASH_TABLE_SIZE; i++)
		hash->table[i] = NULL;

	hash->size = HASH_TABLE_SIZE;

    *hashtable = hash;
	return 0;
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
// Returns 0 if an element was found and foundValue points to that element
// Returns 1 otherwise
int TryFind_Hashtable (struct HashStruct* hashtable, char* key, struct VariableStruct** foundValue) {
    if (hashtable==NULL) {
        printf("Can't find a key in a null hashtable\n");
        return 1;
    }

    unsigned long pos = djb2_hash(key) % hashtable->size;
    if (!TryFind_VariableStruct(hashtable->table[pos], key, foundValue))
        return 1;

    return 0;
}

// This function adds a key/value pair to the table if the key doesn't already exist
// It returns 0 if the pair was added, 1 if there was an error and 2 if the key already existed in the hashtable
int Add_Hashtable (struct HashStruct* hashtable, char* key, struct VariableStruct* value) {
    if (hashtable==NULL) {
        printf("Can't add a key/value pair to a null hashtable\n");
        return 1;
    }

    unsigned long pos = djb2_hash(key) % hashtable->size;
    struct VariableStruct* foundValue;

    // If an element with this key already exists in the hashtable
    if (TryFind_VariableStruct(hashtable->table[pos], key, &foundValue)) {
        printf("An element with this key already exists in the table");
        return 2;
    }

    value->nextInHash = hashtable->table[pos];
    hashtable->table[pos] = value;

    
    return 0;
}
