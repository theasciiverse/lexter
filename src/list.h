#ifndef LIST_H
#define LIST_H

struct list {
	void *data;
	struct list *next;
};

int list_length(struct list *head);
struct list *list_element(void *data);
void list_free(struct list *head, void (*freedata)(void*));
struct list *list_dup(struct list *head, void* (*dupdata)(void*));
struct list *list_join(struct list *h1, struct list *h2);
struct list *list_tail(struct list *head, int num, void (*freedata)(void*));
struct list *list_add(struct list *head, void *data);

#endif
