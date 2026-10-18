/* Copyright 1998,2001  Mark Pulford
 * This file is subject to the terms and conditions of the GNU General Public
 * License. Read the file COPYING found in this archive for details, or
 * visit http://www.gnu.org/copyleft/gpl.html
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "scores.h"
#include "iface.h"
#include "dict.h"
#include "list.h"
#include "paths.h"

struct assoc {
	char *str;
	int num;
};

static int score_open();
static int get_username(char *name, int size);
static int fill_entry(struct hiscore *hs, const struct score *scr);
static int scr_compare(const void *a, const void *b);
static int lockfd(int fd, int type);
static int worst_score(struct hiscore *hs, int entries);
static struct assoc *assoc_new(const char *str);
static struct assoc *assoc_find(struct list **l, const char *str);
static struct assoc *assoc_get(struct list **l, const char *str);	
static void assoc_free(struct list **l);

/* return is same as open(2)
 * gives file descriptor to score file open for RW */
static int score_open()
{
	int fd, u;
	const char *file = paths_userfile("scores");

	if(!file) {
		errno = ENOENT;
		return -1;
	}
	u = umask(0);
	fd = open(file, O_CREAT|O_EXCL|O_RDWR, SCOREPERM);
	umask(u);
	if(fd<0 && EEXIST==errno)	/* File already exists */
		fd = open(file, O_RDWR);

	return fd;
}

/* Fill buffer name (size bytes long) with user name
 * Ensures name is NULL terminated */
static int get_username(char *name, int size)
{
	struct passwd *p;

	p = getpwuid(getuid());
	if(!p)
		return 0;

	strncpy(name, p->pw_name, size);
	name[size-1] = 0;

	return 1;
}

/* -1 = a is higher in the list than b
 *  0 = a & b are equal
 *  1 = a is lower in the list than b */
static int scr_compare(const void *a, const void *b)
{
	const struct hiscore *s1 = (const struct hiscore*) a;
	const struct hiscore *s2 = (const struct hiscore*) b;

	if(s1->total < s2->total)
		return 1;
	if(s1->total > s2->total)
		return -1;
	if(s1->words < s2->words)	/* equal total. low words better */
		return -1;
	if(s1->words > s2->words)
		return 1;
	if(s1->blocks < s2->blocks)	/* equal words?. low blocks better */
		return -1;
	if(s1->blocks > s2->blocks)
		return 1;
	if(s1->date < s2->date)		/* equal blocks??. older games better */
		return -1;
	if(s1->date > s2->date)
		return 1;

	return 0;
}

/* Returns:	1 success
 * 		0 unable to find username */
static int fill_entry(struct hiscore *hs, const struct score *scr)
{
	if(!get_username(hs->name, HI_NAME_LEN))
		return 0;
	hs->total = scr->total;
	hs->words = scr->words;
	hs->blocks = scr->blocks;
	hs->date = time(NULL);
	strncpy(hs->dict, dict_get(), HI_DICT_LEN-1);
	hs->dict[HI_DICT_LEN-1] = 0;

	return 1;
}

/* Load (Merge/Save if scr!=NULL) & return high scores */
/* Returns	NULL	error updating hi score table
 * 		otherwise pointer to struct hiscore array.
 * 		(last entry has null length name */
/* scr may be NULL */
struct hiscore *score_update(const struct score *scr, time_t *scr_time, const char **err)
{
	static char errbuf[81];
	struct hiscore *hs;
	int entries;
	int size;
	int fd;

	*err = NULL;

	fd = score_open();
	if(fd < 0) {
		snprintf(errbuf, 81, _("High score open: %s"), strerror(errno));
		*err = errbuf;
		return NULL;
	}

	/* add 1 entry for terminator */
	hs = malloc(sizeof(*hs)*(SCORE_ENTRIES+1));
	if(!hs)
		abort();

	memset(hs, 0, sizeof(*hs)*(SCORE_ENTRIES+1));

	if(!lockfd(fd, F_WRLCK)) {
		close(fd);
		snprintf(errbuf, 81, _("High score locking: %s"), strerror(errno));
		*err = errbuf;
		free(hs);
		return NULL;
	}

	size = read(fd, hs, sizeof(*hs)*SCORE_ENTRIES);
	if(size < 0) {
		snprintf(errbuf, 81, _("High score read: %s"), strerror(errno));
		*err = errbuf;
		free(hs);
		close(fd);
		return NULL;
	}
	entries = size / sizeof(*hs);
	/* entries <= SCORE_ENTRIES since read will never return more bytes */

	/* Put latest score at the end of the array */
	if(scr) {
		if(!fill_entry(&hs[entries], scr)) {
			snprintf(errbuf, 81, _("Unable to get username"));
			*err = errbuf;
			close(fd);
			free(hs);
			return NULL;
		}
		if(scr_time)
			*scr_time = hs[entries].date; 
		entries++;	/* Included latest score */
	}

	/* Sort the high scores */
	qsort(hs, entries, sizeof(*hs), scr_compare);

	/* Note: If SCORE_ENTRIES-entries > 1 then chances are the
	 * most worthy score for eviction would have been chopped off
	 * anyway, along with a few scores which don't deserve to go.
	 * This should never happen unless lexter is recompiled
	 * with a smaller SCORE_ENTRIES. */
	if(entries > SCORE_ENTRIES) {
		int e;

		e = worst_score(hs, entries);
		memcpy(&hs[e], &hs[entries-1], sizeof(*hs));
		entries = SCORE_ENTRIES;
		/* Re sort after eviction shuffle */
		qsort(hs, entries, sizeof(*hs), scr_compare);
	}

	/* Terminate array - removes lowest score if necessary */
	hs[entries].name[0] = 0;

	/* Seek to start of file and write. Errors aren't fatal
	 * return the high score table either way */
	if(scr) {
		if(0 == lseek(fd, 0, SEEK_SET)) {
			if(write(fd, hs, entries*sizeof(*hs)) <0) {
				snprintf(errbuf, 81, _("High score write: %s"), strerror(errno));
				*err = errbuf;
			}
		} else {
			snprintf(errbuf, 81, _("High score seek: %s"), strerror(errno));
			*err = errbuf;
		}
	}
	close(fd);	/* close file & remove lock */

	return hs;
}

static int lockfd(int fd, int type)
{
	struct flock l;

	l.l_type = type;
	l.l_whence = SEEK_SET;
	l.l_start = 0;
	l.l_len = 0;
	if(-1 == fcntl(fd, F_SETLKW, &l))
		return 0;

	return 1;
}

/* Quick n dirty assoc funcs - only use for small n */
static struct assoc *assoc_new(const char *str)
{
	struct assoc *p;

	p = malloc(sizeof(*p));
	if(!p)
		abort();
	p->str = strdup(str);
	p->num = 0;

	return p;	
}

static struct assoc *assoc_find(struct list **l, const char *str)
{
	struct list *p = *l;
	struct assoc *a;

	for(; p; p = p->next) {
		a = (struct assoc *)p->data;
		if(!strcmp(a->str, str))
			return a;
	}

	return NULL;
}

/* Get the assoc entry for str, if it doesn't exist create it */
static struct assoc *assoc_get(struct list **l, const char *str)
{
	struct assoc *a;

	a = assoc_find(l, str);
	if(!a) {
		a = assoc_new(str);
		*l = list_join(*l, list_element(a));
	}

	return a;
}

/* list_free helper function */
static void freeassoc(void *p)
{
	struct assoc *a = (struct assoc *)p;
	free(a->str);
	free(a);
}

static void assoc_free(struct list **l)
{
	list_free(*l, freeassoc);
	*l = NULL;
}

/* Return the index of the score most worthy of eviction.
 * Evict the lowest score among the set of dictionaries with the most
 * scores in the table. */
static int worst_score(struct hiscore *hs, int entries)
{
	struct list *dcount = NULL;
	struct list *dmin = NULL;
	struct list *p;
	struct assoc *a;
	int worst=0, max = 0;
	int i;

	for(i=0; i<entries; i++) {
		a = assoc_get(&dcount, hs[i].dict);
		a->num++;
		a = assoc_get(&dmin, hs[i].dict);
		a->num = i;
	}

	/* Find the maximum number of scores taken by a dictionary */
	for(p=dcount; p; p=p->next) {
		a = (struct assoc *)p->data;
		if(a->num > max)
			max = a->num;
	}

	/* Scan the set of greediest dictionaries for the lowest score.
	 * Note that the score with the highest index has the lowest score */
	for(p=dcount; p; p=p->next) {
		a = (struct assoc *)p->data;
		if(a->num == max) {
			a = assoc_find(&dmin, a->str);
			if(a->num > worst)
				worst = a->num;
		}
	}

	assoc_free(&dcount);
	assoc_free(&dmin);

	return worst;
}
