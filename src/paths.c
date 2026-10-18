/* Where Lexter's files live; added for the Asciiverse packaging.
 * Same terms as the rest of Lexter (GNU GPL, see COPYING). */

#include <config.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "paths.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* The folder the running program is in, or NULL */
static const char *exe_dir(void)
{
	static char dir[PATH_MAX];
	char *slash;
#ifdef __APPLE__
	char buf[PATH_MAX];
	uint32_t size = sizeof(buf);

	if(_NSGetExecutablePath(buf, &size) != 0 || !realpath(buf, dir))
		return NULL;
#else
	ssize_t len = readlink("/proc/self/exe", dir, sizeof(dir) - 1);

	if(len <= 0)
		return NULL;
	dir[len] = 0;
#endif
	slash = strrchr(dir, '/');
	if(!slash)
		return NULL;
	*slash = 0;
	return dir;
}

const char *paths_dictdir(void)
{
	static char found[PATH_MAX + 16];
	const char *env = getenv("LEXTER_DICTDIR");
	const char *dir;
	struct stat st;

	if(env && *env)
		return env;
	dir = exe_dir();
	if(dir) {
		snprintf(found, sizeof(found), "%s/dict.en", dir);
		if(stat(found, &st) == 0) {
			snprintf(found, sizeof(found), "%s", dir);
			return found;
		}
	}
	return DICTDIR;
}

/* mkdir -p for the folders above and including path */
static int make_dirs(char *path)
{
	char *p;

	for(p = path + 1; *p; p++) {
		if(*p != '/')
			continue;
		*p = 0;
		if(mkdir(path, 0755) != 0 && errno != EEXIST) {
			*p = '/';
			return 0;
		}
		*p = '/';
	}
	return mkdir(path, 0755) == 0 || errno == EEXIST;
}

const char *paths_userfile(const char *name)
{
	static char file[PATH_MAX + 64];
	char dir[PATH_MAX];
	const char *xdg = getenv("XDG_DATA_HOME");
	const char *home = getenv("HOME");

	if(xdg && *xdg == '/')
		snprintf(dir, sizeof(dir), "%s/lexter", xdg);
	else if(home && *home)
		snprintf(dir, sizeof(dir), "%s/.local/share/lexter", home);
	else
		return NULL;
	if(!make_dirs(dir))
		return NULL;
	snprintf(file, sizeof(file), "%s/%s", dir, name);
	return file;
}
