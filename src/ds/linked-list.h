#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef struct list_s {
    struct list_s *next;
} list_t;

typedef list_t node_t;

#define ll_next(node) (node)->next
#define ll_for_each(head, node, condition) \
	for ((node) = (head); (condition) ; (node) = ll_next((node)))

node_t *ll_create_node(void);
void ll_add_node(node_t **head, void* new_node);

#define ll_destroy(list, type, obj_name, ...) \
        do {                                  \
            node_t *tmp = NULL;               \
            for (tmp = list; tmp; ) {     \
                type *obj_name = (type*) tmp; \
                __VA_ARGS__;                  \
                tmp = tmp-> next;             \
                free(obj_name);               \
            }                                 \
        }                                     \
        while(0)
#endif