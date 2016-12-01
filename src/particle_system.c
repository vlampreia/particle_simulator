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
static double _mind(double a, double b) {
  if (a > b) return b;
  return a;
}

#define NSIGN(x) ((x > 0) - (x < 0))

static void _configure_attractor(double *a, size_t i, double x, double y, double z, double g) {
  size_t idx = i*4;
  a[idx] = x;
  a[idx + 1] = y;
  a[idx + 2] = z;
  a[idx + 3] = g;
}

struct particle_system *particle_system_new(size_t numParticles) {
  struct particle_system *s = malloc(sizeof(*s));
  if (!s) return NULL;

  s->particles = vector_new(numParticles, 0);
  s->particle_pos = malloc(sizeof(struct vertex) * numParticles);
  s->particle_col = malloc(sizeof(struct vertexb) * numParticles);
  s->particle_idx = malloc(sizeof(GLubyte) * numParticles);
  s->emitters = vector_new(10, 0);

  for (size_t i=0; i<numParticles; ++i) {
    struct particle *p = particle_new();
    p->pos_idx = i;
    vector_add(s->particles, p);
    s->particle_col[i] = (struct vertexb){255,0,0,255};
    s->particle_idx[i] = i;
  }

  s->gravity = 9.807f;
  s->friction = 0.98f;
  s->air_density = 1.0f;

  s->collideFloor = 0;
  s->collideWalls = 0;

  s->num_attractors = 4;
  s->attractors = malloc(sizeof(*s->attractors) * 4 * s->num_attractors);

  _configure_attractor(s->attractors, 0, 0, 0, 50000, -4.0);
  _configure_attractor(s->attractors, 1, 50000, -50000, -500, 2.2);
  _configure_attractor(s->attractors, 2, -5000, 50000, 8000, 4.1);
  _configure_attractor(s->attractors, 3, 5000000, 0, 500000, -0.01);

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
//    ((struct emitter*)elist->elements[i])->pitch += 10.0f;
//    ((struct emitter*)elist->elements[i])->yaw += 10.0f;
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

static inline void _update_particle_pos(
  struct particle_system *s,
  struct particle *p,
  double t,
  double dt
) {
  static int trip = 0;
  double force = 0;

//  double airforce = 1;//0.985;
//
//  struct vector3f wind_force = (struct vector3f) {
//    0.0003 * _clampedRand(0.5,1.0) / p->mass,
//    0.5,
//    0.002 * _clampedRand(0.5,1.0) / p->mass
//  };

//  if (p->pos[1] <= 0.5) {
//    wind_force.x /= 1.2;
//    wind_force.y /= 1.2;
//  } else {
//    wind_force.x = p->pos[1]/10 * wind_force.x;
//    //wind_force.y = p->pos[1]/wind_force.x*wind_force.z;
//    wind_force.z = p->pos[1]/10 * wind_force.z;
  //}
  //
  //if (trip) {
//    wind_force.x = 0;
//    wind_force.y = 0;
//    wind_force.z = 0;
  //}

  //force is static down 1 for now
//  p->acceleration[0] = wind_force.x;
//  p->acceleration[1] = wind_force.y+ force;/// / p->mass;
//  //p->acceleration[1] -= airforce * p->velocity[1];
//  p->acceleration[2] = wind_force.z;

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

//  p->velocity[0] += p->acceleration[0];
//  p->velocity[1] += p->acceleration[1];
//  p->velocity[2] += p->acceleration[2];


//  p->velocity[0] /= airforce;
//  p->velocity[1] /= airforce;
//  p->velocity[2] /= airforce;

//  p->velocity[0] += 0-p->pos[0] * 0.000199;
//  p->velocity[1] += 0-p->pos[1] * 0.000199;
//  p->velocity[2] += 0-p->pos[2] * 0.000199;

  static const double EULER_CONST = 2.71828182845904523536;
  static const double G = 0.0005;
  static const double factor=3/2;

  for (size_t i=0; i<s->num_attractors; ++i) {
    size_t idx = i*4;

    double mass = s->attractors[idx + 3];

    double dv[3] = {
      s->attractors[idx + 0] - p->pos[0],
      s->attractors[idx + 1] - p->pos[1],
      s->attractors[idx + 2] - p->pos[2]
    };

    double magnitude = sqrt(dv[0]*dv[0] + dv[1]*dv[1] + dv[2]+dv[2]);

    double dvn[3] = {
      dv[0] / magnitude,
      dv[1] / magnitude,
      dv[2] / magnitude
    };

    double e2 = EULER_CONST * EULER_CONST;

    p->velocity[0] += G * ((mass * dv[0])/pow((dvn[0]*dvn[0] + e2), factor));
    p->velocity[1] += G * ((mass * dv[1])/pow((dvn[1]*dvn[1] + e2), factor));
    p->velocity[2] += G * ((mass * dv[2])/pow((dvn[2]*dvn[2] + e2), factor));
  }

  //vector3f_normalise(p->velocity);

  p->pos[0] += p->velocity[0];// * dt;
  p->pos[1] += p->velocity[1];// * dt;
  p->pos[2] += p->velocity[2];// * dt;

  //double e = sqrt(p->velocity[0]*p->velocity[0] + p->velocity[1]*p->velocity[1] + p->velocity[2]*p->velocity[2]);

  double v = (p->tod_usec/(double)p->tod_max)*p->base_color[3];
  double vd = v/(double)255;
  p->color[3] = v;
//  p->color[0] = p->base_color[0] * vd;
//  p->color[1] = p->base_color[1] * vd;
//  p->color[2] = p->base_color[2] * vd;
  //double m = sqrt(p->velocity[0]*p->velocity[0] + p->velocity[1]*p->velocity[1] + p->velocity[2]*p->velocity[2]);

  //p->color[0] -= (v/255)/p->tod_max;//_mind(p->color[0], p->color[0]-v);
  //p->color[1] -= (v/255)/p->tod_max;//_mind(p->color[1], p->color[1]-v);
  //p->color[2] -= (v/255)/p->tod_max;//_mind(p->color[2], p->color[2]-v);

  if (trip) {
    p->color[0] += p->velocity[0]/255;
    p->color[1] += p->velocity[1]/255;
    p->color[2] += p->velocity[2]/255;
  }


  struct vertex *_v = &s->particle_pos[p->pos_idx];
  _v->x = p->pos[0];
  _v->y = p->pos[1];
  _v->z = p->pos[2];

  struct vertexb *_vb = &s->particle_col[p->pos_idx];
  _vb->x = p->color[0];
  _vb->y = p->color[1];
  _vb->z = p->color[2];
  _vb->a = p->color[3];
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
