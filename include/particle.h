#ifndef _PARTICLE_H__
#define _PARTICLE_H__

#include "vector3f.h"

struct particle {
  GLfloat pos[3];
  GLfloat acceleration[3];
  GLfloat velocity[3];

  double mass;
  double collision_chaos;

  long tod_usec;
  long tod_max;

  double bounce;

  int active;

  GLubyte color[4];

  size_t pos_idx;

  float size;
};


struct particle *particle_new     (void);
void             particle_delete  (struct particle **p);

void             particle_copy    (struct particle *s, struct particle *t);

#endif
