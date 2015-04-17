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
    nw->w = NULL;
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
      myTextLayersLoad(mw, mtl);
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

int alloc_window() {
    MyWindow *mw = malloc(sizeof(*MyWindow));
    if (mw == NULL) {
      return -ENOMEM;
    }
    mw->myTextLayers = CreateObject(MyTextLayerDestructor);
    if (mw == NULL) {
      free(mw);
      return -ENOMEM;
    }
  
    mw->w = create_window();
    if (mw->w == NULL) {
      freeObjects(mw->myTextLayers);
      free(mw);
      return -ENOMEM;
    }  

    window_set_user_data(mw->w, mw);
  
    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload
    });
}

