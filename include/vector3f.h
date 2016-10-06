#ifndef _VECTOR3F_H__
#define _VECTOR3F_H__


struct vector3f {
  double x, y, z;
};

struct vector3f *vector3f_new     (double x, double y, double z);
void             vector3f_delete  (struct vector3f **v);

void             vector3f_init    (struct vector3f *v);

void             vector3f_normalise (struct vector3f *v);

void             vector3f_copy    (struct vector3f *s, struct vector3f *t);

char            *vector3f_to_str  (struct vector3f *p);

#endif
