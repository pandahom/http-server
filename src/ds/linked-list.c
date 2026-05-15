#include "linked-list.h"
#include <stdlib.h>

node_t *ll_create_node(void) {
    node_t *node = (node_t *) malloc(sizeof(node_t));
    node->next   = NULL;

    return node;
}

void ll_add_node(node_t **head, void* lnode) {
    node_t *new_node = lnode;
    node_t *tmp  = *head;

    if (*head == NULL) {
        *head = new_node;
        return;
    }

    ll_for_each(*head, tmp, tmp->next != NULL);
    tmp->next = new_node;
}

