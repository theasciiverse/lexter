#ifndef IFACE_H
#define IFACE_H

#define PITWIDTH	10
#define PITDEPTH	21
#define MAXPITSTR	((PITWIDTH>PITDEPTH ? PITWIDTH:PITDEPTH)+1)

#include <sys/time.h>
#include <time.h>
#include "scores.h"

enum btype {NONE, NORMAL, HIGHLIGHT};

/* text.c or any other interface used */
void if_init();
void if_drawpit();
void if_putpit(char letter, enum btype bt, int x, int y);      /* pit coords */
void if_putscore(const struct score *scr, char next);
void if_putwords(const struct list *wordlist);
void if_showhelp();
void if_showletters();
void if_putmsg(const char *fmt, ...);
int if_getaction(struct timeval *delay);
void if_flushinput();
void if_refresh();	/* refresh the display */
void if_exit(char code);
void if_screenshot();
void if_highscore(const struct hiscore *hs, time_t hl);
void if_error(const char *fmt, ...);

#define NO_ACT		0
#define LEFT_ACT	1
#define RIGHT_ACT	2
#define DOWN_ACT	3	/* make piece fall faster */
#define DROP_ACT	4	/* drop all the way */
#define PAUSE_ACT	5
#define QUIT_ACT	6
#define HELP_ACT	7
#define SSHOT_ACT	8
#define LETTER_ACT	9
#define WORDL_ACT	10

#endif
