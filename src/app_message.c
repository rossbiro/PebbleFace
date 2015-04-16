#include <pebble.h>
  
typedef (*ObjectDestructor)(void *);
struct objects {
  void **objects;
  short count;
  ObjectDestructor destruct;
};

objects *createObjects(ObjectDestruct d) {
  struct objects *o = malloc(sizeof(*o));
  if (o == NULL) {
    return NULL;
  }
  
  o->objects = NULL;
  o->count = 0;
  o->destructor = d;
  
  return o;
  
}

int allocObjects(struct objects *objects, void *new_obj) {
  for (int i = 0; i < objects->count; ++i) {
    if (objects->objects[i] == NULL) {
        objects->objects[i] = new_obj;
        return i;
    }
  }

  // need to allocate some more.
  short new_object_count = objects->count == 0 ? 1 : objects->count * 2;
  void **new_objects = realloc(objects->objects, sizeof(*new_objects) * new_object_count);
  if (new_objects == NULL) {
    objects->destruct(new_obj);
    return -ENOMEM;
  }
  
  memset(new_objects + objects->count, 0, (new_object_count - objects->count) * sizeof(*new_objects));
  objects->count = new_object_count;
  objects->objects = new_objects;
  return allocObjects(objects, new_obj); // hopefully the compiler figures out this is tail recursion.
}

void freeObjects(obiects *o) {
  for (int i = 0; i < o->count; ++i) {
    if (o->objects[i] != NULL) {
      o->destruct(o->objects[i]);
      o->objects[i] = NULL;
    }
  }
  free(o);
}

#define DEFINE_OBJECT(name) \
\
struct objects *obj_name; \
void obj_init_name(void) { obj_name = createObjects(destructor_name); } \
int obj_new_name(name *n) { return allocObjects(obj_name, n); } \
void obj_deinit_name(void) {freeObjects(obj_name); obj_name = NULL; }

DEFINE_OBJECT(Window)
DEFINE_OBJECT(TextLayer)

int newWindow() {
  Window *w = window_create();
  if (w == NULL) {
    return -ENOWINDOW;
  }
  
  return object_new_Window(w);
}

void destructor_Window(void *w) {
  window_destroy(w);
}

int newTextLayer(GRect gr) {
  TextLayer *t = text_layer_create(gr);
  if (t == NULL) {
    return -ENOLAYER;
  }
  return object_new_TextLayer(t);
}

enum ObjectTypes {
  OBJ_GLOBAL = 0, //no object
  OBJ_WINDOW = 1,
  OBJ_TEXT_LAYER = 2,
};

enum GlobalFuncs {
  FUNC_NO_FUNC = 0,
  FUNC_NEW_WINDOW = 1,
  FUNC_NEW_TEXT_LAYER = 2,
};

enum Keys {
  KEY_STATUS = 0,
  KEY_API_VERSION=1,
  KEY_OBJECT_TYPE = 2,
  KEY_OBJECT_ID = 3,
  KEY_METHOD_ID = 4,
  KEY_RETURN_CODE = 5,
  KEY_RETURN_VALUE = 6,
  KEY_ARG1 = 7,
  KEY_ARG2 = 8,
  KEY_ARG3 = 9,
};

enum Status {
  STATUS_STARTED = 1,
  STATUS_STOPPED = 2,
};

// Write message to buffer & send
void send_start_message() {
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, STATUS_KEY, STATUS_STARTED);
	
	dict_write_end(iter);
  app_message_outbox_send();
}

void callGlobal(DictionaryIterator *received) {
  Tuple *tuple = dict_find(recieved, KEY_METHOD_ID);
  if (tuple == NULL) {
    send_err(-EINVALID_OP);
    return;
  }
  
  GlobalFuncs gf;
  gf = (int)tuple->value->uint32;
  switch (gf) {
    case FUNC_NEW_WINDOW:
      
  }
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
	enum ObjectTypes ot;
  
	tuple = dict_find(received, KEY_OBJECT_TYPE);
	if(tuple) {
    ot = (int)tuple->value->uint32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Object Type: %d", (int)tuple->value->uint32); 
	} else {
    ot = OBJ_GLOBAL;
  }
  
  switch (ot) {
    default:
      caollObject(ot, received);
      break;
    
    case OBJ_GLOBAL:
      callGlobal(received);
      break;
  }
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

void init(void) {
	obj_init_Window();
  obj_init_TextLayer();
	
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
		
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	send_message();
}

void deinit(void) {
	app_message_deregister_callbacks();
	obj_deinit_TextLayer();
  obj_deinit_Window();
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}
