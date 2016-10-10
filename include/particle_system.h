#ifndef _PARTICLE_SYSTEM_H__
#define _PARTICLE_SYSTEM_H__

#include <stdlib.h>

struct particle;
struct emitter;

struct particle_system {
  struct vector *particles;
  struct vector *emitters;

  double gravity;
};


struct particle_system *particle_system_new     (size_t numParticles);
void                    particle_system_destroy (struct particle_system **s);

void  particle_system_add_emitter (struct particle_system *s, struct emitter *e);

void  particle_system_step  (struct particle_system *s, int t, int dt);


#endif
