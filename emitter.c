#include "emitter.h"

#include <stdlib.h>

#include "particle.h"
#include "vector3f.h"


static void _init_particle(struct emitter *e, struct particle *p);


struct emitter *emitter_new(struct particle **particle_pool) {
  struct emitter *e = malloc(sizeof(*e));
  if (!e) return NULL;

  e->position = (struct vector3f) {0.0f, 0.0f, 0.0f};
  e->orientation = (struct vector3f) {0.0f, 0.0f, 0.0f};
  vector3f_normalise(&e->orientation);

  e->horiz_angle = 0.0f;
  e->vert_angle = 0.0f;

  e->frequency = 0;
  e->last_fire_t = 0;

  e->force = 0.5f;

  e->base_particle = NULL;

  e->particle_pool = particle_pool;

  return e;
}

void emitter_delete(struct emitter **e) {
  if (*e == NULL) return;

  particle_delete(&(*e)->base_particle);

  free(*e);
}

void emitter_fire(struct emitter *e) {
  for (int i=0; i<100; ++i) {
    if (!e->particle_pool[i]->active) {
      _init_particle(e, e->particle_pool[i]);
      e->particle_pool[i]->active = 1;
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


static void _init_particle(struct emitter *e, struct particle *p) {
  particle_copy(e->base_particle, p);
  vector3f_copy(&e->position, &p->pos);

  p->velocity.x = e->force * e->orientation.x;
  p->velocity.y = e->force * e->orientation.y;
  p->velocity.z = e->force * e->orientation.z;
}
