#include <pebble.h>
#include "Standard.h" 

char *strdup(const char *old) {
  int len = strlen(old);
  char *n = malloc(len + 1);
  if (n != NULL) {
    strncpy(n, old, len + 1);
  }
  return n;
}