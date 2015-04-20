#include <pebble.h>
#include "Window.h"
#include "Protocol.h"
#include "TextLayer.h"
  
static Window *background_window;

// Write message to buffer & send
void send_start_message() {
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, KEY_STATUS, STATUS_STARTED);
  dict_write_uint8(iter, KEY_API_VERSION, API_VERSION);
	
	dict_write_end(iter);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "sending start message.");
  app_message_outbox_send();
}

void send_err(uint32_t tid, int res) {
  DictionaryIterator *iter;

  app_message_outbox_begin(&iter);
	dict_write_uint8(iter, KEY_STATUS, STATUS_ERR);
  dict_write_uint16(iter, KEY_API_VERSION, API_VERSION);
  dict_write_uint32(iter, KEY_TRANSACTION_ID, tid);
  dict_write_uint32(iter, KEY_ERROR_CODE, -res);
	
	dict_write_end(iter);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sending error code %d", res);
  app_message_outbox_send();
  
}

void send_handle(uint32_t tid, int key, int res) {
  DictionaryIterator *iter;
  
  if (res < 0) {
    send_err(tid, res);
    return;
  }

	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, KEY_STATUS, STATUS_OK);
  dict_write_uint16(iter, KEY_API_VERSION, API_VERSION);
  dict_write_uint32(iter, KEY_TRANSACTION_ID, tid);
  dict_write_uint32(iter, key, res);
	
	dict_write_end(iter);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "sending handle type %d: %d", key, res);
  
  app_message_outbox_send();
  
}

void send_result(uint32_t tid, int res) {
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

void newWindowWrapper(uint32_t tid, DictionaryIterator *rdi) {
  // No args necessary.  So we just do our thing.
  int wh = allocWindow();
  send_handle(tid, KEY_WINDOW_ID, wh);
}

void newTextLayerWrapper(uint32_t tid, DictionaryIterator *rdi) {
  // We need to get a window handle and covert that to a window
  // before we can create a new text layer.
  MyWindow *mw;
  int ret=0;
  
  RCC(getWindowFromRemote(rdi, &mw)); // mw is output

  ret = createTextLayer(mw);
error_out:
  send_handle(tid, KEY_TEXT_LAYER_ID, ret);
}

void applyAttributesWrapper(uint32_t tid, DictionaryIterator *rdi) {
  int ret = 0;
  MyTextLayer *mtl = NULL;
  
  RCC(getTextLayerFromRemote(rdi, &mtl, NULL));
  RCC(myTextLayerSetAttributes(mtl, rdi));
  
error_out:
  send_result(tid, ret);
}

void pushWindowWrapper(uint32_t tid, DictionaryIterator *rdi) {
  int ret;
  MyWindow *mw;
  
  RCC(getWindowFromRemote(rdi, &mw));
  pushWindow(mw);
  
error_out:
  send_result(tid, ret);
}

typedef void (*RemoteCallWrapper) (uint32_t, DictionaryIterator *);

struct { 
  RemoteFuncs id;
  RemoteCallWrapper rcw;
} remoteCalls[] = {
  {FUNC_NEW_WINDOW, newWindowWrapper},
  {FUNC_NEW_TEXT_LAYER, newTextLayerWrapper},
  {FUNC_APPLY_ATTRIBUTES, applyAttributesWrapper},
  {FUNC_PUSH_WINDOW, pushWindowWrapper},
  {FUNC_NO_FUNC, NULL},
};

void callGlobal(uint32_t tid, DictionaryIterator *received) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In callGlobal");
  
  Tuple *tuple = dict_find(received, KEY_METHOD_ID);
  if (tuple != NULL || tuple->type != TUPLE_UINT) {
    int gf = (int)tuple->value->uint32;
    APP_LOG(APP_LOG_LEVEL_DEBUG," callGlobal gf = %d", gf);
    for (int i = 0; remoteCalls[i].id != FUNC_NO_FUNC; ++i) {
      if (remoteCalls[i].id == gf) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Found Func: %d", i);
        remoteCalls[i].rcw(tid, received);
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
  init_windows();
	
  background_window = window_create();
  window_stack_push(background_window, false);
  
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
		
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	//send_start_message();
}

void deinit(void) {
	app_message_deregister_callbacks();
  window_destroy(background_window);
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
