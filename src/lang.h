#ifndef LANG_H
#define LANG_H

int rnd(int n);
void letter_update_freq();
char letter_generate(int bchance);
int letter_valid(char c);
float letter_prob(char c);
int letter_score(char c);

#endif
