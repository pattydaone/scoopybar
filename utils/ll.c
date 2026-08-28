#include "ll.h"

#include <stdlib.h>

void
LL_push_back_output(struct output_node **head, struct output *data)
{
    struct output_node *node = malloc(sizeof(struct output_node));

    node->data = data;
    node->next = NULL;

    if (*head == NULL) {
        *head = node;
        return;
    }
    struct output_node *current = *head;
    while (current->next != NULL) {
        current = current->next;
    }

    (*head)->next = node;
}

void 
LL_delete_output(struct output_node **head, struct output_node *data)
{
    if (*head == data) {
        *head = data->next;
        free(data);
        return;
    }

    struct output_node *prev;
    struct output_node *cur = *head;
    while (cur != data && cur != NULL) {
        prev = cur;
        cur = cur->next;
    }
    
    if (cur == NULL)
        return;

    prev->next = cur->next;

    free(cur);
}
