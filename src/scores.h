#ifndef SCORES_H
#define SCORES_H

#include <time.h>

#define SCORE_ENTRIES	20

#define HI_NAME_LEN	9
#define HI_DICT_LEN	9

struct hiscore {
	char name[HI_NAME_LEN];	/* null terminated username */
	char dict[HI_DICT_LEN];
	int total;
	int words;
	int blocks;
	time_t date;
};

struct score {
	int total;		/* Total score so far */
	int words;		/* Total words made so far */	
	int blocks;		/* Number of blocks that have fallen */
	struct list *wrd;
};

struct hiscore *score_update(const struct score *scr, time_t *scr_time, const char **err);

#endif
