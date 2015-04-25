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
    layer_add_child(window_get_root_layer(mw->w), text_layer_get_layer(mtl->tl));
  }
  
  text_layer_set_background_color(mtl->tl, mtl->bg);
  text_layer_set_text_color(mtl->tl, mtl->fg);
  if (mtl->text != NULL) {
    text_layer_set_text(mtl->tl, mtl->text);
  }

  text_layer_set_font(mtl->tl, mtl->font);
  text_layer_set_text_alignment(mtl->tl, mtl->alignment);
  
  layer_mark_dirty(text_layer_get_layer(mtl->tl));

}

void myTextLayerUnload(MyWindow *mw, MyTextLayer *mtl) {
  layer_remove_from_parent(text_layer_get_layer(mtl->tl));
  text_layer_destroy(mtl->tl);
  mtl->tl = NULL;
}

int createTextLayer(MyWindow *mw, DictionaryIterator *rdi) {
  Tuple *t;
  MyTextLayer *mtl = malloc(sizeof(*mtl));
  if (mtl == NULL) {
    return -ENOMEM;
  }
  
  mtl->font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  mtl->font_loaded = false;
  mtl->fg = GColorBlack;
  mtl->bg = GColorWhite;
  mtl->text = NULL;
  mtl->alignment = GTextAlignmentLeft;
  mtl->rect = GRect(0, 55, 144, 50);
  
  t = dict_find(rdi, KEY_ID);
 
  mtl->id = tuple_get_uint32(t);
  
  return allocObjects(mw->myTextLayers, mtl);
}

void myTextLayerDestructor(void *vptr) {
  MyTextLayer *mtl = (MyTextLayer *)vptr;
  
  if (mtl->tl != NULL) {
    layer_remove_from_parent(text_layer_get_layer(mtl->tl));
    text_layer_destroy(mtl->tl);
    mtl->tl = NULL;
  }
  
  if (mtl->font_loaded) {
    fonts_unload_custom_font(mtl->font);
    mtl->font_loaded = false;
  }
  
  if (mtl->text) {
    free(mtl->text);
    mtl->text = NULL;
  }
  free(mtl);
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
myTextLayerSetAttributes(MyWindow *mw, MyTextLayer *mtl, DictionaryIterator *attr) {
  bool changed = false;
  for (Tuple *t = dict_read_first(attr);
       t != NULL;
       t = dict_read_next(attr)) {
    switch(t->key) {
      case KEY_ATTRIBUTE_TEXT:
        if (t->type == TUPLE_CSTRING) {
          if (mtl->text != NULL) {
            free(mtl->text);
          }
          mtl->text = strdup(t->value->cstring);
          changed = true;
        } else if (t->type == TUPLE_BYTE_ARRAY) {
          if (mtl->text != NULL) {
            free(mtl->text);
          }
          mtl->text = malloc (t->length + 1);
          if (mtl->text == NULL) {
            return -ENOMEM;
          }
          
          memcpy (mtl->text, t->value->data, t->length);
          mtl->text[t->length] = 0;
          changed = true;
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Updated text to %s", mtl->text);
      break;
      
      case KEY_ATTRIBUTE_FONT:
        if (t->type == TUPLE_CSTRING) {
          if (mtl->font_loaded) {
            fonts_unload_custom_font(mtl->font);
            mtl->font_loaded = false;
          }
          mtl->font = fonts_get_system_font(t->value->cstring);
          changed = true;
        } else if (t->type == TUPLE_UINT) {
          if (mtl->font_loaded) {
            fonts_unload_custom_font(mtl->font);
            mtl->font_loaded = false;
          }
          mtl->font = fonts_load_custom_font(resource_get_handle(tuple_get_uint32(t)));
          if (mtl->font != NULL) {
            mtl->font_loaded = true;
          } else {
            return -ENOFONT;
          }
          changed = true;
        } else if (t->type == TUPLE_BYTE_ARRAY) {
          char font_name[t->length + 1];
          memcpy (font_name, t->value->data, t->length);
          font_name[t->length] = 0;
          if (mtl->font_loaded) {
            fonts_unload_custom_font(mtl->font);
            mtl->font_loaded = false;
          }
          mtl->font = fonts_get_system_font(font_name);
          changed = true;
        }
        break;
      
      case KEY_ATTRIBUTE_FG_COLOR:
        if (t->type == TUPLE_UINT) {
          mtl->fg = tuple_get_uint32(t);
          changed = true;
        }
      break;
      
      case KEY_ATTRIBUTE_BG_COLOR:
        if (t->type == TUPLE_UINT) {
          mtl->bg = tuple_get_uint32(t); 
          changed = true;
        }
      break;
      
      case KEY_ATTRIBUTE_ALIGNMENT:
        if (t->type == TUPLE_UINT) {
          mtl->alignment = tuple_get_uint32(t);
          changed = true;
        }
      break;
      
      case KEY_ATTRIBUTE_RECT:
        if (t->type == TUPLE_BYTE_ARRAY && t->length == 8) {
          changed = true;
          mtl->rect = GRectFromByteArray(t->value->data);
        }
      break;
      
    }    
  }
  
  if (changed) {
    myTextLayerLoad(mw, mtl);
  }
  return 0;
}

MyTextLayer *getTextLayerByHandle(MyWindow *mw, int id) {
  MyTextLayer *mtl;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "in getTextLayerByID: %p, %d", mw, id);
  if (id < 0 || mw == NULL) {
    return NULL;
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In GetTxtLayerByID, count = %d", mw->myTextLayers->count);
  if (id >= mw->myTextLayers->count) {
    return NULL;
  }
  
  mtl = (MyTextLayer *)(mw->myTextLayers->objects[id]);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "GetTextLayerByID, returning %p", mtl);
  return mtl;
}

int getTextLayerByID(MyWindow *mw, DictionaryIterator *rdi) {
  uint32_t id;
  Tuple *t = dict_find(rdi, KEY_ID);
  if (t == NULL) {
    return -ENOLAYER;
  }
  
  id = tuple_get_uint32(t);
  
  if (id == 0 || mw->myTextLayers == NULL) {
    return -ENOLAYER;
  }
  
  for (int i = 0; i < mw->myTextLayers->count; ++i) {
    MyTextLayer *mtl = (MyTextLayer *)(mw->myTextLayers->objects[i]);
    if (mtl != NULL && mtl->id == id) {
        return i;    
    }
  }
  return -ENOLAYER;
 }