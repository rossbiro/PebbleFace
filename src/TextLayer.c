#include <pebble.h>
#include "TextLayer.h"
#include "Window.h"
#include "Standard.h"
  
static GFont default_system_font = NULL;

static int format(char *out, int out_len, const char *in, time_t seconds, int ms) {
  char format[4];
  int out_ptr = 0;
  int format_ptr = 0;
  struct tm *t = localtime(&seconds);
  bool saw_percent = false;
  unsigned update_time = (unsigned)-1;
  
  for (int i = 0; in[i] != 0; ++i) {
    if (out_ptr >= out_len) {
      return update_time;
    }
    
    if (saw_percent) {
      switch (in[i]) {
        case 'k':
          update_time = 1;
          out_ptr += snprintf(out + out_ptr, out_len - out_ptr, "%03d", ms);
          saw_percent = false;
          update_time = 1;
          continue;
        
          case 'K':
            // We want minutes:seconds or hours:minutes depending on how much time has elapsed.
            if (seconds >= 3600) {
              if (update_time >= 60000) {
                update_time = 60000;
              }
              out_ptr += snprintf(out + out_ptr, out_len - out_ptr, "%02d:%02d", (int)seconds/3600, (int)(seconds/60) % 60);
            } else {
              if (update_time > 1000) {
                update_time = 1000;
              }
              out_ptr += snprintf(out + out_ptr, out_len - out_ptr, "%02d:%02d", (int)seconds/60, (int)seconds % 60);
            }
          saw_percent = false;
          continue;
        
        default:
          break;
      }
      
      format[format_ptr++] = in[i];
      if (format_ptr < 3 && (in[i] == 'E' || in[i] == 'O')) {
        continue;
      }
      saw_percent = false;
      format[format_ptr] = 0;
      out_ptr += strftime(out + out_ptr, out_len - out_ptr, format, t);
      continue;      
    }
    
    switch (in[i]) {
      case '%':
        saw_percent = true;
        format[0] = '%';
        format_ptr = 1;
        continue;
      
      case 'H': //hour
      case 'I':
      case 'k':
      case 'l':
        if (update_time > 3600 * 1000) {
          update_time = 3600 * 1000;
        }
        break;
      
      case 'M': //minute
      case 'R':
        if (update_time > 60 * 1000) {
          update_time = 60 * 1000;
        }
        break;
      
      case 's': // seconds
      case 'S':
      case 'T':
      case 'X':
      case '+':
      case 'c':
        if (update_time > 1000) {
          update_time = 1000;
        }
        break;
      
      default:
        break;
    }    
    out[out_ptr++] = in[i];   
  }
  
  return update_time;
}
  
int myTextLayerLoad(MyWindow *mw, MyTextLayer *mtl) {
  int next_ms = -1;
  if (default_system_font == NULL) {
    default_system_font = fonts_get_system_font(FONT_KEY_FONT_FALLBACK);
  }
  
  if (mtl->tl == NULL) {
    mtl->tl = text_layer_create(mtl->rect);
    if (mtl->tl == NULL) {
      return next_ms;
    }
    layer_add_child(window_get_root_layer(mw->w), text_layer_get_layer(mtl->tl));
  }
  
  text_layer_set_background_color(mtl->tl, mtl->bg);
  text_layer_set_text_color(mtl->tl, mtl->fg);
  if (mtl->text != NULL) {
    if (mtl->time != (uint32_t)(-1)) {
       if (mtl->buff == NULL) {
         mtl->buff = malloc (mtl->text_length + 1);
         if (mtl->buff == NULL) {
           return next_ms;
         }
         memset(mtl->buff, 0, mtl->text_length + 1 );
         //time is considered 0 time. Useful for stopwatches.
         // set time to 1 to get formating and updating, but only
         // be off 1ms from real time.
         time_t sec;
         uint16_t ms;
         time_ms(&sec, &ms);
         sec -= (mtl->time / 1000);
         ms -= (mtl->time % 1000);
         while (ms > 32768) {
           ms += 1000;
           sec -= 1;
         }
         
         // now we have seconds since the epoch and ms.
         next_ms = format (mtl->buff, mtl->text_length, mtl->text, sec, ms);
         text_layer_set_text(mtl->tl, mtl->buff);
         
       }
    } else {
      text_layer_set_text(mtl->tl, mtl->text);
    }
  }

  text_layer_set_font(mtl->tl, mtl->font);
  text_layer_set_text_alignment(mtl->tl, mtl->alignment);
  
  layer_mark_dirty(text_layer_get_layer(mtl->tl));
  return next_ms;
}

void myTextLayerUnload(MyWindow *mw, MyTextLayer *mtl) {
  layer_remove_from_parent(text_layer_get_layer(mtl->tl));
  text_layer_destroy(mtl->tl);
  mtl->tl = NULL;
}

int createTextLayer(MyWindow *mw, DictionaryIterator *rdi) {
  Tuple *t;
  int ret;
  MyTextLayer *mtl = malloc(sizeof(*mtl));
  if (mtl == NULL) {
    ret = -ENOMEM;
    goto error_out;
  }
  
  mtl->font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  mtl->font_loaded = false;
  mtl->fg = GColorBlack;
  mtl->bg = GColorWhite;
  mtl->text = NULL;
  mtl->text_length = 0;
  mtl->alignment = GTextAlignmentLeft;
  mtl->rect = GRect(0, 55, 144, 50);
  mtl->time = -1;
  
  t = dict_find(rdi, KEY_ID);
  if (t != NULL && t->type == TUPLE_UINT) {
    mtl->id = tuple_get_uint32(t);
  }
  
  t = dict_find(rdi, KEY_LENGTH);
  mtl->text_length = tuple_get_uint32(t);
  if (mtl->text_length > 0) {
    mtl->text = malloc(mtl->text_length + 1);
    memset(mtl->text, 0, mtl->text_length + 1);
  }
  
  RCC(myTextLayerSetAttributes(mw, mtl, rdi));
  
  return allocObjects(mw->myTextLayers, mtl);
  
error_out:
  if (mtl != NULL) {
    myTextLayerDestructor(mtl);
  }
  
  return ret;
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
      
      case KEY_ATTRIBUTE_TIME_DELTA:
        mtl->time = tuple_get_uint32(t);
        break;
      
      case KEY_ATTRIBUTE_TEXT:
        if (t->type == TUPLE_CSTRING) {
          if (mtl->text == NULL) {
            mtl->text = malloc (t->length + 1);
            if (mtl->text == NULL) {
              return -ENOMEM;
            }
            mtl->text[t->length] = 0;
            mtl->text_length = t->length;
          }
          strncpy(mtl->text, t->value->cstring, mtl->text_length);
          changed = true;
        } else if (t->type == TUPLE_BYTE_ARRAY) {
          if (mtl->text == NULL) {
            mtl->text = malloc (t->length + 1);
            if (mtl->text == NULL) {
              return -ENOMEM;
            }
            mtl->text[t->length] = 0;
            mtl->text_length = t->length;
          }
          memcpy (mtl->text, t->value->data, min(mtl->text_length, t->length));
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
          if (mtl->font == default_system_font) {
            return -ENOFONT;
          }
          mtl->font_loaded = true;
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
    uint32_t next = myTextLayerLoad(mw, mtl);
    if (next != (unsigned)-1) {
      windowRescheduleTimer(mw, next);
    }
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