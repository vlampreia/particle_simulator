#ifndef _EMITTER_H__
#define _EMITTER_H__

struct particle;
#include "vector3f.h"
#include "vector.h"


struct emitter {
  struct particle *base_particle;

  struct vector3f position;
  
  double pitch, yaw;

  double horiz_angle;
  double vert_angle;

  double force;

  int frequency;

  struct vector *particle_pool;

  int last_fire_t;
  int firing;
};

struct emitter *emitter_new (struct vector *particle_pool);
void emitter_delete         (struct emitter **e);

void emitter_fire           (struct emitter *e);
void emitter_step           (struct emitter *e, int t);

void emitter_set_particle_pool  (struct emitter *e, struct vector *particle_pool);

#endif
