#ifndef _EMITTER_H__
#define _EMITTER_H__

struct particle;
#include "vector3f.h"


struct emitter {
  struct particle *base_particle;

  struct vector3f position;
  struct vector3f orientation;
  
  double horiz_angle;
  double vert_angle;

  double force;

  int frequency;

  struct particle **particle_pool;

  int last_fire_t;
};

struct emitter *emitter_new (struct particle **particle_pool);
void emitter_delete         (struct emitter **e);

void emitter_fire           (struct emitter *e);
void emitter_step           (struct emitter *e, int t);

#endif
