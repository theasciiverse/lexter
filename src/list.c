/* Copyright 1998,2001  Mark Pulford
 * This file is subject to the terms and conditions of the GNU General Public
 * License. Read the file COPYING found in this archive for details, or
 * visit http://www.gnu.org/copyleft/gpl.html
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include "list.h"

int list_length(struct list *head)
{
	int c = 0;

	if(!head)
		return 0;

	while(head) {
		c++;
		head = head->next;
	}

	return c;
}

struct list *list_element(void *data)
{
	struct list *p;

	p = malloc(sizeof(*p));
	if(!p)
		abort();

	p->data = data;
	p->next = NULL;

	return p;
}

void list_free(struct list *head, void (*freedata)(void*))
{
	struct list *p;

	while(head) {
		p = head->next;
		if(freedata)
			freedata(head->data);
		free(head);
		head = p;
	}
}

struct list *list_dup(struct list *head, void* (*dupdata)(void*))
{
	struct list *new, *p;
	void *d;

	if(!head)
		return NULL;

	new = list_element(dupdata(head->data));
	p = new;
	while(head->next) {
		d = head->next->data;
		if(dupdata)
			d = dupdata(d);
		p->next = list_element(d);
		p = p->next;
		head = head->next;
	}

	return new;
}

struct list *list_join(struct list *h1, struct list *h2)
{
	struct list *p;

	if(!h1)
		return h2;

	p = h1;			/* Scan to the end of the list */
	while(p->next)
		p = p->next;

	p->next = h2;		/* Append */

	return h1;
}

struct list *list_tail(struct list *head, int num, void (*freedata)(void*))
{
	struct list *p;
	int l = list_length(head);

	if(l<num)
		return head;		/* Nothing to do */

	l -= num;
	while(l--) {
		p = head->next;
		if(freedata)
			freedata(head->data);
		head = p;
	}

	return head;
}

struct list *list_add(struct list *head, void *data)
{
	return list_join(head, list_element(data));
}
