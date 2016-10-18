#ifndef _PARTICLE_H__
#define _PARTICLE_H__

#include "vector3f.h"

struct particle {
  //struct vector3f pos;
  GLfloat pos[3];

  //struct vector3f acceleration;
  GLfloat acceleration[3];

  //struct vector3f velocity;
  GLfloat velocity[3];

  double mass;
  double collision_chaos;

  long tod_usec;

  double bounce;

  int active;

  GLubyte color[4];
  //struct vector3f color;
  //double color_alpha;
};


struct particle *particle_new     ();
void             particle_delete  (struct particle **p);

void             particle_copy    (struct particle *s, struct particle *t);

#endif
