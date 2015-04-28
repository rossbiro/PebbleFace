#ifndef REMOTE_SCREEN_STANDARD_H_
#define REMOTE_SCREEN_STANDARD_H_

#include "Util.h"
char *strdup(const char *);

static inline int min(int x, int y) { return x < y ? x : y;}

#ifndef RCC
#define RCC(x) do { ret = x; if (ret < 0 ) goto error_out; } while (0)
#endif
  
#endif
