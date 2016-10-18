#include "vector3f.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

struct vector3f *vector3f_new(double x, double y, double z) {
  struct vector3f *v = malloc(sizeof(*v));
  if (!v) return NULL;

  v->x = x;
  v->y = y;
  v->z = z;

  return v;
}


void vector3f_delete(struct vector3f **v) {
  if (!*v) return;

  free(*v);
}

void vector3f_init(struct vector3f *v) {
  v->x = 0.0f;
  v->y = 0.0f;
  v->z = 0.0f;
}

void vector3f_normalise(struct vector3f *v) {
  double norm = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);

  v->x = v->x / norm;
  v->y = v->y / norm;
  v->z = v->z / norm;
}

void GLfloat_normalise(GLfloat *v) {
  float norm = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
  v[0] /= norm;
  v[1] /= norm;
  v[2] /= norm;
}

void GLfloat_copy(GLfloat *s, GLfloat *t) {
  t[0] = s[0];
  t[1] = s[1];
  t[2] = s[2];
}

void vector3f_copy(struct vector3f *s, struct vector3f *t) {
  t->x = s->x;
  t->y = s->y;
  t->z = s->z;
}

char *vector3f_to_str(struct vector3f *v) {
  char *str = malloc(64);

  sprintf(str, "(%f, %f, %f)", v->x, v->y, v->z);

  return str;
}
