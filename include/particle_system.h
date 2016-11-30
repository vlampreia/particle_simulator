#ifndef _PARTICLE_SYSTEM_H__
#define _PARTICLE_SYSTEM_H__

#include <stdlib.h>
#include <GL/glut.h>

struct particle;
struct emitter;

struct vertex {
  GLfloat x,y,z;
};

struct vertexb {
  GLubyte x;
  GLubyte y;
  GLubyte z;
  GLubyte a;
};

struct particle_system {
  struct vector *particles;
  struct vertex *particle_pos;
  struct vertexb *particle_col;
  GLubyte       *particle_idx;
  struct vector *emitters;

  double gravity;
  double air_density;
  double friction;

  int collideFloor;
  int collideWalls;

  double attractors[3][4];
};


struct particle_system *particle_system_new     (size_t numParticles);
void                    particle_system_destroy (struct particle_system **s);

void  particle_system_add_emitter (struct particle_system *s, struct emitter *e);

void  particle_system_step  (struct particle_system *s, double t, double dt);

#endif
