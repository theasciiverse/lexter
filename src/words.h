#ifndef WORDS_H
#define WORDS_H

#include "list.h"
#include "iface.h"
#include "lexter.h"

struct word {
	char str[MAXPITSTR];
	int score;
};

struct list *find_pit_words(const struct pit *p, struct pit *u);

#endif
