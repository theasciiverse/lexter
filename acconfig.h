#ifndef CONFIG_H
#define CONFIG_H

#include <sys/stat.h>

/* Game files */
#define DICTDIR		""
#define SCOREFILE	""
#define LOCALEDIR	""

#undef SC_WORLDWRITE
#ifdef SC_WORLDWRITE
#define SCOREPERM       (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#else
#define SCOREPERM       (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)
#endif

@TOP@
/* Define curses header availability */
#undef HAVE_CURSES_H
#undef HAVE_NCURSES_H

/* Define if your locale.h file contains LC_MESSAGES.  */
#undef HAVE_LC_MESSAGES

/* Define to 1 if NLS is requested.  */
#undef ENABLE_NLS

/* Define as 1 if you have catgets and don't want to use GNU gettext.  */
#undef HAVE_CATGETS

/* Define as 1 if you have gettext and don't want to use GNU gettext.  */
#undef HAVE_GETTEXT

/* Define as 1 if you have the stpcpy function.  */
/* #undef HAVE_STPCPY */

@BOTTOM@

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(a)	gettext(a)
#else
#define _(a)	(a)
#endif

#endif
