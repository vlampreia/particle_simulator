#ifndef _GUI_ELEMENT_H__
#define _GUI_ELEMENT_H__

#include <GL/glut.h>

struct gui_element {
  int x, y;
  int width, height;
  char *str;
  void (*callback)(void);

  GLuint compiled_list;
};


//------------------------------------------------------------------------------

struct gui_element *gui_element_new     (int x, int y, int width, int height, const char *str, void(*callback)(void));
void                gui_element_delete  (struct gui_element **e);

void  gui_element_set_position    (struct gui_element *e, int x, int y);
void  gui_element_set_dimensions  (struct gui_element *e, int width, int height);
void  gui_element_set_str         (struct gui_element *e, const char *str);
void  gui_element_set_callback    (struct gui_element *e, void(*callback)(void));

int   gui_element_is_inside (struct gui_element *e, int x, int y);

void  gui_element_draw  (struct gui_element *e);


#endif
