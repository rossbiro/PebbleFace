#ifndef REMOTESCREEN_WINDOW_H_
#define REMOTESCREEN_WINDOW_H_
#include <pebble.h>
#include "Object.h"
#include "TextLayer.h"
  
struct MyWindow {
  Window *w;
  struct objects *myTextLayers;
};

extern struct objects *myWindows;

int init_windows();
void deinit_windows();
  
#endif
