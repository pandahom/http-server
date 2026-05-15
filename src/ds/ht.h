#ifndef HASH_H
#define HASH_H

#include "linked-list.h"
#define HT_CAPACITY 64

typedef struct entry_s {
    node_t next; // Handling collision by this
    char  *key;
    char  *value;
} ht_entry_t;

typedef struct {
    ht_entry_t *buckets[HT_CAPACITY];
    int         count;
} ht_t;

ht_t *ht_init(void);
int ht_insert(ht_t *ht, char* key, char* val);
ht_entry_t* ht_get(ht_t *ht, char *key);
void ht_destroy(ht_t *ht);

#endif