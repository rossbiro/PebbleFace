#ifndef REMOTESCREEN_UTIL_H_
#define REMOTESCREEN_UTIL_H_

#include <pebble.h>

static inline uint32_t tuple_get_uint32(Tuple *t) {
  if (t == NULL || t->type != TUPLE_UINT) {
    return 0;
  }
  
  if (t->length == 1) {
    return t->value->uint8;
  } else if (t->length == 2) {
    return t->value->uint16;
  } else if (t->length == 4) {
    return t->value->uint32;
  }
  
  return 0;
  
}

#endif