#include "particle_system.h"

#include <math.h>
#include <stdlib.h>

#include "vector.h"
#include "vector3f.h"
#include "particle.h"
#include "emitter.h"


static void _update_emitters(struct vector *elist, double t, double dt);
static void _update_particles(struct particle_system *s, struct vector *plist, double t, double dt);
static inline void _update_particle_pos(struct particle_system *s, struct particle *p, double t, double dt);
static inline void _update_particle_collision(struct particle_system *s, struct particle *p, double t, double dt);
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

#define NSIGN(x) ((x > 0) - (x < 0))


struct particle_system *particle_system_new(size_t numParticles) {
  struct particle_system *s = malloc(sizeof(*s));
  if (!s) return NULL;

  s->particles = vector_new(numParticles, 0);
  s->particle_pos = malloc(sizeof(struct vertex) * numParticles);
  s->particle_col = malloc(sizeof(struct vertex) * numParticles);
  s->particle_idx = malloc(sizeof(GLubyte) * numParticles);
  s->emitters = vector_new(10, 0);

  for (size_t i=0; i<numParticles; ++i) {
    struct particle *p = particle_new();
    p->pos_idx = i;
    vector_add(s->particles, p);
    s->particle_col[i] = (struct vertex){1.0,0.0,0.0};
    s->particle_idx[i] = i;
  }

  s->gravity = 9.807f;
  s->friction = 0.98f;
  s->air_density = 1.1f;

  s->collideFloor = 1;
  s->collideWalls = 1;

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


void particle_system_step(struct particle_system *s, double t, double dt) {
  _update_emitters(s->emitters, t, dt);
  _update_particles(s, s->particles, t, dt);
}


static void _update_emitters(struct vector *elist, double t, double dt) {
  for (size_t i=0; i<elist->size; ++i) {
    if (!elist->elements[i]) continue;
    if (!((struct emitter*)elist->elements[i])->firing) continue;
    emitter_step(elist->elements[i], t);
  }
}

static void _update_particles(struct particle_system *s, struct vector *plist, double t, double dt) {
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

static inline void _update_particle_pos(
  struct particle_system *s,
  struct particle *p,
  double t,
  double dt
) {
  double force = -s->gravity * p->mass;// * 1/dt;
  //double friction = 0.990f;

  double airforce = s->air_density;

  struct vector3f wind_force = (struct vector3f) {
    0.003 * _clampedRand(0.5,1.0) / p->mass,
    0.0,
    0.001 * _clampedRand(0.5,1.0) / p->mass
  };

  if (p->pos[1] <= 0.5) {
    wind_force.x /= 1.2;
    wind_force.y /= 1.2;
  } else {
    wind_force.x = p->pos[1] * wind_force.x;
    wind_force.z = p->pos[1] * wind_force.z;
  }

  //force is static down 1 for now
  p->acceleration[0] = wind_force.x;
  p->acceleration[1] = force;/// / p->mass;
  //p->acceleration[1] -= airforce * p->velocity[1];
  p->acceleration[2] = wind_force.z;

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

  p->velocity[0] /= airforce;
  p->velocity[1] /= airforce;
  p->velocity[2] /= airforce;

  //vector3f_normalise(p->velocity);

  p->pos[0] += p->velocity[0] * dt;
  p->pos[1] += p->velocity[1] * dt;
  p->pos[2] += p->velocity[2] * dt;

  struct vertex *v = &s->particle_pos[p->pos_idx];
  v->x = p->pos[0];
  v->y = p->pos[1];
  v->z = p->pos[2];
}

static inline void _update_particle_collision(struct particle_system *s, struct particle *p, double t, double dt) {
  //  particle-particle collision code is suboptimal..
//  for (size_t i=0; i<s->particles->size; ++i) {
//    struct particle *_p = s->particles->elements[i];
//    if (_p == p || !_p->active) continue;
//
//    double dx = p->pos[0] - _p->pos[0];
//    double dy = p->pos[1] - _p->pos[1];
//    double dz = p->pos[2] - _p->pos[2];

    //assume radius of 5
//    if (dx*dx + dy*dy + dz*dz < 100) {
//      p->velocity[0] = (p->velocity[0] * (p->mass - _p->mass) + (2 * _p->mass * _p->velocity[0])) / (p->mass + _p->mass);
//      p->velocity[2] = (p->velocity[0] * (p->mass - _p->mass) + (2 * _p->mass * _p->velocity[0])) / (p->mass + _p->mass);
//
//      _p->velocity[0] = (_p->velocity[0] * (_p->mass - p->mass) + (2 * p->mass * p->velocity[0])) / (_p->mass + p->mass);
//      _p->velocity[2] = (_p->velocity[0] * (_p->mass - p->mass) + (2 * p->mass * p->velocity[0])) / (_p->mass + p->mass);
//
//      p->pos[0] += p->velocity[0];
//      p->pos[2] += p->velocity[2];
//
//      _p->pos[0] += _p->velocity[0];
//      _p->pos[2] += _p->velocity[2];
//
////      double vx = p->velocity[0];
////      double vy = p->velocity[1];
////      double vz = p->velocity[2];
////
////      p->velocity[0] = _p->velocity[0];
////      p->velocity[1] = _p->velocity[1];
////      p->velocity[2] = _p->velocity[2];
////
////      _p->velocity[0] = vx;
////      _p->velocity[1] = vy;
////      _p->velocity[2] = vz;
//      break;
//    }
//  }

  if (s->collideFloor) {
    if (p->pos[1] <= 0.05f) {
      p->velocity[0] *= s->friction*p->bounce;
      p->velocity[2] *= s->friction*p->bounce;

      if (p->pos[1] < 0.0f) {
        p->pos[1] = 0.0f;
        p->velocity[1] = (-p->velocity[1] * p->bounce);// * _clampedRand(1.0f-p->collision_chaos, 1.0f) * dt;
        
  //      p->velocity[0] += 1/ (p->bounce * p->velocity[1]);
  //      p->velocity[2] += 1/ (p->bounce * p->velocity[1]);
  //      p->velocity[0] *= NSIGN(p->velocity[0]) * 0.5*(p->velocity[1] * p->bounce);
  //      p->velocity[2] *= NSIGN(p->velocity[2]) * 0.5*(p->velocity[1] * p->bounce);
        //TODO: this is not real physics, makes surfaces look bumpy though
        //p->velocity[0] += 0.001 * myRandom();
        //p->velocity[2] += 0.001 * myRandom();
  //      p->velocity[2] += 0.1 * p->bounce * _clampedRand(-p->collision_chaos, p->collision_chaos);
  //      p->velocity[0] += 0.1 * p->bounce * _clampedRand(-p->collision_chaos, p->collision_chaos);
      }
    }
  }

  if (s->collideWalls) {
  //TODO: expensive....
    if (p->pos[0] >= 400.0f) {
      p->pos[0] = 400.0f;
      //p->velocity[0] = -p->velocity[0] * p->bounce * dt;
      p->velocity[0] = s->friction * -p->velocity[0] * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
      p->velocity[2] += p->collision_chaos * myRandom() * dt;
    }

    if (p->pos[0] <= -400.0f) {
      p->pos[0] = -400.0f;
      //p->velocity[0] = -p->velocity[0] * p->bounce * dt;
      p->velocity[0] = s->friction * -p->velocity[0] * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
      p->velocity[2] += p->collision_chaos * myRandom() * dt;
    }

    if (p->pos[2] >= 400.0f) {
      p->pos[2] = 400.0f;
      //p->velocity[2] = -p->velocity[2] * p->bounce * dt;
      p->velocity[2] = s->friction * -p->velocity[2] * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
      p->velocity[0] += p->collision_chaos * myRandom() * dt;
    }

    if (p->pos[2] <= -400.0f) {
      p->pos[2] = -400.0f;
      p->velocity[2] = -p->velocity[2] * p->bounce;
      //p->velocity[2] = friction * -p->velocity[2] * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
      p->velocity[0] += p->collision_chaos * myRandom();
    }
  }
}
