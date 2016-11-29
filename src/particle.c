#include "particle.h"

#include <stdlib.h>
#include "vector3f.h"

struct particle *particle_new(void) {
  struct particle *p = malloc(sizeof(*p));
  if (!p) return NULL;

  p->pos[0] = 0.0f;
  p->pos[1] = 0.0f;
  p->pos[2] = 0.0f;
  p->acceleration[0] = 0.0f;
  p->acceleration[1] = 0.0f;
  p->acceleration[2] = 0.0f;
  p->velocity[0] = 0.0f;
  p->velocity[1] = 0.0f;
  p->velocity[2] = 0.0f;
  p->color[0] = 0;
  p->color[1] = 0;
  p->color[2] = 0;
  p->color[3] = 255;


  //p->pos = (struct vector3f) {0.0f, 0.0f, 0.0f};
  //p->acceleration = (struct vector3f) {0.0f, 0.0f, 0.0f};
  //p->velocity = (struct vector3f) {0.0f, 0.0f, 0.0f};
  //p->color = (struct vector3f) {0.0f, 0.0f, 0.0f};
  //p->color_alpha = 1.0f;

  p->collision_chaos = 0.0f;

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
  GLfloat_copy(s->pos, t->pos);
  GLfloat_copy(s->acceleration, t->acceleration);
  GLfloat_copy(s->velocity, t->velocity);
  t->color[0] = s->color[0];
  t->color[1] = s->color[1];
  t->color[2] = s->color[2];
  t->color[3] = s->color[3];

  //vector3f_copy(&s->pos, &t->pos);

  //vector3f_copy(&s->acceleration, &t->acceleration);
  //vector3f_copy(&s->velocity, &t->velocity);

  //vector3f_copy(&s->color, &t->color);
  //t->color_alpha = s->color_alpha;

  t->mass = s->mass;
  t->tod_usec = s->tod_usec;
  t->active = s->active;
  t->bounce = s->bounce;
  t->collision_chaos = s->collision_chaos;
}
