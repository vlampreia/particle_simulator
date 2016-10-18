#include "particle_system.h"

#include <math.h>
#include <stdlib.h>

#include "vector.h"
#include "vector3f.h"
#include "particle.h"
#include "emitter.h"


static void _update_emitters(struct vector *elist, int t, int dt);
static void _update_particles(struct particle_system *s, struct vector *plist, int t, int dt);
static inline void _update_particle_pos(struct particle_system *s, struct particle *p, int t, int dt);
static inline void _update_particle_collision(struct particle_system *s, struct particle *p, int t, int dt);
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

  s->gravity = 9.807f;

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
    struct particle *p = plist->elements[i];
    if (!p->active) continue;
    if (p->tod_usec <= 0) {
      p->active = 0;
      continue;
    }
    p->tod_usec -= dt;

    _update_particle_pos(s, plist->elements[i], t, dt);
  }

  for (size_t i=0; i<plist->size; ++i) {
    struct particle *p = plist->elements[i];
    if (!p->active) continue;

    _update_particle_collision(s, plist->elements[i], t, dt);
  }
}

static inline double _max_double(double a, double b) {
  return a > b ? a : b;
}

static inline void _update_particle_pos(struct particle_system *s, struct particle *p, int t, int dt) {
  double force = -s->gravity * p->mass;
  //double friction = 0.990f;
  double friction = 4;
  double airdens = 1.2;


  //force is static down 1 for now
  p->acceleration[0] = 0.0f;
  p->acceleration[1] = force / p->mass;
  p->acceleration[1] -= 0.5 * airdens * 0.47 * p->velocity[1];
  p->acceleration[2] = 0.0f;

//  p->acceleration[0] += -p->velocity[0] /p->mass ;
//  p->acceleration[2] += -p->velocity[2] /p->mass ;
//  if (p->velocity[0] > 0.0f) p->acceleration[0] += -friction/p->mass;
//  else                       p->acceleration[0] +=  friction/p->mass;
//  if (p->velocity[2] > 0.0f) p->acceleration[2] += -friction/p->mass;
//  else                       p->acceleration[2] +=  friction/p->mass;

  //vector3f_normalise(p->acceleration);
  
//  p->velocity[0] *= friction;
//  p->velocity[1] *= friction;
//  p->velocity[2] *= friction;

  p->velocity[0] += p->acceleration[0];
  p->velocity[1] += p->acceleration[1];
  p->velocity[2] += p->acceleration[2];

  //vector3f_normalise(p->velocity);

  p->pos[0] += p->velocity[0];
  p->pos[1] += p->velocity[1];
  p->pos[2] += p->velocity[2];
}

static inline void _update_particle_collision(struct particle_system *s, struct particle *p, int t, int dt) {
  //  particle-particle collision code is suboptimal..
  for (size_t i=0; i<s->particles->size; ++i) {
    struct particle *_p = s->particles->elements[i];
    if (_p == p || !_p->active) continue;

    double dx = p->pos[0] - _p->pos[0];
    double dy = p->pos[1] - _p->pos[1];
    double dz = p->pos[2] - _p->pos[2];

    //assume radius of 5
    if (dx*dx + dy*dy + dz*dz < 100) {
      p->velocity[0] = (p->velocity[0] * (p->mass - _p->mass) + (2 * _p->mass * _p->velocity[0])) / (p->mass + _p->mass);
      p->velocity[2] = (p->velocity[0] * (p->mass - _p->mass) + (2 * _p->mass * _p->velocity[0])) / (p->mass + _p->mass);

      _p->velocity[0] = (_p->velocity[0] * (_p->mass - p->mass) + (2 * p->mass * p->velocity[0])) / (_p->mass + p->mass);
      _p->velocity[2] = (_p->velocity[0] * (_p->mass - p->mass) + (2 * p->mass * p->velocity[0])) / (_p->mass + p->mass);

      p->pos[0] += p->velocity[0];
      p->pos[2] += p->velocity[2];

      _p->pos[0] += _p->velocity[0];
      _p->pos[2] += _p->velocity[2];

//      double vx = p->velocity[0];
//      double vy = p->velocity[1];
//      double vz = p->velocity[2];
//
//      p->velocity[0] = _p->velocity[0];
//      p->velocity[1] = _p->velocity[1];
//      p->velocity[2] = _p->velocity[2];
//
//      _p->velocity[0] = vx;
//      _p->velocity[1] = vy;
//      _p->velocity[2] = vz;
      break;
    }
  }

  if (p->pos[1] <= 0.0f) {
    p->pos[1] = 0.0f;
    p->velocity[1] = -p->velocity[1] * p->bounce * _clampedRand(1.0f-p->collision_chaos, 1.0f);
    //TODO: this is not real physics, makes surfaces look bumpy though
    //p->velocity[0] += 0.001 * myRandom();
    //p->velocity[2] += 0.001 * myRandom();
//    p->velocity[2] += 0.1 * p->bounce * _clampedRand(-collision_chaos, collision_chaos);
//    p->velocity[0] += 0.1 * p->bounce * _clampedRand(-collision_chaos, collision_chaos);
  }

  if (p->pos[0] >= 400.0f) {
    p->pos[0] = 400.0f;
    p->velocity[0] = -p->velocity[0] * p->bounce;
    //p->velocity[0] = friction * -p->velocity[0] * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
    p->velocity[2] += p->collision_chaos * myRandom();
  }

  if (p->pos[0] <= -400.0f) {
    p->pos[0] = -400.0f;
    p->velocity[0] = -p->velocity[0] * p->bounce;
    //p->velocity[0] = friction * -p->velocity[0] * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
    p->velocity[2] += p->collision_chaos * myRandom();
  }

  if (p->pos[2] >= 400.0f) {
    p->pos[2] = 400.0f;
    p->velocity[2] = -p->velocity[2] * p->bounce;
    //p->velocity[2] = friction * -p->velocity[2] * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
    p->velocity[0] += p->collision_chaos * myRandom();
  }

  if (p->pos[2] <= -400.0f) {
    p->pos[2] = -400.0f;
    p->velocity[2] = -p->velocity[2] * p->bounce;
    //p->velocity[2] = friction * -p->velocity[2] * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
    p->velocity[0] += p->collision_chaos * myRandom();
  }
}
