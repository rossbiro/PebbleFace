#ifndef REMOTE_SCREEN_H_
#define REMOTE_SCREEN_H_

#include "Util.h"
char *strdup(const char *);

#ifndef RCC
#define RCC(x) do { ret = x; if (ret < 0 ) goto error_out; } while (0)
#endif
  
#endif
