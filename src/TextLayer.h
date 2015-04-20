#ifndef REMOTESCREEN_TEXTLAYER_H_
#define REMOTESCREEN_TEXTLAYER_H_
  
#include <pebble.h>

struct MyWindow;
  
typedef struct MyTextLayer {
  TextLayer *tl;
  GRect rect;
  GColor fg;
  GColor bg;
  GFont font;
  char *text;
  GTextAlignment alignment;
  struct MyWindow *parent;
} MyTextLayer;

void text_layer_load(struct MyWindow *, MyTextLayer *);
void text_layer_unload(struct MyWindow *, MyTextLayer *);

int createTextLayer(struct MyWindow *mw);
int myTextLayerSetAttributes(struct MyWindow *mw, MyTextLayer *mtl, DictionaryIterator *attr);
void myTextLayerDestructor(void *);
void myTextLayerLoad(struct MyWindow *mw, MyTextLayer *mtl);
void myTextLayerUnload(struct MyWindow *mw, MyTextLayer *mtl);
MyTextLayer *getTextLayerByID(struct MyWindow *mw, int id);

#endif //REMOTESCREEN_TEXTLAYER_H_