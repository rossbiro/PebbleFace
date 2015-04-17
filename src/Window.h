#ifndef REMOTESCREEN_WINDOW_H_
#define REMOTESCREEN_WINDOW_H_
#include <pebble.h>
#include "Object.h"
#include "TextLayer.h"
#include "Protocol.h"
  
typedef struct MyWindow {
  Window *w;
  struct objects *myTextLayers;
} MyWindow;

extern struct objects *myWindows;

int init_windows();
void deinit_windows();
int allocWindow();
MyWindow *getWindowByID(int id);
void pushWindow(MyWindow *);
  
#endif
