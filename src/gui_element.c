#include "gui_element.h"

#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

#define GUI_ELEMENT_CHAR_WIDTH 9

static void _compile(struct gui_element *e);

struct gui_element *gui_element_new(
  int x, int y,
  int width, int height,
  const char *str,
  void(*callback)(void)
) {
  struct gui_element *e = malloc(sizeof(*e));
  if (!e) return NULL;

  e->x = x;
  e->y = y;
  if (width <= 0 && str != NULL) e->width = 10 + GUI_ELEMENT_CHAR_WIDTH * strlen(str);
  else e->width = width;

  if (height <= 0) e->height = 20;
  else e->height = height;

  if (str != NULL) {
    e->str = NULL;
    e->str = malloc(256);
    if (str != NULL) {
      //e->str = malloc(sizeof(str));
      strcpy(e->str, str);
      e->str[255] = '\0';
    }
  }

  e->callback = callback;

  e->visible = 1;

  _compile(e);

  return e;
}

void gui_element_delete(struct gui_element **e) {
  if (!*e) return;

  if ((*e)->str) free((*e)->str);

  *e = NULL;
}

void gui_element_set_position(struct gui_element *e, int x, int y) {
  e->x = x;
  e->y = y;
  _compile(e);
}

void gui_element_set_dimensions(struct gui_element *e, int width, int height) {
  e->width = width;
  e->height = height;
  _compile(e);
}

void gui_element_set_visible(struct gui_element *e, int visible) {
  e->visible = visible;
}

void gui_element_set_str(struct gui_element *e, const char *str, int resize) {
  //char *nstr = realloc(e->str, len+1);
  //if (!nstr) return;
  //e->str = nstr;
  strcpy(e->str, str);
  e->str[255] = '\0';

  if (resize) {
    e->width = 10 + GUI_ELEMENT_CHAR_WIDTH * strlen(str);
    _compile(e);
  }
}

void gui_element_set_callback(struct gui_element *e, void(*callback)(void)) {
  e->callback = callback;
}

int gui_element_is_inside(struct gui_element *e, int top, int x, int y) {
  if (!e->visible) return 0;
  return (
    x >= e->x && x <= e->x + e->width &&
    y >= top+e->y && y <= top+e->y + e->height
  );
}

void gui_element_draw(struct gui_element *e, int top) {
  if (!e->str) return;
  if (!e->visible) return;

  if (e->callback == NULL) glColor4f(0.7f, 0.7f, 0.7f, 0.3f);
  else glColor4f(0.4f, 0.55f, 0.9f, 0.4f);

  glPushMatrix();
  static const int offset = 8;
  glTranslatef(e->x + offset, top + e->y + 6, 0);
  glCallList(e->compiled_list);

  //glScalef(1.0f/(152.38f), 1.0f/152.38f, 1.0f/152.38f);
  glScalef(1.0f/15.0f,1.0f/15.0f,1.0f/15.0f);

  int i = 0;
  //int x_offset = 5;
  glColor4ub(255, 255, 255, 255);
  //glRasterPos2i(e->x + x_offset, 5 + e->y);
  for (char *c = e->str; *c && i < e->width - offset; c++) {
    glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *c);
    i += offset;
  }
  glPopMatrix();
}

static void _compile(struct gui_element *e) {
  e->compiled_list = glGenLists(1);

  int offset = -7;
  glNewList(e->compiled_list, GL_COMPILE);
    glBegin(GL_QUADS);
      glVertex2f(offset,             offset);
      glVertex2f(e->width+offset,  offset);
      glVertex2f(e->width+offset,  e->height+offset);
      glVertex2f(offset,         e->height+offset);
      //glVertex2f(e->x,             e->y);
      //glVertex2f(e->x + e->width,  e->y);
      //glVertex2f(e->x + e->width,  e->y + e->height);
      //glVertex2f(e->x,             e->y + e->height);
    glEnd();
  glEndList();
}
