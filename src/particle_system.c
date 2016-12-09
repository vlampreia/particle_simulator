#include "particle_system.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "vector.h"
#include "vertex.h"
#include "particle.h"
#include "emitter.h"


static void _update_emitters(struct vector *elist, double t, double dt);
static int _update_particles(struct particle_system *s, struct vector *plist, double t, double dt);
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
  s->particle_col = malloc(sizeof(struct vertex_col) * numParticles);
  s->particle_idx = malloc(sizeof(GLubyte) * numParticles);
  s->emitters = vector_new(10, 0);

  for (size_t i=0; i<numParticles; ++i) {
    struct particle *p = particle_new(NULL, i);
    vector_add(s->particles, p);
    s->particle_pos[i] = (struct vertex) {0.0f, 0.0f, 0.0f};
    s->particle_col[i] = (struct vertex_col){0,0,0,0};
    s->particle_idx[i] = i;
  }

  s->gravity = 9.807f;
  s->friction = 0.98f;
  s->air_density = 1.0f;

  s->collideFloor = 0;
  s->collideWalls = 0;
  s->isCollisionEnabled = 1;

  s->num_attractors = 3;
  s->attractors = malloc(sizeof(*s->attractors) * 4 * s->num_attractors);

  s->trip = 0;

  _configure_attractor(s->attractors, 0, -1000, -5000, 800 ,    0);//-2000000000.5);
  _configure_attractor(s->attractors, 1, 1, 2150, 4090,         5500000000.3);
  _configure_attractor(s->attractors, 2, 3000, 7000, 0,        0);//-3000000000.0);

//  _configure_attractor(s->attractors, 2, 2000000, 50000, 0, -0.3);
  //_configure_attractor(s->attractors, 1, 0, 0, 0, -2);
  //_configure_attractor(s->attractors, 1, 0, 0, 0, 10);
//  _configure_attractor(s->attractors, 1, -10000, 0, 500000, 0.5);
//_configure_attractor(s->attractors, 0, 50000, -50000, -50000, 8.52);
  //_configure_attractor(s->attractors, 2, -5000, 500, 8000, -0.4);
//  _configure_attractor(s->attractors, 2, 50000, 0, 5000, -1);

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
  e->psystem = s;
}


int particle_system_step(struct particle_system *s, double t, double dt) {
  _update_emitters(s->emitters, t, dt);
  return _update_particles(s, s->particles, t, dt);
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

static int _update_particles(
  struct particle_system *s,
  struct vector *plist,
  double t,
  double dt
) {
  int active = 0;

  for (size_t i=0; i<plist->size; ++i) {
    struct particle *p = plist->elements[i];
    if (!p->active) continue;
    if (p->tod_usec <= 0) {
      p->active = 0;
      continue;
    }
    p->tod_usec -= dt;

    _update_particle_pos(s, plist->elements[i], t, dt);
    ++active;
  }

  if (s->isCollisionEnabled) {
    for (size_t i=0; i<plist->size; ++i) {
      struct particle *p = plist->elements[i];
      if (!p->active) continue;

      _update_particle_collision(s, plist->elements[i], t, dt);
    }
  }

  return active;
}

//god bless gg
static inline float fisqrt(float number) {
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;                       // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck? 
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

static inline void _update_particle_pos(
  struct particle_system *s,
  struct particle *p,
  double t,
  double dt
) {
  double force = -s->gravity;
  const size_t pidx = p->pos_idx;
  struct vertex *ppos = &s->particle_pos[pidx];
  struct vertex_col *pcol = &s->particle_col[pidx];

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
  double e2 = EULER_CONST * EULER_CONST;
  //static const double G = 0.0000000000667408;
  static const double G = 0.003;
  static const double factor=3.0f/2.0f;

  struct vertex na = {0,0,0};

  for (size_t i=0; i<s->num_attractors; ++i) {
    size_t idx = i*4;

    double mass = s->attractors[idx + 3];

    struct vertex dv = (struct vertex) {
      s->attractors[idx + 0] - ppos->x,
      s->attractors[idx + 1] - ppos->y,
      s->attractors[idx + 2] - ppos->z
    };

    double magnitude = sqrt(dv.x*dv.x + dv.y*dv.y + dv.z*dv.z);

    //magnitude = fisqrt(magnitude);//1.0f / sqrt(magnitude);
    //650mspf @ 200k
    //550mspf @ 200k

    na.x += ((mass * dv.x)/pow((magnitude*magnitude + e2), factor));
    na.y += ((mass * dv.y)/pow((magnitude*magnitude + e2), factor));
    na.z += ((mass * dv.z)/pow((magnitude*magnitude + e2), factor));
  }

  na.x *= G;
  na.y *= G;
  na.z *= G;
  //vector3f_normalise(p->velocity);

  s->particle_pos[pidx].x += p->velocity.x * dt + 1.0f/2.0f * p->acceleration.x * dt*dt;
  s->particle_pos[pidx].y += p->velocity.y * dt + 1.0f/2.0f * p->acceleration.y * dt*dt;
  s->particle_pos[pidx].z += p->velocity.z * dt + 1.0f/2.0f * p->acceleration.z * dt*dt;

  p->velocity.x += 1.0f/2.0f * (na.x + p->acceleration.x) * dt;
  p->velocity.y += 1.0f/2.0f * (na.y + p->acceleration.y) * dt;
  p->velocity.z += 1.0f/2.0f * (na.z + p->acceleration.z) * dt;

  p->acceleration.x = na.x;
  p->acceleration.y = na.y;
  p->acceleration.z = na.z;

  //double e = sqrt(p->velocity[0]*p->velocity[0] + p->velocity[1]*p->velocity[1] + p->velocity[2]*p->velocity[2]);

  double v = (p->tod_usec/(double)p->tod_max)*p->base_color.a;
  double vd = v/(double)255;
  pcol->a = v;
//  p->color[0] = p->base_color[0] * vd;
//  p->color[1] = p->base_color[1] * vd;
//  p->color[2] = p->base_color[2] * vd;
  //double m = sqrt(p->velocity[0]*p->velocity[0] + p->velocity[1]*p->velocity[1] + p->velocity[2]*p->velocity[2]);

  //p->color[0] -= (v/255)/p->tod_max;//_mind(p->color[0], p->color[0]-v);
  //p->color[1] -= (v/255)/p->tod_max;//_mind(p->color[1], p->color[1]-v);
  //p->color[2] -= (v/255)/p->tod_max;//_mind(p->color[2], p->color[2]-v);

  if (s->trip) {
    double mg = sqrt(
      p->velocity.x * p->velocity.x +
      p->velocity.y * p->velocity.y +
      p->velocity.z * p->velocity.z
    );
    //double hue = (p->velocity.x/mg + p->velocity.y/mg + p->velocity.z/mg) * 360.0f;
    double hue = fmod(abs(p->velocity.x+p->velocity.y+p->velocity.z)/2, 360);
    double hp = hue/60.0f;
    double x = 255.0f / (1 - abs(fmod(hp,2.0f)-1.0f));

    if (hp <= 1) {
      pcol->r = 255;
      pcol->g = x;
      pcol->b = 0;
    } else if (hp <= 2) {
      pcol->r = x;
      pcol->g = 255;
      pcol->b = 0;
    } else if (hp <= 3) {
      pcol->r = 0;
      pcol->g = 255;
      pcol->b = x;
    } else if (hp <= 4) {
      pcol->r = 0;
      pcol->g = x;
      pcol->b = 255;
    } else if (hp <= 5) {
      pcol->r = x;
      pcol->g = 0;
      pcol->b = 255;
    } else if (hp <= 6) {
      pcol->r = 255;
      pcol->g = 0;
      pcol->b = x;
    }
//    p->color[0] += p->velocity[0]/255;
//    p->color[1] += p->velocity[1]/255;
//    p->color[2] += p->velocity[2]/255;
  }
}

static inline void _update_particle_collision(
  struct particle_system *s,
  struct particle *p,
  double t,
  double dt
) {
  struct vertex *ppos = &s->particle_pos[p->pos_idx];
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
    if (ppos->y <= -10000.05f) {
      p->velocity.x *= s->friction*p->bounce;
      p->velocity.z *= s->friction*p->bounce;

      if (ppos->y < -10000.0f) {
        ppos->y = -10000.0f;
        p->velocity.y = (-p->velocity.y * p->bounce);// * _clampedRand(1.0f-p->collision_chaos, 1.0f) * dt;
        
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
    if (ppos->x >= 400.0f) {
      ppos->x = 400.0f;
      //p->velocity[0] = -p->velocity[0] * p->bounce * dt;
      p->velocity.x = s->friction * -p->velocity.x * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
      p->velocity.z += p->collision_chaos * myRandom() * dt;
    }

    if (ppos->x <= -400.0f) {
      ppos->x = -400.0f;
      //p->velocity[0] = -p->velocity[0] * p->bounce * dt;
      p->velocity.x = s->friction * -p->velocity.x * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
      p->velocity.z += p->collision_chaos * myRandom() * dt;
    }

    if (ppos->z >= 400.0f) {
      ppos->z = 400.0f;
      //p->velocity[2] = -p->velocity[2] * p->bounce * dt;
      p->velocity.z = s->friction * -p->velocity.z * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
      p->velocity.x += p->collision_chaos * myRandom() * dt;
    }

    if (ppos->z <= -400.0f) {
      ppos->z = -400.0f;
      p->velocity.z = -p->velocity.z * p->bounce;
      //p->velocity[2] = friction * -p->velocity[2] * p->bounce * _clampedRand(1.0f-p->collision_chaos, p->collision_chaos);
      p->velocity.x += p->collision_chaos * myRandom();
    }
  }
}


void particle_system_set_particle_pos(
  struct particle_system *s,
  struct particle *p,
  struct vertex pos
) {
  s->particle_pos[p->pos_idx].x = pos.x;
  s->particle_pos[p->pos_idx].y = pos.y;
  s->particle_pos[p->pos_idx].z = pos.z;
}

void particle_system_set_particle_col(
  struct particle_system *s,
  struct particle *p,
  struct vertex_col col
) {
  s->particle_col[p->pos_idx].r = col.r;
  s->particle_col[p->pos_idx].g = col.g;
  s->particle_col[p->pos_idx].b = col.b;
  s->particle_col[p->pos_idx].a = col.a;
}

void particle_system_reset(struct particle_system *s) {
  for (size_t i=0; i<s->emitters->size; ++i) {
    ((struct emitter*)s->emitters->elements[i])->firing = 0;
  }

  for (size_t i=0; i<s->particles->size; ++i) {
    struct particle *p = s->particles->elements[i];
    p->active = 0;
    s->particle_col[p->pos_idx].a = 0;
  }
}
