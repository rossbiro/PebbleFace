#include <pebble.h>
  
#define API_VERSION 0x1
  
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
void obj_init_name(void) { obj_name = createObjects(destructor##name); } \
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

void destructorWindow(void *w) {
  window_destroy((Window *)w);
}

int newTextLayer(GRect gr) {
  TextLayer *t = text_layer_create(gr);
  if (t == NULL) {
    return -ENOLAYER;
  }
  return object_new_TextLayer(t);
}

void destructorTextLayer(void *l) {
  destroy_text_layer((TextLayer *)l);
}

enum ObjectType {
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
  KEY_API_VERSION,
  KEY_TRANSACTINO_ID,
  KEY_OBJECT_TYPE,
  KEY_OBJECT_ID,
  KEY_METHOD_ID,
  KEY_RETURN_CODE,
  KEY_RETURN_VALUE,
  KEY_ARG1,
  KEY_ARG2,
  KEY_ARG3,
};

enum Status {
  STATUS_STARTED = 1,
  STATUS_STOPPED = 2,
};

int push_window_wrapper(void *vptr) {
  Winodw *w = (Window *)vptr;
  
}

struct Method {
  ObjectType ot;
  int method_id;
  int m_num_args;
  void *(*getObject)(int);
  void *func;
  bool is_handle;
} method_list[] = {
  {OBJ_WINDOW, METHOD_ID_PUSH_WINDOW, 0, getWindow, push_window_wrapper, false},
};

// Write message to buffer & send
void send_start_message() {
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, KEY_STATUS, STATUS_STARTED);
  dict_write_uint8(iter, KEY_API_VERSION, API_VERSION);
	
	dict_write_end(iter);
  app_message_outbox_send();
}

void callGlobal(uint32_t tid, DictionaryIterator *received) {
  Tuple *tuple = dict_find(recieved, KEY_METHOD_ID);
  if (tuple == NULL) {
    send_err(0, -EINVALID_OP);
    return;
  }
  
  GlobalFuncs gf;
  gf = (int)tuple->value->uint32;
  switch (gf) {
    default:
      send_err(tid, -EINVALID_OP);
      return;
    
    case FUNC_NEW_WINDOW:
      int wh = newWindow();
      if (wh < 0) {
        send_err(tid, wh);
      } else {
        send_res(tid, wh);
      }
    return;
    
    case FUNC_NEW_TEXT_LAYER:
      GRect gr;
      int err = GetGRect(&gr, dict_find(received, KEY_ARG1));
      if (err < 0) {
        send_handle(tid, err);
        return;
      }
    
      int tlh = newTextLayer(g);
      if (tlh < 0) {
        send_err(tid, tlh);
      } else {
        send_handle(tid, tlh)
      }
    return;
  
  }
}

void call_method(int tid, int oid, stuct Method *m,
                 DictionaryIterator *received ) {
    void *obj = m->getObject(oid);
    if (obj == NULL) {
      send_err(tid, -ERRNO_OBJ);
      return;
    }
  
    int err = -ERRINVALID_ARGS;
    switch(m->num_args) {
      case 0:
        err = (METHOD_TYPE_0)(m->func)(obj);
        break;
      
      case 1:
        Tuple a1 = dict_find(received, KEY_ARG1);
        err = (METHOD_TYPE_1)(m->func)(obj, a1);
        break;
      
      case 2:
        Tuple a1 = dict_find(received, KEY_ARG1);
        Tuple a2 = dict_find(received, KEY_ARG2);
        err = (METHOD_TYPE_2)(m->func)(obj, a1, a2);
        break;
      
      case 3:
        Tuple a1 = dict_find(received, KEY_ARG1);
        Tuple a2 = dict_find(received, KEY_ARG2);
        Tuple a3 = dict_find(received, KEY_ARG3);
        err = (METHOD_TYPE_3)(m->func)(obj, a1, a2, a3);
        break;
    }
  
    if (err < 0) {
      send_err(tid, err);
    } else if (m->is_handle) {
      send_handle(tid, err);
    } else {
      send_res(tid, err);
    }
  
}

void callObject(int tid, int ot, DictionaryIterator *received) {
  Tuple t = dict_find(received, KEY_OBJECT_ID);
  if (t == NULL) {
    send_err(tid, -ERRNO_OBJ);
    return;
  }
  
  int oid = (int) t->value->uint32;
  
  t = dict_find(received, KEY_METHOD_ID );
  if (t == NULL) {
    send_err(tid, -ERRINVALID_OP);
      return;
  }
  
  int mid = (int)t->value->uint32;
  
  struct Method *m = getMethod(ot, mid);
  if (m == NULL) {
    send_err(tid, -ERRINVALID_OP);
    return;
  }
  
  call_methd(tid, ot, oid, m, received);
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
	enum ObjectTypes ot;

  tuple = dict_find(received, KEY_TRANSACTION_ID);
  if (tuple == NULL) {
    send_err(0, -EINVALID_TRANSACTION);
    return;
  }
  
  uint32_t tid = tuple->value->uint32;
  
	tuple = dict_find(received, KEY_OBJECT_TYPE);
	if(tuple) {
    ot = (int)tuple->value->uint32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Object Type: %d", (int)tuple->value->uint32); 
	} else {
    ot = OBJ_GLOBAL;
  }
  
  switch (ot) {
    default:
      callObject(tid, ot, received);
      break;
    
    case OBJ_GLOBAL:
      callGlobal(tid, received);
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
  init_windows();
	
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
		
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	send_message();
}

void deinit(void) {
	app_message_deregister_callbacks();
  deinit_windows();
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}
