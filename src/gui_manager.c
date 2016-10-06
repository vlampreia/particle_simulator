#include "gui_manager.h"

#include <stdlib.h>
#include <GL/glut.h>

#include "gui_element.h"

struct gui_manager *gui_manager_new(void) {
  struct gui_manager *m = malloc(sizeof(*m));
  if (!m) return NULL;

  m->capacity = GUI_MANAGER_INIT_CAPACITY;
  m->n_elements = 0;

  m->elements = malloc(sizeof(*m->elements) * m->capacity);
  if (!m) {
    gui_manager_delete(&m);
    return NULL;
  }

  for (int i=0; i<m->capacity; ++i) {
    m->elements[i] = NULL;
  }

  return m;
}


void gui_manager_delete(struct gui_manager **m) {
  if (!*m) return;

  if ((*m)->elements) free((*m)->elements);

  *m = NULL;
}


void gui_manager_add_element(struct gui_manager *m, struct gui_element *e) {
  if (m->n_elements >= m->capacity) return;

  m->elements[m->n_elements++] = e;
}

void gui_manager_draw(struct gui_manager *m) {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0.0f, m->w_width, 0.0f, m->w_height);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  for (int i=0; i<m->capacity; ++i) {
    if (m->elements[i]) {
      gui_element_draw(m->elements[i]);
    }
  }

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void gui_manager_event_click(struct gui_manager *m, int x, int y, int state) {
  if (state != GLUT_DOWN) return;

  int ry = m->w_height - y;

  for (int i=0; i<GUI_MANAGER_INIT_CAPACITY; ++i) {
    struct gui_element *e = m->elements[i];
    if (!e) continue;

    if (gui_element_is_inside(e, x, ry)) {
      e->callback();

      break;
    }
  }
}


void gui_manager_set_dimensions(struct gui_manager *m, int width, int height) {
  m->w_width = width;
  m->w_height = height;
}
