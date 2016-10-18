#include "emitter.h"

#include <stdlib.h>
#include <math.h>

#include "particle.h"
#include "vector3f.h"

#define DEG_TO_RAD 0.017453293

static void _init_particle(struct emitter *e, struct particle *p);


struct emitter *emitter_new(struct vector *particle_pool) {
  struct emitter *e = malloc(sizeof(*e));
  if (!e) return NULL;

  e->position = (struct vector3f) {0.0f, 0.0f, 0.0f};

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

  return e;
}

void emitter_delete(struct emitter **e) {
  if (*e == NULL) return;

  particle_delete(&(*e)->base_particle);

  free(*e);
}

void emitter_fire(struct emitter *e) {
  for (size_t i=0; i<e->particle_pool->size; ++i) {
    struct particle *p = (struct particle*) e->particle_pool->elements[i];
    if (!p->active) {
      _init_particle(e, p);
      p->active = 1;
      break;
    }
  }
}

void emitter_step(struct emitter *e, int t) {
  if (t - e->last_fire_t > e->frequency) {
    emitter_fire(e);
    e->last_fire_t = t;
  }
}


void emitter_set_particle_pool(struct emitter *e, struct vector *pool) {
  e->particle_pool = pool;
}

static double myRandom(void)
{
  return ((-RAND_MAX/2 + rand())/(double)RAND_MAX/2);
}

static void _init_particle(struct emitter *e, struct particle *p) {
  particle_copy(e->base_particle, p);
  p->pos[0] = e->position.x;
  p->pos[1] = e->position.y;
  p->pos[2] = e->position.z;

  double pmod = e->vert_angle * myRandom();
  double ymod = e->horiz_angle * myRandom();

  p->velocity[0] = -cos((e->pitch + pmod) * DEG_TO_RAD) * sin((e->yaw + ymod) * DEG_TO_RAD);
  p->velocity[1] =  sin((e->pitch + pmod) * DEG_TO_RAD);
  p->velocity[2] =  cos((e->pitch + pmod) * DEG_TO_RAD) * cos((e->yaw + ymod) * DEG_TO_RAD);
  GLfloat_normalise(p->velocity);
  p->velocity[0] *= e->force;
  p->velocity[1] *= e->force;
  p->velocity[2] *= e->force;
}
