#include "attractor.h"
#include "vertex.h"

#include <memory.h>

struct attractor *attractor_new(struct vertex pos, long mass) {
  struct attractor *a = malloc(sizeof(*a));
  if (!a) return NULL;

  a->pos = pos;
  a->mass = mass;
  a->enabled = 0;

  return a;
}

void attractor_delete(struct attractor **a) {
  if (!*a) return;

  free(*a);

  *a = NULL;
}
