/* Copyright 1998,2001  Mark Pulford
 * This file is subject to the terms and conditions of the GNU General Public
 * License. Read the file COPYING found in this archive for details, or
 * visit http://www.gnu.org/copyleft/gpl.html
 */

/* Check all combinations found in the pit. Policy is decided by the
 * dictionary file.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexter.h"
#include "words.h"
#include "iface.h"
#include "list.h"
#include "dict.h"
#include "lang.h"

static char *reverse(char *str, int len);
static int word_score(const char *word);
static struct list *find_words(const char *str, char *used, int length);
static struct list *find_words_dual(const char *str, char *used, int length);
static struct word *make_word(const char *w);

static char *reverse(char *str, int len)
{
	int i;
	char t;

	for(i=0; i<len/2; i++) {
		t = str[i];
		str[i] = str[len-i-1];
		str[len-i-1] = t;
	}

	return str;
}

/* This only works with letters [a-z]. no capitals. */
static int word_score(const char *word)
{
	int score = 0;
	int c;

	for(c=0; c<strlen(word); c++)	/* bonus for each letter/difficulty */
		score += letter_score(word[c]);

	return score;
}

static struct list *find_words(const char *str, char *used, int length)
{
	struct list *p = NULL;
	char temp[MAXPITSTR];
	int i, j, r;	/* j represents length - 1 of the current string */

	for(i=0; i<length; i++) {
		for(j=0; j<length-i; j++) {
			temp[j] = str[i+j];
			if(0 == temp[j])
				break;	/* No more string, advance i */
			temp[j+1] = 0;
			r = dict_check(temp);
			if(1 == r) {	/* Found */
				p = list_add(p, make_word(temp));
				memset(used+i, 1, j+1);
			}
		}
	}

	return p;
}

/* Given a string (which may contain blanks in it) return all valid words 
 * and set which chars were used in forming the words, forwards and
 * backwards */
static struct list *find_words_dual(const char *str, char *used, int length)
{
	struct list *p = NULL;

	p = find_words(str, used, length);
	reverse((char *)str, length);
	reverse(used, length);
	p = list_join(p, find_words(str, used, length));
	reverse((char *)str, length);	/* Not necessary in this context */
	reverse(used, length);

	return p;
}

static struct word *make_word(const char *w)
{
	struct word *p;

	p = malloc(sizeof(*p));
	if(!p)
		abort();
	strncpy(p->str, w, sizeof(p->str));
	p->score = word_score(p->str);

	return p;
}

struct list *find_pit_words(const struct pit *p, struct pit *u)
{
	char temp[MAXPITSTR];		/* Max string + 1 */
	char used[MAXPITSTR];
	int x, y, i;
	struct list *l = NULL;

	memset(u->pit, 0, PITWIDTH*PITDEPTH);

	/* Extract and test rows */
	for(y=0; y<PITDEPTH; y++) {
		for(i=0; i<PITWIDTH; i++)
			temp[i] = p->pit[i][y];

		memset(used, 0, PITWIDTH);
		l = list_join(l, find_words_dual(temp, used, PITWIDTH));

		for(i=0; i<PITWIDTH; i++)
			u->pit[i][y] += used[i];
	}

	/* Extract and test columns */
	for(x=0; x<PITWIDTH; x++) {
		for(i=0; i<PITDEPTH; i++)
			temp[i] = p->pit[x][i];

		memset(used, 0, PITDEPTH);
		l = list_join(l, find_words_dual(temp, used, PITDEPTH));

		for(i=0; i<PITDEPTH; i++)
			u->pit[x][i] += used[i];
	}

	return l;
}
