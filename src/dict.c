/* Copyright 1998,2001  Mark Pulford
 * This file is subject to the terms and conditions of the GNU General Public
 * License. Read the file COPYING found in this archive for details, or
 * visit http://www.gnu.org/copyleft/gpl.html
 */

/* The upper bound for letter combinations at the moment is ~7000.
 * Usually the number of checks required will be considerably less.
 *
 * A P166 can do 250000 failed lookups (the worst case) within a second on
 * a 40k dictionary. Speed shouldn't be a problem.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "dict.h"

static char **windex;	/* Contains the index to the dictionary */
static char *dict;	/* Contains the dictionary */
static int words = 0;	/* The number of the words in the dictionary */
static char dict_name[9];

static int scompare(const void *a, const void *b);
static int sorted();
static void dict_sort();

/* Dictionary entries are separated by \n. It is faster if they are sorted
 * in strcmp order. Extended characters may be ok depending on your
 * charset & curses.
 */

/* Returns 0 on error (err points to error message), 1 on success
 * If the dictionary is already sorted it will decrease the loading time.
 */
int dict_load(const char *dname, const char **err)
{
	static char errbuf[81];
	int fd;
	struct stat info;
	int c;
	char fn[256];
	int rsize;

	if(dict)
		return 1;

	snprintf(dict_name, 9, "%s", dname);

	snprintf(fn, 256, "%s/dict.%s", DICTDIR, dict_name);
	fd = open(fn, O_RDONLY);
	if(fd < 0) {
		if(err) {
			snprintf(errbuf, 81, "%s", strerror(errno));
			*err = errbuf;
		}
		return 0;
	}

	if(fstat(fd, &info) < 0) {
		close(fd);
		if(err) {
			snprintf(errbuf, 81, "%s", strerror(errno));
			*err = errbuf;
		}
		return 0;
	}
	dict = malloc(info.st_size + 1);	/* Inc null terminator */
	if(!dict) {
		if(err) {
			strcpy(errbuf, _("Out of memory"));
			*err = errbuf;
		}
		return 0;
	}

	rsize = read(fd, dict, info.st_size);
	if(rsize != info.st_size) {
		if(rsize < 0)
			snprintf(errbuf, 81, "%s", strerror(errno));
		else
			strcpy(errbuf, _("Partial read"));
		*err = errbuf;
		close(fd);
		free(dict);
		dict = NULL;
		return 0;
	}
	close(fd);

	dict[info.st_size] = 0;

	for(c=0; c<info.st_size; c++)
		if('\n' == dict[c])
			words++;

	windex = malloc(sizeof(*windex) * (words+1));	/* Inc strtok NULL */
	if(!windex) {
		free(dict);
		dict=NULL;
                if(err) {
			strcpy(errbuf, _("Out of memory"));
			*err = errbuf;
		}
		return 0;
	}

	/* Convert into word arrary */
	words = 0;
	windex[words] = strtok(dict, "\n");
	while((windex[++words] = strtok(NULL, "\n")));

	if(!sorted())
		dict_sort();

	return 1;
}

void dict_free()
{
	if(dict) {
		free(dict);
		dict = NULL;
	}
}

static int sorted()
{
	int c;

	for(c=0; c<(words-1); c++)
		if(strcmp(windex[c],windex[c+1]) >= 0)
			return 0;

	return 1;
}

/* Dictionaries must be sorted for this binary search to work
 * returns: 0 not found (or dictionary not loaded), 1 found
 */
int dict_check(const char *word)
{
	int min,max;
	int d,i;

	if(!dict)
		return 0;

	min = 0;
	max = words - 1;	/* div 2 rounds down, so words-1 is last */

	do {
		i = (min + max) / 2;
		d = strcmp(word, windex[i]);
		if(d < 0) {
			max = i-1;
		} else if(0 == d) {
			return 1;
		} else {
			min = i+1;
		}
	} while(min <= max);

	return 0;
}

/* Dump the dictionary in the "correct" format */
int dict_dump()
{
	int i;

	if(!dict)
		return 0;

	for(i=0; i<words; i++)
		printf("%s\n", windex[i]);

	return 1;
}

/* Fills freq[256] with frequency info from the loaded dictionary
 * Returns: 0	dictionary not loaded
 * 	    1	success */
int dict_get_freq(int *freq)
{
	int w, i;

	if(!dict)
		return 0;

	memset(freq, 0, 256*sizeof(*freq));

	for(w=0; w<words; w++)
		for(i=0; windex[w][i]; i++)
			freq[windex[w][i]&255]++;

	return 1;
}

static int scompare(const void *a, const void *b)
{
	const char **s1, **s2;

	s1 = (const char **) a;
	s2 = (const char **) b;
	return strcmp(*s1, *s2);
}

static void dict_sort()
{
	qsort(windex, words, sizeof(*windex), scompare);
}

char *dict_get()
{
	return dict_name;
}
