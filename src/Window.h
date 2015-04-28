#ifndef REMOTESCREEN_WINDOW_H_
#define REMOTESCREEN_WINDOW_H_
#include <pebble.h>
#include "Object.h"
#include "TextLayer.h"
#include "Protocol.h"
#include "Util.h"
  
typedef struct MyWindow {
  Window *w;
  uint32_t id;
  struct objects *myTextLayers;
  uint32_t button_config[NUM_BUTTONS];
  AppTimer *appTimer;
} MyWindow;

// delay at least 10 ms between updates no matter what resolution is displayed.
#define MIN_REFRESH_DELAY 10 
  
extern struct objects *myWindows;

void deinit_windows();
int allocWindow();
MyWindow *getWindowByHandle(int handle);
void windowRescheduleTimer(MyWindow *mw, uint32_t update_freq);

// The Window Functions called remotely.
int pushWindow(MyWindow *, DictionaryIterator *);
int requestClicks(MyWindow *, DictionaryIterator *);
int getWindowByID(DictionaryIterator *);
int resetWindows(DictionaryIterator *);
int clearWindow(MyWindow *mw, DictionaryIterator *);
  
#endif
