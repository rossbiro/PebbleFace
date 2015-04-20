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

void pushWindow(MyWindow *mw) {
  // Show the Window on the watch, with animated=true
  APP_LOG(APP_LOG_LEVEL_DEBUG, "pushWindow mw=%p window=%p", mw, mw == NULL ? NULL : mw->w);
  window_stack_push(mw->w, true);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Pushed window.");
}

MyWindow *getWindowByID(int id) {
  if (id < 0 || myWindows == NULL || myWindows->count <= id) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to find window at %d, count = %d, myWindows = %p",
           id, myWindows == NULL ? -1 : myWindows->count, myWindows);
    return NULL;
  }
  
  return (MyWindow *)(myWindows->objects[id]);
}
