#ifndef _VERTEX_H__
#define _VERTEX_H__

#include <GL/glut.h>

struct vertex {
  GLfloat x;
  GLfloat y;
  GLfloat z;
};

struct vertex_col {
  GLubyte r;
  GLubyte g;
  GLubyte b;
  GLubyte a;
};


void vertex_copy(struct vertex *s, struct vertex *t);

void vertex_normalise(struct vertex *v);

#endif
