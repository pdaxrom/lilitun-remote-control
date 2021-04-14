#ifndef __LIST_H__
#define __LIST_H__

//struct list_item;

struct list_item *list_add_data(struct list_item **list_items, void *data);
struct list_item *list_remove_data(struct list_item **list_items, void *data);
struct list_item *list_next_item(struct list_item *list_items);
void *list_get_data(struct list_item *list_items);

#endif
