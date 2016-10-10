#include "vector.h"

#include <stdlib.h>

struct vector *vector_new(size_t size, size_t growth) {
  struct vector *v = malloc(sizeof(*v));
  if (!v) return NULL;

  v->capacity = size;
  v->size = 0;
  v->growth = growth;

  v->elements = malloc(sizeof(*v->elements) * size);
  for (size_t i=0; i<size; ++i) {
    v->elements[i] = NULL;
  }

  return v;
}

void vector_delete(struct vector **v) {
  if (!*v) return;

  if ((*v)->elements) free((*v)->elements);
  
  *v = NULL;
}

int vector_add(struct vector *v, void *e) {
  if (v->size >= v->capacity) {
    if (v->growth <= 1) return -1;

    v->capacity *= v->growth;

    v->elements = realloc(v->elements, sizeof(*v->elements) * v->capacity);

    for (size_t i=v->size+1; i<v->capacity; ++i) {
      v->elements[i] = NULL;
    }
  }

  v->elements[v->size++] = e;

  return v->size-1;
}

void vector_remove(struct vector *v, size_t i) {
  if (i >= v->size) return;

  v->elements[i] = NULL;
}
