#ifndef REMOTESCREEN_TEXTLAYER_H_
#define REMOTESCREEN_TEXTLAYER_H_
  
#include <pebble.h>

struct MyWindow;

typedef struct MyWindow MyWindow;
  
typedef struct MyTextLayer {
  uint32_t id;
  TextLayer *tl;
  GRect rect;
  GColor fg;
  GColor bg;
  GFont font;
  char *text;
  char *buff;
  int text_length;
  bool font_loaded;
  GTextAlignment alignment;
  struct MyWindow *parent;
  uint32_t time; // always in ms.
} MyTextLayer;

void text_layer_load(struct MyWindow *, MyTextLayer *);
void text_layer_unload(struct MyWindow *, MyTextLayer *);

void myTextLayerDestructor(void *);
int myTextLayerLoad(struct MyWindow *mw, MyTextLayer *mtl);
void myTextLayerUnload(struct MyWindow *mw, MyTextLayer *mtl);
MyTextLayer *getTextLayerByHandle(struct MyWindow *mw, int id);

// Remote Calls
int getTextLayerByID(struct MyWindow *, DictionaryIterator *);
int createTextLayer(struct MyWindow *mw, DictionaryIterator *rdi);
int myTextLayerSetAttributes(struct MyWindow *mw, MyTextLayer *mtl, DictionaryIterator *attr);

#endif //REMOTESCREEN_TEXTLAYER_H_