#include "particle_system.h"

#include <math.h>
#include <stdlib.h>

#include "vector.h"
#include "vector3f.h"
#include "particle.h"
#include "emitter.h"


static void _update_emitters(struct emitter **elist, size_t n, int t, int dt);
static void _update_particles(struct vector *plist, int t, int dt);
static void _update_particle(struct particle *p, int t, int dt);
static double myRandom(void)
{
  return ((-RAND_MAX/2 + rand())/(double)RAND_MAX);
}


struct particle_system *particle_system_new(size_t numParticles) {
  struct particle_system *s = malloc(sizeof(*s));
  if (!s) return NULL;

  s->particles = vector_new(numParticles, 0);

  s->emitters_cap = 10;
  s->emitters_count = 0;

  s->emitters = malloc(sizeof(*s->emitters) * s->emitters_cap);
  if (!s->emitters) {
    particle_system_destroy(&s);
    return NULL;
  }

  for (size_t i=0; i<numParticles; ++i) {
    vector_add(s->particles, particle_new());
  }

  return s;
}

void particle_system_destroy(struct particle_system **s) {
  if (!*s) return;

  vector_delete(&(*s)->particles);
  if ((*s)->emitters) free((*s)->emitters);

  *s = NULL;
}


void particle_system_add_emitter(struct particle_system *s, struct emitter *e) {
  if (s->emitters_count >= s->emitters_cap) return;

  s->emitters[s->emitters_count++] = e;
  emitter_set_particle_pool(e, s->particles);
}


void particle_system_step(struct particle_system *s, int t, int dt) {
  _update_emitters(s->emitters, s->emitters_count, t, dt);
  _update_particles(s->particles, t, dt);
}


static void _update_emitters(struct emitter **elist, size_t n, int t, int dt) {
  for (size_t i=0; i<n; ++i) {
    if (!elist[i]) continue;
    if (!elist[i]->firing) continue;
    emitter_step(elist[i], t);
  }
}

static void _update_particles(struct vector *plist, int t, int dt) {
  for (size_t i=0; i<plist->size; ++i) {
    _update_particle(plist->elements[i], t, dt);
  }
}

static void _update_particle(struct particle *p, int t, int dt) {
  if (p->tod_usec <= 0) {
    p->active = 0;
    return;
  }
  p->tod_usec -= dt;

  /**
   * Here we compute basic gravitational force using euler computations:
   * a = F/m
   * v = v' + a * dT
   * p = p' + v * dT
   *
   * a = acceleration   F = force   m = mass  v = velocity  p = position
   */

  double gravity = -0.02f;
  double force = gravity * p->mass;
  double friction = 0.995f;

  //force is static down 1 for now
  p->acceleration.x = 0;
  p->acceleration.y = force / p->mass;
  p->acceleration.z = 0;

  //vector3f_normalise(p->acceleration);
  
  p->velocity.x *= friction;
  p->velocity.z *= friction;

  p->velocity.x += p->acceleration.x;
  p->velocity.y += p->acceleration.y;
  p->velocity.z += p->acceleration.z;

  //vector3f_normalise(p->velocity);

  p->pos.x += p->velocity.x * dt;
  p->pos.y += p->velocity.y * dt;
  p->pos.z += p->velocity.z * dt;


  if (p->pos.y <= 0.0f) {
    p->pos.y = 0.0f;
    p->velocity.y = -p->velocity.y * p->bounce;
    //TODO: this is not real physics, makes surfaces look bumpy though
    p->velocity.z += 0.0001 * myRandom();
    p->velocity.x += 0.0001 * myRandom();
  }

  if (p->pos.x >= 400.0f) {
    p->pos.x = 400.0f;
    p->velocity.x = -p->velocity.x * p->bounce;

    //probably not real physics but looks good
    p->velocity.z += 0.01 * myRandom();
  }

  if (p->pos.x <= -400.0f) {
    p->pos.x = -400.0f;
    p->velocity.x = -p->velocity.x * p->bounce;
    p->velocity.z += 0.01 * myRandom();
  }

  if (p->pos.z >= 400.0f) {
    p->pos.z = 400.0f;
    p->velocity.z = -p->velocity.z * p->bounce;
    p->velocity.x += 0.01 * myRandom();
  }

  if (p->pos.z <= -400.0f) {
    p->pos.z = -400.0f;
    p->velocity.z = -p->velocity.z * p->bounce;
    p->velocity.x += 0.01 * myRandom();
  }
}
