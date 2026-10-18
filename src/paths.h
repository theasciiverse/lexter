/* Where Lexter's files live; added for the Asciiverse packaging.
 * Same terms as the rest of Lexter (GNU GPL, see COPYING). */
#ifndef PATHS_H
#define PATHS_H

/* The folder holding dict.<name>: $LEXTER_DICTDIR, else the folder the
 * program is in when the dictionaries are there (an unpacked archive),
 * else DICTDIR (an installed package). */
const char *paths_dictdir(void);

/* name inside the player's own folder, $XDG_DATA_HOME/lexter (else
 * ~/.local/share/lexter), which is made when missing. NULL when there is
 * no home to put it in. */
const char *paths_userfile(const char *name);

#endif
