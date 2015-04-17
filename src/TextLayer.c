#include <pebble.h>
#include "TextLayer.h"
#include "Window.h"
#include "Standard.h"
  
void myTextLayerLoad(MyWindow *mw, MyTextLayer *mtl) {
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

void myTextLayerUnload(MyWindow *mw, MyTextLayer *mtl) {
  text_layer_destroy(mtl->tl);
  mtl->tl = NULL;
}

int createTextLayer(MyWindow *mw) {
  MyTextLayer *mtl = malloc(sizeof(*mtl));
  if (mtl == NULL) {
    return -ENOMEM;
  }
  
  mtl->font = fonts_get_system_font("FONT_KEY_BITHAM_14_BOLD");
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

// Call must make sure the array is 8 bytes.
GRect GRectFromByteArray(uint8_t *ba) {
  // Big Endian.
  int16_t p0 = (int16_t)((ba[0] << 8) + ba[1]);
  int16_t p1 = (int16_t)((ba[2] << 8) + ba[3]);
  int16_t p2 = (int16_t)((ba[4] << 8) + ba[5]);
  int16_t p3 = (int16_t)((ba[6] << 8) + ba[7]);
  
  return GRect(p0, p1, p2, p3);
}

int 
myTextLayerSetAttributes(MyTextLayer *mtl, DictionaryIterator *attr) {
  for (Tuple *t = dict_read_first(attr);
       t != NULL;
       t = dict_read_next(attr)) {
    switch(t->key) {
      case KEY_ATTRIBUTE_TEXT:
        if (t->type == TUPLE_CSTRING) {
          if (mtl->text != NULL) {
            free(mtl->text);
          }
          mtl->text = strdup(mtl->value->cstring);
        }
      break;
      
      case KEY_ATTRIBUTE_FONT:
        if (t->type == TUPLE_CSTRING) {
          mtl->font = fonts_get_system_font(tuple->value.cstring);
        } // Int could be a resource.  But then we need to unload.
        break;
      
      case KEY_ATTRIBUTE_FG_COLOR:
        if (t->type == TUPLE_UINT) {
          mtl->fg = t->value.uint32; 
        }
      break;
      
      case KEY_ATTRIBUTE_BG_COLOR:
        if (t->type == TUPLE_UINT) {
          mtl->bg = t->value.uint32; 
        }
      break;
      
      case KEY_ATTRIBUTE_ALIGNMENT:
        if (t->type == TUPLE_UINT) {
          mtl->alignment = t->value.uint32;
        }
      break;
      
      case KEY_ATTRIBUTE_RECT:
        if (t->type == TUPLE_BYTE_ARRAY && t->length == 8) {
          mtl->rect = GRectFromByteArray(t->value.data)
        }
      break;
      
    }    
  }
}
