#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

word_t expr(char *e, bool *success);

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char exp[32];
  word_t value;
  /* TODO: Add more members if necessary */

} WP;

WP* wp_new(char* exp);
void wp_free(WP* wp);
void wp_delete(int num);
void wp_display();
bool wp_check();

#endif
