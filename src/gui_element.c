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

  if (str) {
    e->str = malloc(sizeof(str));
    strcpy(e->str, str);
  }
  e->callback = callback;

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

void gui_element_set_str(struct gui_element *e, const char *str) {
  if (e->str) free(e->str);
  e->str = malloc(sizeof(str));
  strcpy(e->str, str);
}

void gui_element_set_callback(struct gui_element *e, void(*callback)(void)) {
  e->callback = callback;
}

int gui_element_is_inside(struct gui_element *e, int x, int y) {
  return (
    x >= e->x && x <= e->x + e->width &&
    y >= e->y && y <= e->y + e->height
  );
}

void gui_element_draw(struct gui_element *e) {
  glColor3f(0.7f, 0.7f, 0.7f);
  glCallList(e->compiled_list);

  if (!e->str) return;
  glColor3f(0.4f, 0.4f, 0.4f);
  size_t i = 0;
  int x_offset = 5;
  while(e->str[i] != '\0' && x_offset < e->width - GUI_ELEMENT_CHAR_WIDTH) {
    glRasterPos2i(e->x + x_offset, 5 + e->y);
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, e->str[i]);
    ++i;
    x_offset += GUI_ELEMENT_CHAR_WIDTH;
  }
}

static void _compile(struct gui_element *e) {
  e->compiled_list = glGenLists(1);

  glNewList(e->compiled_list, GL_COMPILE);
    glBegin(GL_QUADS);
      glVertex2f(e->x,             e->y);
      glVertex2f(e->x + e->width,  e->y);
      glVertex2f(e->x + e->width,  e->y + e->height);
      glVertex2f(e->x,             e->y + e->height);
    glEnd();
  glEndList();
}