#include <pebble.h>
#include "Window.h"

struct objects *myWindows;

static void MyWindowDestructor(void *vptr) {
  MyWindow *mw = (MyWindow *)vptr;
  if (mw->myTextLayers) {
    freeObjects(mw->myTextLayers);
    mw->myTextLayers = NULL;
  }
  
  if (mw->w) {
    window_destroy(mw->w);
    mw->w = NULL;
  }
}

static MyWindow *findMyWindow(Window *w) {
  MyWindow *mw = (MyWindow *)window_get_user_data(w);
  return mw;
}

static void window_load(Window *w) {
  MyWindow *mw = findMyWindow(w);
  for (int i = 0; i < mw->myTextLayers->count; ++i) {
    MyTextLayer *mtl = (MyTextLayer *)mw->myTextLayers->objects[i];
    if (mtl != NULL) {
      myTextLayerLoad(mw, mtl);
    }
  }
}

static void window_unload(Window *w) {
  MyWindow *mw = findMyWindow(w);
  for (int i = 0; i < mw->myTextLayers->count; ++i) {
    MyTextLayer *mtl = (MyTextLayer *)mw->myTextLayers->objects[i];
    if (mtl != NULL) {
      myTextLayerLoad(mw, mtl);
    }
  }
}

int init_windows() {
  myWindows = createObjects(MyWindowDestructor);
  return 0;
}

void deinit_windows() {
  if (myWindows) {
    freeObjects(myWindows);
    myWindows = NULL;
  }
}

int allocWindow() {
    MyWindow *mw = malloc(sizeof(MyWindow));
    if (mw == NULL) {
      return -ENOMEM;
    }
    mw->myTextLayers = createObjects(myTextLayerDestructor);
    if (mw == NULL) {
      free(mw);
      return -ENOMEM;
    }
  
    for (int i = 0;i < NUM_BUTTONS; ++i) {
      mw->button_config[i] = 0;
    }
  
    mw->w = window_create();
    if (mw->w == NULL) {
      freeObjects(mw->myTextLayers);
      free(mw);
      return -ENOMEM;
    }  

    window_set_user_data(mw->w, mw);
  
    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(mw->w, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload
    });
  
    return allocObjects(myWindows, mw);
}

uint32_t timestamp() {
  time_t t;
  uint16_t ms;
  time_ms(&t, &ms);
  return (t << 16) | ms;
}

void onClick(ClickRecognizerRef recognizer, void *context) {
  uint8_t count =  click_number_of_clicks_counted(recognizer);
  uint8_t button = click_recognizer_get_button_id(recognizer);
  bool repeating = click_recognizer_is_repeating(recognizer);
  DictionaryIterator *di = begin_message(STATUS_OK, timestamp());
  dict_write_uint32(di, KEY_CLICK, CLICK_DATA(repeating, count, button));
  send_message(di);
}

void onLongClickDown(ClickRecognizerRef recognizer, void *context) {
  uint8_t count =  click_number_of_clicks_counted(recognizer);
  uint8_t button = click_recognizer_get_button_id(recognizer);
  bool repeating = click_recognizer_is_repeating(recognizer);
  DictionaryIterator *di = begin_message(STATUS_OK, timestamp());
  dict_write_uint32(di, KEY_CLICK, LONG_CLICK_DATA(LONG_CLICK_DOWN, repeating, count, button));
  send_message(di);
}

void onLongClickUp(ClickRecognizerRef recognizer, void *context) {
  uint8_t count =  click_number_of_clicks_counted(recognizer);
  uint8_t button = click_recognizer_get_button_id(recognizer);
  bool repeating = click_recognizer_is_repeating(recognizer);
  DictionaryIterator *di = begin_message(STATUS_OK, timestamp());
  dict_write_uint32(di, KEY_CLICK, LONG_CLICK_DATA(LONG_CLICK_UP, repeating, count, button));
  send_message(di);
}

void click_config_provider(void *context) {
  MyWindow *mw = (MyWindow *)context;
  for (int i = 0; i < NUM_BUTTONS; ++i) {
    if (mw->button_config[i] & BUTTON_WANT_SINGLE_CLICK) {
      window_single_click_subscribe(i, onClick);
    }
    if (mw->button_config[i] & BUTTON_WANT_REPEATED_MASK) {
      window_single_repeating_click_subscribe(i, BUTTON_REPEAT_DELAY(mw->button_config[i]), onClick);
    }
    
    if (mw->button_config[i] & BUTTON_WANT_MULTI_MASK) {
      window_multi_click_subscribe(i, 2, BUTTON_MULTI_MAX(mw->button_config[i]), 0, true, onClick);
    }
    
    if (mw->button_config[i] & BUTTON_LONG_CLICK_MASK) {
      window_long_click_subscribe(i, BUTTON_LONG_CLICK_DELAY(mw->button_config[i]), onLongClickDown,
                                  onLongClickUp);
    } 
  }
}

int pushWindow(MyWindow *mw, DictionaryIterator *rdi) {
  // Show the Window on the watch, with animated=true
  APP_LOG(APP_LOG_LEVEL_DEBUG, "pushWindow mw=%p window=%p", mw, mw == NULL ? NULL : mw->w);
  window_stack_push(mw->w, true);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Pushed window.");
  return 0;
}

int requestClicks(MyWindow *mw, DictionaryIterator *rdi) {
    for (int i = 0; i < NUM_BUTTONS; ++i) {
      Tuple *t =  dict_find(rdi, KEY_BUTTON_0 + i);
      if (t == NULL || t->type != TUPLE_UINT || t->length != 4) {
        continue;
      }
      mw->button_config[i] = t->value->uint32;
    }
    window_set_click_config_provider_with_context(mw->w, click_config_provider, mw);
    return 0;
}

MyWindow *getWindowByID(int id) {
  if (id < 0 || myWindows == NULL || myWindows->count <= id) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to find window at %d, count = %d, myWindows = %p",
           id, myWindows == NULL ? -1 : myWindows->count, myWindows);
    return NULL;
  }
  
  return (MyWindow *)(myWindows->objects[id]);
}
