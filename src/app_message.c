#include <pebble.h>
#include "Window.h"
#include "Protocol.h"
#include "TextLayer.h"


DictionaryIterator *begin_message(int status, uint32_t tid) {
  DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
  dict_write_uint8(iter, KEY_API_VERSION, API_VERSION);
 	dict_write_uint8(iter, KEY_STATUS, status); 
  dict_write_uint32(iter, KEY_TRANSACTION_ID, tid);
  return iter;

}

void send_message(DictionaryIterator *iter) {
 	dict_write_end(iter);
  app_message_outbox_send(); 
}


static void send_err(uint32_t tid, int res) {
  DictionaryIterator *iter = begin_message(STATUS_ERR, tid);
  dict_write_uint32(iter, KEY_ERROR_CODE, -res);
	send_message(iter);
  
}

static void send_handle(uint32_t tid, int key, int res) {
  DictionaryIterator *iter;
  
  if (res < 0) {
    send_err(tid, res);
    return;
  }

	iter = begin_message(STATUS_OK, tid);
  dict_write_uint32(iter, key, res);
  send_message(iter);	
}

static void send_result(uint32_t tid, int res) {
  send_handle (tid, KEY_RETURN_VALUE, res);
}

static int getWindowFromRemote(DictionaryIterator *rdi, MyWindow **mw /* output */) {
  Tuple *t = dict_find(rdi, KEY_WINDOW_ID);
  int wh;

  if (t == NULL || t->type != TUPLE_UINT) {
    return -ENOWINDOW;
  }
  
  if (mw == NULL) {
    return 0;
  }
  
  wh = (int) t->value->uint32;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got Window Handle %d", wh);
  *mw = getWindowByID(wh);
  if (*mw == NULL) {
    return -ENOWINDOW;
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Found Window %p", *mw);
  return 0;
}

static int getTextLayerFromRemote(DictionaryIterator *rdi, MyTextLayer **mtl, MyWindow **mw) {
  int ret;
  int tlh;
  Tuple *t;
  MyWindow *lmw;
  
  RCC(getWindowFromRemote(rdi, &lmw));
  if (mw != NULL) {
    *mw = lmw;
  }
  
  t = dict_find(rdi, KEY_TEXT_LAYER_ID);
  
  if (t == NULL || t->type != TUPLE_UINT) {
    return -ENOLAYER;
  }
  
  tlh = (int) t->value->uint32;
  if (lmw == NULL) {
    return 0;
  }
  
  *mtl = getTextLayerByID(lmw, tlh);
  
  if (*mtl == NULL) {
    return -ENOLAYER;
  }
error_out:
  return 0;
}

void newWindowWrapper(uint32_t tid, DictionaryIterator *rdi, void *unused) {
  // No args necessary.  So we just do our thing.
  int wh = allocWindow();
  send_handle(tid, KEY_WINDOW_ID, wh);
}

void newTextLayerWrapper(uint32_t tid, DictionaryIterator *rdi, void *unused) {
  // We need to get a window handle and covert that to a window
  // before we can create a new text layer.
  MyWindow *mw;
  int ret=0;
  
  RCC(getWindowFromRemote(rdi, &mw)); // mw is output

  ret = createTextLayer(mw);
error_out:
  send_handle(tid, KEY_TEXT_LAYER_ID, ret);
}

void applyAttributesWrapper(uint32_t tid, DictionaryIterator *rdi, void *unused) {
  int ret = 0;
  MyTextLayer *mtl = NULL;
  MyWindow *mw = NULL;
  
  RCC(getTextLayerFromRemote(rdi, &mtl, &mw));
  RCC(myTextLayerSetAttributes(mw, mtl, rdi));
  
error_out:
  send_result(tid, ret);
}


typedef int (*window_func)(MyWindow *, DictionaryIterator *rdi);

void call_window_func(uint32_t tid, DictionaryIterator *rdi, void *vfunc) {
  int ret;
  MyWindow *mw;
  window_func func = (window_func) vfunc;
  
  RCC(getWindowFromRemote(rdi, &mw));
  RCC(func(mw, rdi));
  
error_out:
  send_result(tid, ret);
}

typedef void (*RemoteCallWrapper) (uint32_t, DictionaryIterator *, void *data);

struct { 
  RemoteFuncs id;
  RemoteCallWrapper rcw;
  void *arg;
} remoteCalls[] = {
  {FUNC_NEW_WINDOW, newWindowWrapper, NULL},
  {FUNC_NEW_TEXT_LAYER, newTextLayerWrapper, NULL},
  {FUNC_APPLY_ATTRIBUTES, applyAttributesWrapper, NULL},
  {FUNC_PUSH_WINDOW, call_window_func, (void *)pushWindow },
  {FUNC_REQUEST_CLICKS, call_window_func, (void *)requestClicks },
  {FUNC_NO_FUNC, NULL, NULL},
};

void callGlobal(uint32_t tid, DictionaryIterator *received) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In callGlobal");
  
  Tuple *tuple = dict_find(received, KEY_METHOD_ID);
  if (tuple != NULL && tuple->type == TUPLE_UINT) {
    int gf = (int)tuple->value->uint32;
    APP_LOG(APP_LOG_LEVEL_DEBUG," callGlobal gf = %d", gf);
    for (int i = 0; remoteCalls[i].id != FUNC_NO_FUNC; ++i) {
      if (remoteCalls[i].id == gf) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Found Func: %d", i);
        remoteCalls[i].rcw(tid, received, remoteCalls[i].arg);
        return;
      }
    }
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "callGlobal returining -EINVALID_OP");
  send_err(tid, -EINVALID_OP);
  return;  
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message:");
  tuple = dict_find(received, KEY_TRANSACTION_ID);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "dict[%d] = %p", KEY_TRANSACTION_ID, tuple);
  if (tuple == NULL) {
    send_err(0, -EINVALID_TRANSACTION);
    return;
  }
  
  uint32_t tid = tuple->value->uint32;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: tid = %d", (int)tid);
  callGlobal(tid, received);
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "out_failed_handler: %d", reason);
}

void init(void) {
  int rh;
  MyWindow *mw;
  
  init_windows();
	
  // Need to create a window to keep the app from
  // exiting, so we might as wll make it available.
  rh = allocWindow();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "root window handle = %d", rh);
  if (rh != 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Root window handle %d != 0", rh);
  }
  mw = getWindowByID(rh);
  if (mw == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Root window null");
  }
  
  pushWindow(mw, NULL);
  
  
  
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
		
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	//send_start_message();
}

void deinit(void) {
	app_message_deregister_callbacks();
  deinit_windows();
}

int main( void ) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "PebbleRemote: in Main");
	init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "PebbleRemote: Done with init");
  app_event_loop();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "PebbleRemote: Calling deinit");
	deinit();
}
