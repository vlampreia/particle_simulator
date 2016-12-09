#include "emitter.h"

#include <stdlib.h>
#include <math.h>

#include "particle.h"
#include "particle_system.h"

#define DEG_TO_RAD 0.017453293

static void _init_particle(struct emitter *e, struct particle *p);

struct emitter *emitter_new(struct vector *particle_pool) {
  struct emitter *e = malloc(sizeof(*e));
  if (!e) return NULL;

  e->position = (struct vertex) {0.0f, 0.0f, 0.0f};

  e->pitch = 0.0f;
  e->yaw = 0.0f;

  e->horiz_angle = 0.0f;
  e->vert_angle = 0.0f;

  e->frequency = 0;
  e->last_fire_t = 0;

  e->force = 0.5f;

  e->base_particle = NULL;

  e->particle_pool = particle_pool;

  e->firing = 0;

  e->emission_count = 1;

  return e;
}

void emitter_delete(struct emitter **e) {
  if (*e == NULL) return;

  particle_delete(&(*e)->base_particle);

  free(*e);
}

void emitter_fire(struct emitter *e, int count) {
  for (size_t i=0; i<e->particle_pool->size && count > 0; ++i) {
    struct particle *p = (struct particle*) e->particle_pool->elements[i];
    if (!p->active) {
      _init_particle(e, p);
      p->active = 1;
      count--;
    }
  }
}

void emitter_step(struct emitter *e, double t) {
  //if (t - e->last_fire_t > e->frequency) {
  if (t - e->last_fire_t > e->frequency) {
    emitter_fire(e, e->emission_count);
    //emitter_fire(e, 55);
    e->last_fire_t = t;
  }
}


void emitter_set_particle_pool(struct emitter *e, struct vector *pool) {
  e->particle_pool = pool;
}

static long myRandom(long max)
{
  unsigned long
    num_bins = (unsigned long) max + 1,
    num_rand = (unsigned long) RAND_MAX + 1,
    bin_size = num_rand / num_bins,
    defect = num_rand % num_bins;

  long x;
  do {
    x = random();
  } while (num_rand - defect <= (unsigned long)x);

  return x/bin_size;
  //return rand()/(double)RAND_MAX;
//  if (max == RAND_MAX+1) {
//    return rand();
//  }
//
//  long limit = (RAND_MAX/max)*max;
//  int r;
//  while (1) {
//    r = rand();
//    if (r < limit) break;
//  }
//  
//  return r % max;
  //return rand()/(double)RAND_MAX;
  //return ((-RAND_MAX/2 + rand())/(double)RAND_MAX/2);
}

static void _init_particle(struct emitter *e, struct particle *p) {
  particle_copy(e->base_particle, p);
  particle_system_set_particle_pos(e->psystem, p, e->position);

  double pmod = e->pitch + (myRandom(RAND_MAX)/(double)RAND_MAX) * e->vert_angle;
  double ymod = e->yaw   + (myRandom(RAND_MAX)/(double)RAND_MAX) * e->horiz_angle;
  pmod *= DEG_TO_RAD;
  ymod *= DEG_TO_RAD;
//  double pmod = (e->pitch + (e->vert_angle  * myRandom())) * DEG_TO_RAD;
//  double ymod = (e->yaw   + (e->horiz_angle * myRandom())) * DEG_TO_RAD;

  p->velocity.x = -cos(pmod) * sin(ymod);
  p->velocity.y =  sin(pmod);
  p->velocity.z =  cos(pmod) * cos(ymod);

  vertex_normalise(&p->velocity);
  p->velocity.x *= e->force;
  p->velocity.y *= e->force;
  p->velocity.z *= e->force;

  p->base_color.r = e->base_particle->base_color.r;
  p->base_color.g = e->base_particle->base_color.g;
  p->base_color.b = e->base_particle->base_color.b;
  p->base_color.a = e->base_particle->base_color.a;

  particle_system_set_particle_col(e->psystem, p, p->base_color);
}
