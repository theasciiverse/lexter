#ifndef LEXTER_H
#define LEXTER_H

#define PIT_EMPTY_BONUS	100
#define LETTER_BONUS	25

#include <sys/time.h>
#include "scores.h"
#include "iface.h"

struct block {
	int x, y;
	char letter;
};

struct pit {
	char pit[PITWIDTH][PITDEPTH];
};

struct game {
	struct pit p;
	struct block blk;
	struct score scr;
	char next_char;
	int next_blank;
	struct timeval next_tick;
};

void game_run();
void game_load_dict(const char *dname);

#endif
