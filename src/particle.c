#include "particle.h"

#include <stdlib.h>

#include "vertex.h"
#include "emitter.h"

struct particle *particle_new(struct emitter *emitter, size_t idx) {
  struct particle *p = malloc(sizeof(*p));
  if (!p) return NULL;

//  p->pos[0] = 0.0f;
//  p->pos[1] = 0.0f;
//  p->pos[2] = 0.0f;
  p->acceleration.x = 0.0f;
  p->acceleration.y = 0.0f;
  p->acceleration.z = 0.0f;
  p->velocity.x = 0.0f;
  p->velocity.y = 0.0f;
  p->velocity.z = 0.0f;
//  p->color[0] = 0;
//  p->color[1] = 0;
//  p->color[2] = 0;
//  p->color[3] = 255;

//  p->base_color[0] = p->color[0];
//  p->base_color[1] = p->color[1];
//  p->base_color[2] = p->color[2];
//  p->base_color[3] = p->color[3];

  p->emitter = emitter;


  //p->pos = (struct vector3f) {0.0f, 0.0f, 0.0f};
  //p->acceleration = (struct vector3f) {0.0f, 0.0f, 0.0f};
  //p->velocity = (struct vector3f) {0.0f, 0.0f, 0.0f};
  //p->color = (struct vector3f) {0.0f, 0.0f, 0.0f};
  //p->color_alpha = 1.0f;

  p->collision_chaos = 0.0f;

  p->mass = 1.0f;

  p->size =1.0f;

  p->tod_usec = 0l;
  p->tod_max = 1l;

  p->active = 0;

  p->bounce = 0.0f;

  p->pos_idx = idx;

  return p;
}


void particle_delete(struct particle **p) {
  if (!*p) return;

  free(*p);

  *p = NULL;
}

void particle_copy(struct particle *s, struct particle *t) {
  //GLfloat_copy(s->pos, t->pos);
  vertex_copy(&s->acceleration, &t->acceleration);
  vertex_copy(&s->velocity,     &t->velocity);
//  t->color[0] = s->color[0];
//  t->color[1] = s->color[1];
//  t->color[2] = s->color[2];
//  t->color[3] = s->color[3];

  t->size = s->size;

  t->base_color.r = s->base_color.r;
  t->base_color.g = s->base_color.g;
  t->base_color.b = s->base_color.r;
  t->base_color.a = s->base_color.r;

  //vector3f_copy(&s->pos, &t->pos);

  //vector3f_copy(&s->acceleration, &t->acceleration);
  //vector3f_copy(&s->velocity, &t->velocity);

  //vector3f_copy(&s->color, &t->color);
  //t->color_alpha = s->color_alpha;

  t->mass = s->mass;
  t->tod_usec = s->tod_usec;
  t->tod_max = s->tod_max;
  t->active = s->active;
  t->bounce = s->bounce;
  t->collision_chaos = s->collision_chaos;
}
