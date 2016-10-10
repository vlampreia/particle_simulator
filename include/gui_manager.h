#ifndef _GUI_MANAGER_H__
#define _GUI_MANAGER_H__


#define GUI_MANAGER_INIT_CAPACITY 50

struct vector;
struct gui_element;

struct gui_manager {
  struct vector *elements;

  int w_width, w_height;
  int top, left;
};


struct gui_manager *gui_manager_new     (void);
void                gui_manager_delete  (struct gui_manager **m);
 
void  gui_manager_add_element     (struct gui_manager *m, struct gui_element *e);
struct gui_element *gui_manager_new_element     (struct gui_manager *m, const char *str, void(*callback)(void));

void  gui_manager_draw            (struct gui_manager *m);

void  gui_manager_set_dimensions  (struct gui_manager *m, int width, int height, int top, int left);

void  gui_manager_event_click     (struct gui_manager *m, int x, int y, int state);

#endif
