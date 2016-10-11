#ifndef _PARTICLE_H__
#define _PARTICLE_H__

#include "vector3f.h"

struct particle {
  struct vector3f pos;

  struct vector3f acceleration;

  struct vector3f velocity;

  double mass;

  long tod_usec;

  double bounce;

  int active;

  struct vector3f color;
  double color_alpha;
};


struct particle *particle_new     ();
void             particle_delete  (struct particle **p);

void             particle_copy    (struct particle *s, struct particle *t);

#endif
