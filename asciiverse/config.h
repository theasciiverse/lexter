/* config.h for the Asciiverse build (asciiverse/build.sh), in place of
 * the one the 2001 configure script writes: every target here is a
 * current Unix with the same answers. No NLS: Lexter has no translations
 * (po/ holds only the template). */
#ifndef CONFIG_H
#define CONFIG_H

#include <sys/stat.h>

#define PACKAGE		"lexter"
#define VERSION		"1.0.3"

/* Game files; DICTDIR and SCOREFILE can be set by the build */
#ifndef DICTDIR
#define DICTDIR		"/usr/share/lexter"
#endif
#ifndef SCOREFILE
#define SCOREFILE	"/var/games/lexter.scores"
#endif
#define LOCALEDIR	"/usr/share/locale"
#define SCOREPERM	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)

#define HAVE_CURSES_H	1
#define STDC_HEADERS	1
#define TIME_WITH_SYS_TIME 1
#define HAVE_GETCWD	1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_SELECT	1
#define HAVE_STRDUP	1
#define HAVE_STRERROR	1
#define HAVE_FCNTL_H	1
#define HAVE_LIMITS_H	1
#define HAVE_LOCALE_H	1
#define HAVE_STRING_H	1
#define HAVE_SYS_FILE_H	1
#define HAVE_SYS_TIME_H	1
#define HAVE_UNISTD_H	1

#define _(a)	(a)

#endif
