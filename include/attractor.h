#ifndef _ATTRACTOR_H__
#define _ATTRACTOR_H__

#include "vertex.h"

struct attractor {
  struct vertex pos;
  long mass;
  int enabled;
};

struct attractor *attractor_new(struct vertex pos, long mass);
void attractor_delete(struct attractor **a);

#endif
