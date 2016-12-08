#ifndef _PARTICLE_H__
#define _PARTICLE_H__

#include "vertex.h"

struct emitter;

struct particle {
  //GLfloat pos[3];
  struct vertex acceleration;
  struct vertex velocity;

  double mass;
  double collision_chaos;

  double tod_usec;
  double tod_max;

  double bounce;

  int active;

  struct emitter *emitter;

//  GLubyte color[4];
  struct vertex_col base_color;

  size_t pos_idx;

  float size;
};


struct particle *particle_new     (struct emitter *emitter, size_t idx);
void             particle_delete  (struct particle **p);

void             particle_copy    (struct particle *s, struct particle *t);

#endif
