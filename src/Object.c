#include <pebble.h>
#include "Object.h"
#include "Errors.h"

objects *createObjects(ObjectDestructor d) {
  struct objects *o = malloc(sizeof(*o));
  if (o == NULL) {
    return NULL;
  }
  
  o->objects = NULL;
  o->count = 0;
  o->destruct = d;
  
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

void freeObjects(objects *o) {
  for (int i = 0; i < o->count; ++i) {
    if (o->objects[i] != NULL) {
      o->destruct(o->objects[i]);
      o->objects[i] = NULL;
    }
  }
  free(o);
}
