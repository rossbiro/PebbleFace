#ifndef REMOTESCREEN_TEXTLAYER_H_
#define REMOTESCREEN_TEXTLAYER_H_
  
#include <pebble.h>

struct MyTextLayer {
  TextLayer *tl;
  GRect rect;
  GColor fg;
  GColor bg;
  GFont font;
  char *text;
  GTextAlignment alignment;
};

void text_layer_load(MyWindow *, MyTextLayer *);
void text_layer_unload(MyWindow *, MyTextLayer *);

int createTextLayer(MyWindow *mw);
int myTextLayerSetAttributes(MyTextLayer *mtl, DictionaryIterator *attr);
void MyTextLayerDestructor(void *);

#endif //REMOTESCREEN_TEXTLAYER_H_