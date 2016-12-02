#include "gui_manager.h"

#include <stdlib.h>
#include <GL/glut.h>

#include "vector.h"
#include "gui_element.h"

struct gui_manager *gui_manager_new(void) {
  struct gui_manager *m = malloc(sizeof(*m));
  if (!m) return NULL;

  m->elements = vector_new(GUI_MANAGER_INIT_CAPACITY, 2);
  m->aspect_ratio = 0;

  return m;
}


void gui_manager_delete(struct gui_manager **m) {
  if (!*m) return;

  vector_delete(&(*m)->elements);

  *m = NULL;
}


void gui_manager_add_element(struct gui_manager *m, struct gui_element *e) {
  vector_add(m->elements, e);
}


struct gui_element *gui_manager_new_element(
  struct gui_manager *m,
  const char *str,
  int width, int height,
  void(*callback)(void)
) {
  struct gui_element *pe = NULL;
  if (m->elements->size > 0) pe = m->elements->elements[m->elements->size-1];

  int x = m->left, y = m->top;
  if (pe != NULL) {
    if (pe->str == NULL) {
      x = m->left;
      y = pe->y - (pe->height + 5);
    } else {
      x = pe->x + pe->width + 5;
      y = pe->y;
    }
  }

  struct gui_element *e = gui_element_new(x, y, width, height, str, callback);
  gui_manager_add_element(m, e);
  return e;
}


void gui_manager_draw(struct gui_manager *m) {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0.0f, m->w_width, 0.0f, m->w_height);
  //glOrtho(-10*m->aspect_ratio, 10*m->aspect_ratio, -10, 10, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  for (size_t i=0; i<m->elements->size; ++i) {
    if (m->elements->elements[i]) {
      gui_element_draw(m->elements->elements[i]);
    }
  }

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

int gui_manager_event_click(struct gui_manager *m, int x, int y, int state) {
  if (state != GLUT_DOWN) return 0;

  int ry = m->w_height - y;

  for (size_t i=0; i<m->elements->size; ++i) {
    struct gui_element *e = m->elements->elements[i];
    if (!e || e->callback == NULL) continue;

    if (gui_element_is_inside(e, x, ry)) {
      e->callback();
      return 1;
    }
  }
  return 0;
}


void gui_manager_set_dimensions(struct gui_manager *m, int width, int height, int top, int left) {
  m->w_width = width;
  m->w_height = height;
  m->top = top;
  m->left = left;
  m->aspect_ratio = (double)width / (double)height;
}
