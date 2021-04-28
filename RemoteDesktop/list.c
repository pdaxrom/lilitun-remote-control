#include <stdlib.h>
#include "list.h"

struct list_item {
    void *data;
    struct list_item *prev;
};

struct list_item *list_add_data(struct list_item **list_items, void *data)
{
    struct list_item *new = (struct list_item *)malloc(sizeof(struct list_item));
    if (!new) {
	return NULL;
    }
    new->data = data;
    new->prev = *list_items;

    *list_items = new;

    return new;
}

struct list_item *list_remove_data(struct list_item **list_items, void *data)
{
    struct list_item *ptr = *list_items;
    struct list_item *next = NULL;

    while (ptr) {
	if (ptr->data == data) {
	    struct list_item *tmp = ptr->prev;

	    free(ptr);

	    if (next) {
		next->prev = tmp;
	    } else {
		*list_items = tmp;
	    }

	    return tmp;
	}
	next = ptr;
	ptr = ptr->prev;
    }

    return NULL;
}

struct list_item *list_next_item(struct list_item *list_items)
{
    return list_items->prev;
}

void *list_get_data(struct list_item *list_items)
{
    return list_items->data;
}
