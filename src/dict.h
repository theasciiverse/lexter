#ifndef DICT_H
#define DICT_H

int dict_load(const char *dname, const char **err);
void dict_free();
int dict_check(const char *word);
int dict_dump();
char *dict_get();
int dict_get_freq(int *freq);

#endif
