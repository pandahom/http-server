#include "ht.h"
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

static unsigned int hash_f(const char *str) {
    uint64_t hash = 5381;
    int c;
    while ( (c = tolower(*str++)) ) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash & (HT_CAPACITY - 1);
}

ht_t *ht_init(void) {
    ht_t *new_ht = (ht_t *) calloc(1, sizeof (ht_t));
    if (!new_ht) {
        fprintf(stderr, "Could not allocate memory (ht_t)...\n");
        return NULL;
    }
    return new_ht;
}


int ht_insert(ht_t *ht, char* key, char* val) {

    if (ht->count >= HT_CAPACITY) {
        fprintf(stderr, "HashTable[%p] does not have empty entry...\n", ht);
        return -1;
    }

    unsigned int hash_index;
    ht_entry_t *new_entry = NULL, *found = NULL;

    found = ht_get(ht, key);
    if (found) {
        found->value = (char *) realloc(found->value, strlen(found->value) + strlen(val) + 4);
        if (!found->value) {
            fprintf(stderr, "Could not reallocate memory for hash table value ...\n");
            return -1;
        }
        sprintf(found->value, "%s :: %s",found->value,val);
        return 0;
    }

    hash_index = hash_f(key);
    new_entry = (ht_entry_t *) calloc(1, sizeof (ht_entry_t));

    if (!new_entry) {
        fprintf(stderr, "Could not allocate memory (ht_entry_t) ...\n");
        return -1;
    }

    new_entry->key = strdup(key);
    if (!new_entry->key) {
        fprintf(stderr, "Could not allocate memory for key of hash table ... \n");
        return -1;
    }

    new_entry->value = strdup(val);
    if (!new_entry->value) {
        fprintf(stderr, "Could not allocate memory for value of hash table ... \n");
        free(new_entry->key);
        return -1;
    }

    ht->count++;
    ll_add_node((node_t **)&ht->buckets[hash_index], (void *) new_entry);
    return 0;
}

ht_entry_t* ht_get(ht_t *ht, char *key) {
    ht_entry_t *found = NULL, *iter = NULL;
    node_t *tmp = NULL;

    unsigned int hash_index = hash_f(key);
    tmp  =  (node_t *) ht->buckets[hash_index];

    ll_for_each(tmp, tmp, tmp) {
        iter = (ht_entry_t *) tmp;
        if (strcasecmp(iter->key, key) == 0) {
            found = iter;
            break;
        }
    }

    return found;
}


void ht_destroy(ht_t *ht) {
    node_t *iter = NULL;
    ht_entry_t *tmp = NULL;

    for (int i = 0; i < HT_CAPACITY; ++i) {
        iter = (node_t *) ht->buckets[i];
        while (iter) {
            tmp = (ht_entry_t *) iter;
            iter = iter->next;
            free(tmp->key);
            free(tmp->value);
            free(tmp);
        }
    }
//    free(ht);
}
