#ifndef REMOTESCREEN_OBJECT_H_
#define REMOTESCREEN_OBJECT_H_
  
#include <pebble.h>
typedef void (*ObjectDestructor)(void *);

typedef struct objects {
  void **objects;
  short count;
  ObjectDestructor destruct;
} objects;

struct objects *CreateObjects(ObjectDestructor);
int allocObjects(struct objects *objects, void *new_obj);
void freeObjects(objects *o);

#define DEFINE_OBJECT(name) \
struct objects *obj_name; \
void obj_init_name(void) { obj_name = createObjects(destructor##name); } \
int obj_new_name(name *n) { return allocObjects(obj_name, n); } \
void obj_deinit_name(void) {freeObjects(obj_name); obj_name = NULL; }

#endif