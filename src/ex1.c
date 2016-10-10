#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <GL/glut.h>

#include "vector3f.h"
#include "emitter.h"
#include "particle.h"

#include "gui_manager.h"
#include "gui_element.h"

#include "particle_system.h"
#include "camera.h"

//------------------------------------------------------------------------------


static struct gui_manager *_guiManager = NULL;
static struct particle_system *_pSystem = NULL;

static struct camera _camera;

static double myRandom(void)
{
  return ((-RAND_MAX/2 + rand())/(double)RAND_MAX);
}

//#define NUM_PARTICLES 300000
#define NUM_PARTICLES 10000
#define NUM_EMITTERS 2
static int _window_width = 800;
static int _window_height = 600;

static int _ctrl_mode = 0;

static int _distance = 800;
static int _mouse_x_p = 0;
static int _mouse_y_p = 0;
static double _yaw = 0.0f;
static double _pitch = 0.0f;
static int _dragging = 0;

static char _gui_buffer[256];

struct {
  int lastRenderTime;
  int lastRenderDuration;
} _statistics = {0, 0};

// Display list for coordinate axis 
GLuint axisList;

int AXIS_SIZE= 200;
static int axisEnabled= 1;

int ptime = 0;
int msq = 0;

//------------------------------------------------------------------------------

/* main UI */

static struct gui_element *txt_ctrlMode = NULL;
static struct gui_element *txt_fps      = NULL;
static struct gui_element *txt_mspf     = NULL;
static struct gui_element *txt_pcount   = NULL;

static struct gui_element *btn_fire     = NULL;
static struct gui_element *btn_autoFire = NULL;

static struct gui_element *txt_grav     = NULL;
static struct gui_element *btn_incG     = NULL;
static struct gui_element *btn_decG     = NULL;
static struct gui_element *btn_type     = NULL;
static struct gui_element *txt_bounce   = NULL;
static struct gui_element *btn_incBounce = NULL;
static struct gui_element *btn_decBounce = NULL;

//------------------------------------------------------------------------------
static void _gui_update_gravtext() {
  snprintf(_gui_buffer, 256, "Gravity %f", _pSystem->gravity);
  gui_element_set_str(txt_grav, _gui_buffer, 0);
}

static void _cb_decGrav() {
  _pSystem->gravity -= 0.001f;
  _gui_update_gravtext();
}

static void _cb_incGrav() {
  _pSystem->gravity += 0.001f;
  _gui_update_gravtext();
}

static void _set_ctrl_mode(int mode) {
  _ctrl_mode = mode;

  char *str = malloc(11);
  snprintf(str, 11, "Emitter: %d", _ctrl_mode);
  gui_element_set_str(txt_ctrlMode, str, 0);
  free(str);
}

//------------------------------------------------------------------------------

static void initialise_particle(struct particle *p) {
  p->mass = 0.2f + myRandom();
  p->velocity.x = 0.0f;//myRandom() * -0.8f;
  p->velocity.y = 0.0f;
  p->velocity.z = 0.0f;

  p->bounce = 0.6f;

  vector3f_init(&p->pos);
  p->tod_usec = 24000;

  p->color = (struct vector3f) {0.2f, 0.2f, 0.2};
}

//------------------------------------------------------------------------------

static void step_simulation(void) {
  int t = glutGet(GLUT_ELAPSED_TIME);
  int dt = t - ptime;
  ptime = t;

  msq += dt;

  while (msq >= 10) {
    particle_system_step(_pSystem, t, dt);
    msq -= 10;
  }

  glutPostRedisplay();
}

//------------------------------------------------------------------------------

static void render_particles(void) {
  int active = 0;

  glBegin(GL_POINTS);
    for (size_t i=0; i<_pSystem->particles->size; ++i) {
      struct particle *p = (struct particle*)_pSystem->particles->elements[i];
      if (!p->active) continue;

      glColor3f(p->color.x, p->color.y, p->color.z);
      glVertex3f(p->pos.x, p->pos.y, p->pos.z);
      active++;
    }
  glEnd();

  snprintf(_gui_buffer, 256, "Particles: %d", active);
  gui_element_set_str(txt_pcount, _gui_buffer, 1);
}

//------------------------------------------------------------------------------

static void display(void)
{
  int t = glutGet(GLUT_ELAPSED_TIME);
  _statistics.lastRenderDuration = t - _statistics.lastRenderTime;
  _statistics.lastRenderTime = t;

  snprintf(_gui_buffer, 256, "MSPF: %d", _statistics.lastRenderDuration);
  gui_element_set_str(txt_mspf, _gui_buffer, 0);

  snprintf(_gui_buffer, 256, "FPS: %d", 1000/_statistics.lastRenderDuration * 1000);
  gui_element_set_str(txt_fps, _gui_buffer, 0);

  glLoadIdentity();
  gluLookAt(_camera.pos.x, _camera.pos.y, _camera.pos.z,
            0.0, 0.0, 0.0,
            0.0, 1.0, 0.0);
  // Clear the screen
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // If enabled, draw coordinate axis

  int height = 400;
  int width = 400;
  int depth = 400;
  glBegin(GL_TRIANGLES);
  glColor3f(1.0f, 1.0f, 1.0f);
    glVertex3f(width, 0.0f,    depth);
    glVertex3f(width, height,  depth);
    glVertex3f(width, height, -depth);
    glVertex3f(width, height, -depth);
    glVertex3f(width, 0.0f,   -depth);
    glVertex3f(width, 0.0f,    depth);

    glVertex3f(-width, 0.0f,    depth);
    glVertex3f(-width, height,  depth);
    glVertex3f(-width, height, -depth);
    glVertex3f(-width, height, -depth);
    glVertex3f(-width, 0.0f,   -depth);
    glVertex3f(-width, 0.0f,    depth);

  glColor3f(0.98f, 0.98f, 0.98f);
    glVertex3f(-width, 0.0f,   depth);
    glVertex3f( width, 0.0f,   depth);
    glVertex3f( width, height, depth);
    glVertex3f( width, height, depth);
    glVertex3f(-width, height, depth);
    glVertex3f(-width, 0.0f,   depth);

    glVertex3f(-width, 0.0f,   -depth);
    glVertex3f( width, 0.0f,   -depth);
    glVertex3f( width, height, -depth);
    glVertex3f( width, height, -depth);
    glVertex3f(-width, height, -depth);
    glVertex3f(-width, 0.0f,   -depth);


  glColor3f(0.95f, 0.95f, 0.95f);
    glVertex3f( width, 0.0f,  depth);
    glVertex3f(-width, 0.0f,  depth);
    glVertex3f(-width, 0.0f, -depth);
    glVertex3f(-width, 0.0f, -depth);
    glVertex3f( width, 0.0f,  depth);
    glVertex3f( width, 0.0f, -depth);
  glEnd();

  if(axisEnabled) glCallList(axisList);

  glPointSize(4.0f);
  render_particles();

  gui_manager_draw(_guiManager);

  glutSwapBuffers();
}

//------------------------------------------------------------------------------

static void clean_exit(void) {
  exit(0);
}

//------------------------------------------------------------------------------

static void keyboard(unsigned char key, int x, int y)
{
  switch (key) {
    case 27: clean_exit(); break;

    case 102:
      ((struct emitter*)_pSystem->emitters->elements[_ctrl_mode])->firing = !((struct emitter*)_pSystem->emitters->elements[_ctrl_mode])->firing;
      break;

    case 32: emitter_fire(_pSystem->emitters->elements[_ctrl_mode]); break;

    case 97: axisEnabled = !axisEnabled; break;

    case 49: _set_ctrl_mode(1); break;
    case 50: _set_ctrl_mode(2); break;
    case 51: _set_ctrl_mode(3); break;
    case 52: _set_ctrl_mode(4); break;
    case 53: _set_ctrl_mode(5); break;
    case 54: _set_ctrl_mode(6); break;
    case 55: _set_ctrl_mode(7); break;
    case 56: _set_ctrl_mode(8); break;
    case 57: _set_ctrl_mode(9); break;
    case 48: _set_ctrl_mode(0); break;

    default: break;
  }

  glutPostRedisplay();
}


//------------------------------------------------------------------------------

static void myCb(void) {
  emitter_fire(_pSystem->emitters->elements[_ctrl_mode]);
}

//------------------------------------------------------------------------------

static void mouse(int button, int state, int x, int y) {
  switch (button) {
    case 0:
      gui_manager_event_click(_guiManager, x, y, state);
      //_ui_mouse(x, y);
      break;
    case 2:
      _dragging = (state == GLUT_DOWN);
      if (_dragging) {
        _mouse_x_p = x;
        _mouse_y_p = y;
      }
      break;

    case 3: camera_inc_distance(&_camera, -10); break;
    case 4: camera_inc_distance(&_camera,  10); break;

    default: break;
  }
}

//------------------------------------------------------------------------------

static void init_psys(void) {
  _pSystem = particle_system_new(NUM_PARTICLES);

  struct emitter *e1 = emitter_new(NULL);
  //e1->orientation = (struct vector3f) {0.2f, 0.8f, 0.1f};
  //vector3f_normalise(&e1->orientation);
  e1->pitch = 80.0f;
  e1->force = 0.8f;
  e1->base_particle = particle_new();
  initialise_particle(e1->base_particle);
  e1->base_particle->color = (struct vector3f){0.0f, 1.0f, 0.0f};
  e1->frequency = 100;
  particle_system_add_emitter(_pSystem, e1);

  for (int i=0; i<7; ++i) {
    struct emitter *e = emitter_new(NULL);
    e->pitch = 20.0f + i*10;
    e->yaw += i*40;
    //e->orientation = (struct vector3f) {-1.0f * myRandom(), 1.0f - 0.8f *myRandom(), 1.0f * myRandom()};
    //vector3f_normalise(&e->orientation);
    e->force = 0.8f  - 0.5f * myRandom();
    e->base_particle = particle_new();
    initialise_particle(e->base_particle);
    e->base_particle->color = (struct vector3f){1.0f, 0.0f, 0.0f};
    e->frequency = 1;
    particle_system_add_emitter(_pSystem, e);
  }

  struct emitter *e2 = emitter_new(NULL);
  e2->position = (struct vector3f) {50.0f, 0.0f, 50.0f};
  //e2->orientation = (struct vector3f) {1.0f, 0.5f, 0.0f};
  e2->pitch = 45.0f;
  e2->yaw = 0.0f;
  //vector3f_normalise(&e2->orientation);
  e2->force = 0.9f;
  e2->base_particle = particle_new();
  initialise_particle(e2->base_particle);
  e2->base_particle->color = (struct vector3f){1.0f, 1.0f, 0.0f};
  e2->frequency = 1;
  particle_system_add_emitter(_pSystem, e2);

  e2 = emitter_new(NULL);
  e2->position = (struct vector3f) {-50.0f, 0.0f, 50.0f};
  //e2->orientation = (struct vector3f) {1.0f, 0.5f, 1.0f};
  e2->pitch = 45.0f;
  e2->yaw = 45.0f;
  //vector3f_normalise(&e2->orientation);
  e2->force = 0.9f;
  e2->base_particle = particle_new();
  initialise_particle(e2->base_particle);
  e2->base_particle->color = (struct vector3f){0.0f, 1.0f, 1.0f};
  e2->frequency = 1;
  particle_system_add_emitter(_pSystem, e2);
}

//------------------------------------------------------------------------------

static void init_gui(void) {
  _guiManager = gui_manager_new();
  gui_manager_set_dimensions(_guiManager, _window_width, _window_height, 200, 10);

  txt_fps    = gui_manager_new_element(_guiManager, "FPS: 1000", NULL);
  txt_mspf   = gui_manager_new_element(_guiManager, "MSPF: 1000", NULL);
  txt_pcount = gui_manager_new_element(_guiManager,"Particles: 0", NULL);

  gui_manager_new_element(_guiManager,NULL,NULL);
  txt_grav = gui_manager_new_element(_guiManager, "Gravity 0.0000", NULL);
  btn_decG = gui_manager_new_element(_guiManager, "-", _cb_decGrav);
  btn_incG = gui_manager_new_element(_guiManager, "+", _cb_incGrav);

  gui_manager_new_element(_guiManager,NULL,NULL);
  txt_ctrlMode = gui_manager_new_element(_guiManager, "Emitter: 0", NULL);
  btn_autoFire = gui_manager_new_element(_guiManager, "Auto Emit", NULL);
  btn_fire     = gui_manager_new_element(_guiManager, "Emit", myCb);
  btn_type     = gui_manager_new_element(_guiManager, "GL_POINT", NULL);
  txt_bounce   = gui_manager_new_element(_guiManager, "Bounce: 0.000", NULL);
  btn_decBounce = gui_manager_new_element(_guiManager, "-", NULL);
  btn_incBounce = gui_manager_new_element(_guiManager, "+", NULL);
}

//------------------------------------------------------------------------------

static void mouse_move(int x, int y) {
  if (!_dragging) return;

  int dx = (x - _mouse_x_p) * 0.5f;
  int dy = (y - _mouse_y_p) * 0.5f;

  camera_inc_yaw(&_camera, -dy);
  camera_inc_pitch(&_camera, -dx);

  _mouse_x_p = x;
  _mouse_y_p = y;
}

//------------------------------------------------------------------------------

static void reshape(int width, int height)
{
  _window_width = width;
  _window_height = height;
  gui_manager_set_dimensions(_guiManager, width, height, 200, 10);
  glClearColor(0.9, 0.9, 0.9, 1.0);
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, (GLfloat)width / (GLfloat)height, 1.0, 10000.0);
  glMatrixMode(GL_MODELVIEW);
}

//------------------------------------------------------------------------------

static void makeAxes(void) {
// Create a display list for drawing coord axis
  axisList = glGenLists(1);
  glNewList(axisList, GL_COMPILE);
      glLineWidth(2.0);
      glBegin(GL_LINES);
      glColor3f(1.0, 0.0, 0.0);       // X axis - red
      glVertex3f(0.0, 0.0, 0.0);
      glVertex3f(AXIS_SIZE, 0.0, 0.0);
      glColor3f(0.0, 1.0, 0.0);       // Y axis - green
      glVertex3f(0.0, 0.0, 0.0);
      glVertex3f(0.0, AXIS_SIZE, 0.0);
      glColor3f(0.0, 0.0, 1.0);       // Z axis - blue
      glVertex3f(0.0, 0.0, 0.0);
      glVertex3f(0.0, 0.0, AXIS_SIZE);
    glEnd();
  glEndList();
}

//------------------------------------------------------------------------------

static void init_glut(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitWindowSize(_window_width, _window_height);
  glutInitWindowPosition(100, 100);
  glutInitDisplayMode(GLUT_DOUBLE);
  glutCreateWindow("COMP37111 Particles");
  glutIdleFunc(step_simulation);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(mouse_move);
  glutReshapeFunc(reshape);

  //glEnable(GL_LIGHTING);
//  glEnable(GL_LIGHT0);
//  glEnable(GL_COLOR_MATERIAL);
}

//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  srand(time(NULL));
  init_glut(argc, argv);

  init_gui();
  makeAxes();

  init_psys();
  _gui_update_gravtext();

  camera_set_distance(&_camera, 800);
  camera_set_pitch(&_camera, 0.0f);
  camera_set_yaw(&_camera, 0.0f);

  glEnable(GL_POINT_SMOOTH);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  //glEnable(GL_DEPTH_TEST);
  //glDepthFunc(GL_LESS);
  //glEnable(GL_CULL_FACE);
  //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &global_time_p);
  glutMainLoop();

  return 0;
}
