#ifndef _EMITTER_H__
#define _EMITTER_H__

struct particle;
struct particle_system;
#include "vector.h"
#include "vertex.h"


struct emitter {
  struct particle *base_particle;

  struct vertex position;
  
  double pitch, yaw;

  double horiz_angle;
  double vert_angle;

  double force;

  double frequency;

  struct vector *particle_pool;

  double last_fire_t;
  int firing;

  int emission_count;

  struct particle_system *psystem;
};

struct emitter *emitter_new (struct vector *particle_pool);
void emitter_delete         (struct emitter **e);

void emitter_fire           (struct emitter *e, int count);
void emitter_step           (struct emitter *e, double t);

void emitter_set_particle_pool  (struct emitter *e, struct vector *particle_pool);

#endif
