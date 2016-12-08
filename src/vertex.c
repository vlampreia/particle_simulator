#include "vertex.h"
#include <math.h>

void vertex_copy(struct vertex *s, struct vertex *t) {
  t->x = s->x;
  t->y = s->y;
  t->z = s->z;
}

void vertex_normalise(struct vertex *v) {
  GLfloat norm = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
  v->x /= norm;
  v->y /= norm;
  v->z /= norm;
}
