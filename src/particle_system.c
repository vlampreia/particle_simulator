#include "particle_system.h"

#include <math.h>
#include <stdlib.h>

#include "vector.h"
#include "vector3f.h"
#include "particle.h"
#include "emitter.h"


static void _update_emitters(struct vector *elist, int t, int dt);
static void _update_particles(struct particle_system *s, struct vector *plist, int t, int dt);
static void _update_particle(struct particle_system *s, struct particle *p, int t, int dt);
static double myRandom(void)
{
  return ((-RAND_MAX/2 + rand())/(double)RAND_MAX/2);
}
static double myPosRandom(void) {
  return rand()/(double)RAND_MAX;
}
static double _clampedRand(double min, double max) {
  return min + rand() / ((double)RAND_MAX/(max-min));
}
static double _clamp_double(double v, double min, double max) {
  if (v > max) return max - v;
  if (v < min) return min + v;
  return v;
}


struct particle_system *particle_system_new(size_t numParticles) {
  struct particle_system *s = malloc(sizeof(*s));
  if (!s) return NULL;

  s->particles = vector_new(numParticles, 0);
  s->emitters = vector_new(10, 0);

  for (size_t i=0; i<numParticles; ++i) {
    vector_add(s->particles, particle_new());
  }

  s->gravity = 0.02f;

  return s;
}

void particle_system_destroy(struct particle_system **s) {
  if (!*s) return;

  vector_delete(&(*s)->particles);
  vector_delete(&(*s)->emitters);

  *s = NULL;
}


void particle_system_add_emitter(struct particle_system *s, struct emitter *e) {
  if (vector_add(s->emitters, e) == -1) return;
  emitter_set_particle_pool(e, s->particles);
}


void particle_system_step(struct particle_system *s, int t, int dt) {
  _update_emitters(s->emitters, t, dt);
  _update_particles(s, s->particles, t, dt);
}


static void _update_emitters(struct vector *elist, int t, int dt) {
  for (size_t i=0; i<elist->size; ++i) {
    if (!elist->elements[i]) continue;
    if (!((struct emitter*)elist->elements[i])->firing) continue;
    emitter_step(elist->elements[i], t);
  }
}

static void _update_particles(struct particle_system *s, struct vector *plist, int t, int dt) {
  for (size_t i=0; i<plist->size; ++i) {
    _update_particle(s, plist->elements[i], t, dt);
  }
}

static inline double _max_double(double a, double b) {
  return a > b ? a : b;
}

static void _update_particle(struct particle_system *s, struct particle *p, int t, int dt) {
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

  double force = -s->gravity * p->mass;
  double friction = 0.995f;
  double collision_chaos = 0.1f;


  //force is static down 1 for now
  p->acceleration.x = 0.0f;
  p->acceleration.y = force / p->mass;
  p->acceleration.z = 0.0f;

  //vector3f_normalise(p->acceleration);
  
  p->velocity.x *= friction;
  p->velocity.y *= friction;
  p->velocity.z *= friction;

  p->velocity.x += p->acceleration.x;
  p->velocity.y += p->acceleration.y;
  p->velocity.z += p->acceleration.z;

  //vector3f_normalise(p->velocity);

  p->pos.x += p->velocity.x * dt;
  p->pos.y += p->velocity.y * dt;
  p->pos.z += p->velocity.z * dt;

  //  particle-particle collision code is suboptimal..
//  for (size_t i=0; i<s->particles->size; ++i) {
//    struct particle *_p = s->particles->elements[i];
//    if (_p == p || !_p->active) continue;
//    if (
//      (int)p->pos.x >= (int)_p->pos.x -1 && (int)p->pos.x <= (int)_p->pos.x + 1 &&
//      (int)p->pos.y >= (int)_p->pos.y -1 && (int)p->pos.y <= (int)_p->pos.y + 1 &&
//      (int)p->pos.z >= (int)_p->pos.z -1 && (int)p->pos.z <= (int)_p->pos.z + 1
//    ) {
//      double vx = p->velocity.x;
//      double vy = p->velocity.y;
//      double vz = p->velocity.z;
//
//      p->velocity.x = _p->velocity.x;
//      p->velocity.y = _p->velocity.y;
//      p->velocity.z = _p->velocity.z;
//
//      _p->velocity.x = vx;
//      _p->velocity.y = vy;
//      _p->velocity.z = vz;
//      break;
//    }
//  }

  if (p->pos.y <= 0.0f) {
    p->pos.y = 0.0f;
    p->velocity.y = -p->velocity.y * p->bounce * _clampedRand(1.0f-collision_chaos, 1.0f);
    //TODO: this is not real physics, makes surfaces look bumpy though
    p->velocity.x += 0.001 * myRandom();
    p->velocity.z += 0.001 * myRandom();
//    p->velocity.z += 0.1 * p->bounce * _clampedRand(-collision_chaos, collision_chaos);
//    p->velocity.x += 0.1 * p->bounce * _clampedRand(-collision_chaos, collision_chaos);
  }

  if (p->pos.x >= 400.0f) {
    p->pos.x = 400.0f;
    p->velocity.x = friction * -p->velocity.x * p->bounce * _clampedRand(1.0f-collision_chaos, p->bounce);
    p->velocity.z += collision_chaos * myRandom();
  }

  if (p->pos.x <= -400.0f) {
    p->pos.x = -400.0f;
    p->velocity.x = friction * -p->velocity.x * p->bounce * _clampedRand(1.0f-collision_chaos, p->bounce);
    p->velocity.z += collision_chaos * myRandom();
  }

  if (p->pos.z >= 400.0f) {
    p->pos.z = 400.0f;
    p->velocity.z = friction * -p->velocity.z * p->bounce * _clampedRand(1.0f-collision_chaos, p->bounce);
    p->velocity.x += collision_chaos * myRandom();
  }

  if (p->pos.z <= -400.0f) {
    p->pos.z = -400.0f;
    p->velocity.z = friction * -p->velocity.z * p->bounce * _clampedRand(1.0f-collision_chaos, p->bounce);
    p->velocity.x += collision_chaos * myRandom();
  }
}
