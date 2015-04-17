#include <pebble.h>
#include "TextLayer.h"
  
void text_layer_load(MyWindow *mw, MyTextLayer *mtl) {
  if (mtl->tl == NULL) {
    mtl->tl = text_layer_create(mtl->rect);
    if (mtl->tl == NULL) {
      return;
    }
  }
  
  text_layer_set_background_color(mtl->tl, mtl->bg);
  text_layer_set_text_color(mtl->tl, mtl->fg);
  if (mtl->text != NULL) {
    text_layer_set_text(mtl->tl, mtl->text);
  }

  // Improve the layout to be more like a watchface
  text_layer_set_font(mtl->tl, mtl->font);
  text_layer_set_text_alignment(mtl->tl, mtl->alignment);
  
  layer_add_child(window_get_root_layer(mw->w), text_layer_get_layer(mtl->tl));

}

void text_layer_unload(MyWindow *mw, MyTextLayer *mtl) {
  text_layer_destroy(mtl->tl);
  mtl->tl = NULL;
}

int createTextLayer(MyWindow *mw) {
  MyTextLayer *mtl = malloc(sizoef(*mtl));
  if (mtl == NULL) {
    return -ENOMEM;
  }
  
  mtl->font = fonts_get_system_font(FONT_KEY_BITHAM_14_BOLD);
  mtl->fg = GColorWhite;
  mtl->bg = GColorBlack;
  mtl->text = NULL;
  mtl->alignment = GTextAlignmentLeft;
  mtl->rect = GRect(0, 55, 144, 50);
  
  return allocObjects(mw->myTextLayers, mtl);
}

void MyTextLayerDestructor(void *vptr) {
  MyTextLayer *mtl = (MyTextLayer *)vptr;
  if (mtl->text) {
    free(mtl->text);
    mtl->text = NULL;
  }
  
  if (mtl->tl != NULL) {
    text_layer_destroy(mtl->tl);
    mtl->tl = NULL;
  }
}