/* Copyright 1998,2001  Mark Pulford
 * This file is subject to the terms and conditions of the GNU General Public
 * License. Read the file COPYING found in this archive for details, or
 * visit http://www.gnu.org/copyleft/gpl.html
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#include "lang.h"
#include "dict.h"
#include "iface.h"
#include "lexter.h"

static int lfreq[256];
static int freq_total;
static int lscore[256];

/* Cannot trust RAND_MAX with random(). Some systems define it for a
 * 15bit rand() function */
#define RMAX 2147483647

/* Return a random number in [0,n-1] */
int rnd(int n)
{
	static int noseed = 1;
	double i = n;

	if(noseed) {
	        srandom(getpid()+time(NULL));
		noseed = 0; 
	}

	return (int) (i * random() / (RMAX+1.0));
}

/* update frequency/letter score tables */
void letter_update_freq()
{
	int i;
	int max;

	if(!dict_get_freq(lfreq))
		abort();

	freq_total = 0;
	max = 0;
	/* Calculate sum for proportional random letter */
	for(i=0; i<256; i++) {
		freq_total += lfreq[i];
		if(lfreq[i]>max)
			max = lfreq[i];
	}

	/* Calculate score */
	for(i=0; i<256; i++)
		lscore[i] = ((max - lfreq[i])*100)/max;
}

float letter_prob(char c)
{
	return (float)lfreq[c&255]/freq_total;
}

int letter_score(char c)
{
	assert(letter_valid(c));
	return lscore[c&255] + LETTER_BONUS;
}

/* bchance is % chance of a blank character */
char letter_generate(int bchance)
{
	int lsum=0, i, r;

	if(rnd(100)<bchance)
		return ' ';

	r = rnd(freq_total);

	lsum = 0;
	for(i=0; i<256; i++) {
		lsum += lfreq[i];
		if(r<lsum)
			break;
	}

	/* Assuming i<256. This must be true since
	 * - at the end of the loop lsum == freq_total
	 * - r < freq_total 
	 * - therefore it will always "break" out of the loop */
	/* If this fails then random() probably returned higher than RMAX */
	assert(i<256);

	return i;
}

/* Returns: 1	valid letter found in dictionary
 *          0	letter not found in dictionary */
int letter_valid(char c)
{
	return lfreq[c&255]!=0;
}
