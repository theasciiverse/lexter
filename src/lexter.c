/* Copyright 1998,2001  Mark Pulford
 * This file is subject to the terms and conditions of the GNU General Public
 * License. Read the file COPYING found in this archive for details, or
 * visit http://www.gnu.org/copyleft/gpl.html
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#include "lexter.h"
#include "words.h"
#include "iface.h"
#include "scores.h"
#include "lang.h"
#include "dict.h"

static void game_end(const struct game *g);
static void game_redraw(const struct game *g);
static void tick_reset(struct game *g);
static int calc_delay(const struct score *s);
static void score_add_wordlist(struct score *s, struct list *wl);
static void score_reset(struct score *scr);
static int block_falling(struct game *g);
static int create_block(struct game *g);
static int handle_action(struct game *g, int act);
static int move_block(struct game *g, int rx, int ry);
static void pit_process_words(struct game *g);
static int pit_gravity(struct pit *p);
static int pit_empty(const struct pit *p);
static void pit_highlight(const struct pit *p, const struct pit *sel);
static void pit_blank();
static void pit_clear_selected(struct pit *p, const struct pit *sel);
static void getlangenvn(char *env, char *name, int len);
static char *find_language();
static char next_letter(struct game *g);
static void time_sub(struct timeval *r, struct timeval *a, struct timeval *b);

static void score_reset(struct score *scr)
{
	scr->total = 0;
	scr->words = 0;
	scr->blocks = 0;
	scr->wrd = NULL;
}

static char next_letter(struct game *g)
{
	char ch;

	if(g->next_blank) {
		g->next_blank = 0;
		return ' ';
	}
	
	ch = g->next_char;
	g->next_char = letter_generate(13);	/* ~1/8 for blank */
	if(' ' == g->next_char) {
		g->next_char = letter_generate(0);
		g->next_blank = 1;
	}

	return ch;
}

void game_run()
{
	struct game g;

	/* Setup lexter data */
	memset(g.p.pit, 0, PITWIDTH*PITDEPTH);
	score_reset(&g.scr);

	g.next_blank=0;
	next_letter(&g);

	if_putscore(&g.scr, g.next_char);
	if_refresh();

	while(create_block(&g) && block_falling(&g));

	game_end(&g);
}

/* C & posix are not valid langs, return nothing and let the caller
 * find a sane language */
static void getlangenvn(char *env, char *name, int len)
{
	char *e;

	memset(env, 0, len);
	e = getenv(name);
	if(e && strcmp(e, "C") && strcmp(e, "posix"))
		strncpy(env, e, len-1);
}

/* Returns the 2 letter language code */
static char *find_language()
{
	static char env[3];

	getlangenvn(env, "LANGUAGE", 3);
	if(!env[0])
		getlangenvn(env, "LC_ALL", 3);
	if(!env[0])
		getlangenvn(env, "LC_MESSAGES", 3);
	if(!env[0])
		getlangenvn(env, "LANG", 3);
	if(!env[0])
		strcpy(env, "en");

	return env;
}

/* Load dictionary and prep letter frequency tables */
void game_load_dict(const char *dname)
{
	const char *err;
	int def = 0;

	/* Default dictionary is the local language */
	if(!dname) {
		dname = find_language();
		def = 1;
	}

	/* Fall back to english dictionary if the requested dictionary cannot
	 * be found. */
	if(!dict_load(dname, &err)) {
		if(def && dict_load("en", NULL)) {
			if_error(_("Unable to load %s dictionary, using en instead"), dname);
		} else {
			if_error(_("Dictionary load: %s"), err);
			if_exit(-1);
		}
	}

	letter_update_freq();
}

/* do whatever is needed before quitting.. saving highscores etc.. */
static void game_end(const struct game *g)
{
	struct hiscore *hs;
	const char *err;
	time_t hl;

	hs = score_update(&g->scr, &hl, &err);
	if(!hs) {
		if_error(err);
		if_exit(-1);
	}

	if_putmsg(_("Press a key to view the high score table"));
	if_getaction(NULL);
	if_putmsg("");

	if_highscore(hs, hl);
	free(hs);
	if(err) {
		if_error(err);
	} else {
		if_putmsg(_("Press a key to quit"));
		if_getaction(NULL);
	}
	if_exit(0);
}

static void tick_reset(struct game *g)
{
	int delay;

	delay = calc_delay(&g->scr);

	gettimeofday(&g->next_tick, NULL);

	g->next_tick.tv_usec += delay;
	if(g->next_tick.tv_usec >= 1000000) {
		g->next_tick.tv_sec += g->next_tick.tv_usec/1000000;
		g->next_tick.tv_usec %= 1000000;
	}
}

static int calc_delay(const struct score *s)
{
	int d;

	/* Minimum delay after 100 words */
	d = 750000 - (s->words*6000);
	d = d<50000 ? 50000 : d;

	return d;
}

static void score_add_wordlist(struct score *s, struct list *wl)
{
	struct list *p = wl;
	struct word *w;

	while(p) {
		w = (struct word *)p->data;
		s->total += w->score;
		s->words++;
		p = p->next;
	}
	s->wrd = list_join(s->wrd, wl);
	s->wrd = list_tail(s->wrd, 16, free);
}

/* Returns: 0 - end game
 *          1 - continue game */
static int block_falling(struct game *g)
{
	struct timeval curr_tick, delay_tick;
	int act;
	int pstopped = 0;	/* block has stopped moving */

	tick_reset(g);
	if_flushinput();

	while(!pstopped) {
		/* Calculate time till next drop */
		gettimeofday(&curr_tick, NULL);
		if( timercmp(&curr_tick, &g->next_tick, >) ) {
			tick_reset(g);
			/* Move block down 1 if possible */
			pstopped = !move_block(g, 0, 1);
		}
		if(!pstopped) {
			/* Get key and act on it */
			time_sub(&delay_tick, &g->next_tick, &curr_tick);
			act = if_getaction(&delay_tick);
			pstopped = handle_action(g, act);
		}
	}
	/* block can no longer fall - freeze block */
	if_putmsg("");
	if(' ' == g->blk.letter) {
		if_putmsg(_("No letter chosen, picking a random one!"));
		g->blk.letter = letter_generate(0);	/* No blanks */
		move_block(g, 0, 0);
	}

	g->p.pit[g->blk.x][g->blk.y] = g->blk.letter;
	g->scr.blocks++;

	pit_process_words(g);

	return 1;
}

static void pit_process_words(struct game *g)
{
	int stotal, swords;
	struct pit used;
	struct list *w;

	/* Find words, update the pit and the score */
	stotal = g->scr.total;
	swords = g->scr.words;
	do {
		w = find_pit_words(&g->p, &used);
		if(w) {
			score_add_wordlist(&g->scr, w);
			pit_highlight(&g->p, &used);
			if_putscore(&g->scr, g->next_char);
			if_putwords(g->scr.wrd);
			sleep(1);
		}
		pit_clear_selected(&g->p, &used);
		if_refresh();
		while(pit_gravity(&g->p));
	} while(w);
	if(g->scr.total - stotal) {
		char buf[80];
		int dw, dt;

		dw = g->scr.words - swords;
		dt = g->scr.total - stotal;
		if(1 == dw)
			snprintf(buf, 80, _("Found 1 word for %d points."), dt);
		else
			snprintf(buf, 80, _("Found %d words for %d points."), dw, dt);
		if(pit_empty(&g->p)) {
			g->scr.total += PIT_EMPTY_BONUS;
			strncat(buf, _(" Pit empty bonus!"), 80-strlen(buf));
		}
		if_putmsg(buf);
	}
}

/* Returns: 1 - block fixed
 *          0 - block not fixed */
static int handle_action(struct game *g, int act)
{
	int pstopped = 0;

	switch(act) {
	case NO_ACT:
		break;
	case LEFT_ACT:
		move_block(g, -1, 0);
		break;
	case RIGHT_ACT:
		move_block(g, 1, 0);
		break;
	case DOWN_ACT:
		pstopped = !move_block(g, 0, 1);
		tick_reset(g);
		break;
	case DROP_ACT:
		while(move_block(g, 0, 1));
		pstopped = 1;
		tick_reset(g);
		break;
	case PAUSE_ACT:
		if(' ' == g->blk.letter) {
			if_putmsg(_("Cannot pause while a blank letter is in play"));
			break;
		}
		if_putmsg(_("The game is paused, press any key to continue"));
		pit_blank();
		if_getaction(NULL);
		game_redraw(g);
		if_putmsg("");
		/* don't set the delay, just move */
		break;
	case QUIT_ACT:
		game_end(g);
		break;
	case HELP_ACT:
		if_showhelp();
		break;
	case SSHOT_ACT:
		if_screenshot();
		break;
	case LETTER_ACT:
		if_showletters();
		break;
	case WORDL_ACT:
		if_putwords(g->scr.wrd);
		break;
	default:	/* a letter has been pressed */
		if(' ' == g->blk.letter) {
			g->blk.letter = act;
			move_block(g, 0, 0);	/* Redraw piece */
		}
	}

	return pstopped;
}

/* trys to create a falling block
 * returns: 1	created
 *	    0   could not create, no space. End of game.
 */
static int create_block(struct game *g)
{
	/* return if there is no room for block */
	if(0 != g->p.pit[PITWIDTH>>1][0])
		return 0;

	g->blk.x = PITWIDTH>>1;
	g->blk.y = 0;
	g->blk.letter = next_letter(g);

	move_block(g, 0, 0);
	if_putscore(&g->scr, g->next_char);

	return 1;
}

/* Move the block (if possible) and erase the old block
 * Returns: 1 - success
 *          0 - failed */
static int move_block(struct game *g, int rx, int ry)
{
	int x, y;

	x = g->blk.x + rx;
	y = g->blk.y + ry;
	if(x<0 || PITWIDTH<=x || y<0 || PITDEPTH<=y || g->p.pit[x][y])
		return 0;

	if_putpit(' ', NONE, g->blk.x, g->blk.y);

	g->blk.x = x;
	g->blk.y = y;
	if_putpit(g->blk.letter, NORMAL, x, y);

	if_refresh();

	return 1;
}

static void pit_blank()
{
	int x, y;

	for(y=0; y<PITDEPTH; y++)
		for(x=0; x<PITWIDTH; x++)
			if_putpit(' ', NONE, x, y);
	if_refresh();
}

static void game_redraw(const struct game *g)
{
	int x, y;

	for(y=0; y<PITDEPTH; y++)
		for(x=0; x<PITWIDTH; x++)
			if(g->p.pit[x][y])
				if_putpit(g->p.pit[x][y], NORMAL, x, y);
			else
				if_putpit(0, NONE, x, y);
	if_putpit(g->blk.letter, NORMAL, g->blk.x, g->blk.y);
	if_refresh();
}

static void pit_highlight(const struct pit *p, const struct pit *sel)
{
	int x, y;

	for(y=0; y<PITDEPTH; y++)
		for(x=0; x<PITWIDTH; x++)
			if(sel->pit[x][y])
				if_putpit(p->pit[x][y], HIGHLIGHT, x, y);
}

static void pit_clear_selected(struct pit *p, const struct pit *sel)
{
	int x, y;

	for(y=0; y<PITDEPTH; y++) {
		for(x=0; x<PITWIDTH; x++) {
			if(sel->pit[x][y]) {
				if_putpit(' ', NONE, x, y);
				p->pit[x][y] = 0;
			}
		}
	}
}

static int pit_empty(const struct pit *p)
{
	int x, y;

	for(x=0; x<PITWIDTH; x++)
		for(y=0; y<PITDEPTH; y++)
			if(p->pit[x][y])
				return 0;
	return 1;
}

static int pit_gravity(struct pit *p)
{
	int x, y;
	int count = 0;

	/* -2 since the bottom row cannot fall.. */
	for(y=PITDEPTH-2; y>=0; y--) {
		for(x=0; x<PITWIDTH; x++) {
			if(p->pit[x][y] && !p->pit[x][y+1]) {
				/* Drop piece */
				p->pit[x][y+1] = p->pit[x][y];
				p->pit[x][y] = 0;
				if_putpit(' ', NONE, x, y);
				if_putpit(p->pit[x][y+1], NORMAL, x, y+1);
				count++;
			}
		}
	}
	if_refresh();

	return count;
}

static void time_sub(struct timeval *r, struct timeval *a, struct timeval *b)
{
	r->tv_sec = a->tv_sec - b->tv_sec;
	r->tv_usec = a->tv_usec - b->tv_usec;
	if(r->tv_usec < 0) {
		r->tv_sec--;
		r->tv_usec += 1000000;
	}
}
