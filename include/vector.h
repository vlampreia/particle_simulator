#ifndef _VECTOR_H__
#define _VECTOR_H__

#include <stdlib.h>

struct vector {
  size_t capacity;
  size_t size;
  size_t growth;

  void **elements;
};

struct vector *vector_new     (size_t size, size_t growth);
void           vector_delete  (struct vector **v);

int  vector_add    (struct vector *v, void *e);
void vector_remove (struct vector *v, size_t i);

#endif
