#include "particle.h"

#include <stdlib.h>
#include "vector3f.h"

struct particle *particle_new() {
  struct particle *p = malloc(sizeof(*p));
  if (!p) return NULL;

  p->pos = (struct vector3f) {0.0f, 0.0f, 0.0f};
  p->acceleration = (struct vector3f) {0.0f, 0.0f, 0.0f};
  p->velocity = (struct vector3f) {0.0f, 0.0f, 0.0f};
  p->color = (struct vector3f) {0.0f, 0.0f, 0.0f};
  p->color_alpha = 1.0f;

  p->mass = 1.0f;

  p->tod_usec = 0l;

  p->active = 0;

  p->bounce = 0.0f;

  return p;
}


void particle_delete(struct particle **p) {
  if (!*p) return;

  free(*p);
}

void particle_copy(struct particle *s, struct particle *t) {
  vector3f_copy(&s->pos, &t->pos);

  vector3f_copy(&s->acceleration, &t->acceleration);
  vector3f_copy(&s->velocity, &t->velocity);

  vector3f_copy(&s->color, &t->color);
  t->color_alpha = s->color_alpha;

  t->mass = s->mass;
  t->tod_usec = s->tod_usec;
  t->active = s->active;
  t->bounce = s->bounce;
}
