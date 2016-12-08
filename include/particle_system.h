#ifndef _PARTICLE_SYSTEM_H__
#define _PARTICLE_SYSTEM_H__

#include <stdlib.h>
#include <GL/glut.h>

#include "vertex.h"

struct particle;
struct emitter;

struct particle_system {
  struct vector *particles;
  struct vertex *particle_pos;
  struct vertex_col *particle_col;
  GLubyte       *particle_idx;
  struct vector *emitters;

  double gravity;
  double air_density;
  double friction;

  int collideFloor;
  int collideWalls;
  int trip;

  size_t num_attractors;
  double *attractors;
  //double attractors[3][4];
  

  int isCollisionEnabled;
};


struct particle_system *particle_system_new     (size_t numParticles);
void                    particle_system_destroy (struct particle_system **s);

void  particle_system_add_emitter (struct particle_system *s, struct emitter *e);

int  particle_system_step  (struct particle_system *s, double t, double dt);

void particle_system_set_particle_pos(
      struct particle_system *s, struct particle *p, struct vertex pos);

void particle_system_set_particle_col(
      struct particle_system *s, struct particle *p, struct vertex_col col);

void particle_system_reset(struct particle_system *s);
#endif
