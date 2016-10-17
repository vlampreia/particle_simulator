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
#define DEG_TO_RAD 0.017453293
#define NUM_PARTICLES 1000000
#define NUM_EMITTERS 2
static int _window_width = 800;
static int _window_height = 600;

static struct vector3f _mouseWorldPos = (struct vector3f) {0.0f, 0.0f, 0.0f};

static int _ctrl_mode = 1;
static int _ctrl_edit = 0;
static int _ctrl_edit_mode = 0;

static int _mouse_x = 0;
static int _mouse_y = 0;
static int _mouse_x_p = 0;
static int _mouse_y_p = 0;
static int _dragging = 0;
static int _move_emitter = 0;
static int _drawWalls = 1;

static char _gui_buffer[256];

GLdouble _modelview[16];
GLdouble _projection[16];
GLint    _viewport[4];


struct {
  int lastRenderTime;
  int lastRenderDuration;
} _statistics = {0, 0};

// Display list for coordinate axis 
GLuint axisList;
GLuint crosshairList;

int AXIS_SIZE= 200;
static int axisEnabled= 1;

int ptime = 0;
int msq = 0;


static void _toggleEdit() {
  _ctrl_edit = !_ctrl_edit;
  if (!_ctrl_edit) glutSetCursor(GLUT_CURSOR_INHERIT);
}

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

static struct gui_element *txt_rate = NULL;
static struct gui_element *btn_incRate = NULL;
static struct gui_element *btn_decRate = NULL;

static struct gui_element *txt_mass = NULL;
static struct gui_element *btn_decMass = NULL;
static struct gui_element *btn_incMass = NULL;

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
  p->color_alpha = 0.7f;
}

//------------------------------------------------------------------------------

static void step_simulation(void) {
  int t = glutGet(GLUT_ELAPSED_TIME);
  int dt = t - ptime;
  ptime = t;

  msq += dt;

  while (msq >= 10) {
    particle_system_step(_pSystem, t, dt);
    msq = 0;
    //msq -= 10;
  }

  glutPostRedisplay();
}

//------------------------------------------------------------------------------

static void render_particles(void) {
  int active = 0;

  glPointSize(5.0f);
  glBegin(GL_POINTS);
    for (size_t i=0; i<_pSystem->particles->size; ++i) {
      struct particle *p = (struct particle*)_pSystem->particles->elements[i];
      if (!p->active) continue;

      glColor4f(p->color.x, p->color.y, p->color.z, p->color_alpha);
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

  snprintf(_gui_buffer, 256, "FPS: %d", 1000/(_statistics.lastRenderDuration+1)* 1000);
  gui_element_set_str(txt_fps, _gui_buffer, 0);

  glLoadIdentity();
  gluLookAt(_camera.pos.x, _camera.pos.y, _camera.pos.z,
            0.0, 0.0, 0.0,
            0.0, 1.0, 0.0);
  // Clear the screen
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  int height = 400;
  int width = 400;
  int depth = 400;

  glBegin(GL_TRIANGLES);
  glColor3f(0.95f, 0.95f, 0.95f);
    glVertex3f( width, 0.0f,  depth);
    glVertex3f(-width, 0.0f,  depth);
    glVertex3f(-width, 0.0f, -depth);
    glVertex3f(-width, 0.0f, -depth);
    glVertex3f( width, 0.0f,  depth);
    glVertex3f( width, 0.0f, -depth);
  glEnd();


  double x,y,z;
  glGetDoublev(GL_MODELVIEW_MATRIX, _modelview);
  glGetDoublev(GL_PROJECTION_MATRIX, _projection);
  glGetIntegerv(GL_VIEWPORT, _viewport);

  GLfloat _z;
  glReadPixels(_mouse_x, _viewport[3]-_mouse_y,1,1,GL_DEPTH_COMPONENT, GL_FLOAT, &_z);
  gluUnProject(
    _mouse_x, _viewport[3] - _mouse_y, _z,
    _modelview, _projection, _viewport,
    &x, &y, &z
  );

  _mouseWorldPos.x = x;
  _mouseWorldPos.y = y;
  _mouseWorldPos.z = z;


  glBegin(GL_TRIANGLES);
    glColor4f(0.95f, 0.95f, 0.95f, 1.0f);
    glVertex3f(width, 0.0f, depth);
    glVertex3f(-width, 0.0f, depth);
    glColor4f(0.95f, 0.95f, 0.95f, 0.0f);
    glVertex3f(-width, 0.0f, depth + 500.0f);

    glVertex3f(-width, 0.0f, depth + 500.0f);
    glVertex3f(width, 0.0f, depth + 500.0f);
    glColor4f(0.95f, 0.95f, 0.95f, 1.0f);
    glVertex3f(width, 0.0f, depth);

    glVertex3f(width, 0.0f, depth);
    glVertex3f(width, 0.0f, -depth);
    glColor4f(0.95f, 0.95f, 0.95f, 0.0f);
    glVertex3f(width + 500.0f, 0.0f, -depth);

    glVertex3f(width + 500.0f, 0.0f, -depth);
    glVertex3f(width + 500.0f, 0.0f, depth);
    glColor4f(0.95f, 0.95f, 0.95f, 1.0f);
    glVertex3f(width, 0.0f, depth);

    glVertex3f(-width, 0.0f, -depth);
    glColor4f(0.95f, 0.95f, 0.95f, 0.0f);
    glVertex3f(-500-width, 0.0f, -depth);
    glVertex3f(-500-width, 0.0f, depth);

    glVertex3f(-500-width, 0.0f, depth);
    glColor4f(0.95f, 0.95f, 0.95f, 1.0f);
    glVertex3f(-width, 0.0f, -depth);
    glVertex3f(-width, 0.0f, depth);

    glVertex3f(-width, 0.0f, -depth);
    glColor4f(0.95f, 0.95f, 0.95f, 0.0f);
    glVertex3f(-width, 0.0f, -500-depth);
    glVertex3f(width, 0.0f, -500-depth);

    glVertex3f(width, 0.0f, -500-depth);
    glColor4f(0.95f, 0.95f, 0.95f, 1.0f);
    glVertex3f(-width, 0.0f, -depth);
    glVertex3f(width, 0.0f, -depth);
  glEnd();


  if (_ctrl_edit) {
    if(axisEnabled) glCallList(axisList);

    for (size_t i=0; i<_pSystem->emitters->size; ++i) {
      struct emitter *e = _pSystem->emitters->elements[i];

      struct vector3f *col = &e->base_particle->color;

      glPointSize(10.0f);
      if (i == _ctrl_mode) glColor3f(col->x, col->y, col->z);
      else glColor3f(col->x/2, col->y/2, col->z/2);

      double pitch = e->pitch * DEG_TO_RAD;
      double yaw   = e->yaw * DEG_TO_RAD;
      double pmod = e->vert_angle /2* DEG_TO_RAD;
      double ymod = e->horiz_angle /2* DEG_TO_RAD;

      struct vector3f angle = (struct vector3f) {
        e->position.x + 100 * -cos(pitch) * sin(yaw),
        e->position.y + 100 *  sin(pitch),
        e->position.z + 100 *  cos(pitch) * cos(yaw)
      };
      //vector3f_normalise(&angle);
///      angle.x += e->position.x;
///      angle.y += e->position.y;
///      angle.z += e->position.z;
///      angle.x *= 100;
///      angle.y *= 100;
///      angle.z *= 100;
      //vector3f_normalise(&angle);
//      angle.x *= 100;
//      angle.y *= 100;
//      angle.z *= 100;

      struct vector3f angle_l = (struct vector3f) {
        e->position.x + 100*-cos(pitch + pmod) * sin(yaw + ymod),
        e->position.y + 100* sin(pitch + pmod),
        e->position.z + 100* cos(pitch + pmod) * cos(yaw + ymod)
      };
//      vector3f_normalise(&angle_l);
//      angle_l.x *= 100;
//      angle_l.y *= 100;
//      angle_l.z *= 100;

      struct vector3f angle_r = (struct vector3f) {
        e->position.x +100* -cos(pitch - pmod) * sin(yaw - ymod),
        e->position.y +100*  sin(pitch - pmod),
        e->position.z +100*  cos(pitch - pmod) * cos(yaw - ymod)
      };
//      vector3f_normalise(&angle_r);
//      angle_r.x *= 100;
//      angle_r.y *= 100;
//      angle_r.z *= 100;

      glBegin(GL_POINTS);
        glVertex3f(e->position.x, e->position.y, e->position.z);
      glEnd();

      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glBegin(GL_TRIANGLES);
        glVertex3f(e->position.x, e->position.y, e->position.z);
        glVertex3f(angle.x, angle.y, angle.z);
        glVertex3f(angle.x, e->position.y, angle.z);

        //glVertex3f(e->position.x, e->position.y, e->position.z);
        //glVertex3f(angle_l.x, angle_l.y, angle_l.z);
        //glVertex3f(angle_l.x, angle_r.y, angle_l.z);

        //glVertex3f(e->position.x, e->position.y, e->position.z);
        //glVertex3f(angle_r.x, angle_l.y, angle_r.z);
        //glVertex3f(angle_r.x, angle_r.y, angle_r.z);
      glEnd();

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      if (i == _ctrl_mode) glColor4f(col->x, col->y, col->z, 0.7f);
      else glColor4f(col->x/2, col->y/2, col->z/2, 0.7f);

      glBegin(GL_TRIANGLES);
        glVertex3f(e->position.x, e->position.y, e->position.z);
        glVertex3f(angle.x, angle.y, angle.z);
        glVertex3f(angle.x, e->position.y, angle.z);
        glVertex3f(e->position.x, e->position.y, e->position.z);
      glEnd();
    }

    if (_z == 1.0f) {
      glutSetCursor(GLUT_CURSOR_INHERIT);
    } else {
      glutSetCursor(GLUT_CURSOR_NONE);
      glPushMatrix();
        glTranslatef(_mouseWorldPos.x, 0.0f, _mouseWorldPos.z);
        glCallList(crosshairList);
      glPopMatrix();
    }
  }


  render_particles();

  if (_drawWalls) {
    glBegin(GL_TRIANGLES);
    glColor4f(0.6f, 0.6f, 0.6f, 0.5f);
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

    glColor4f(0.8f, 0.8f, 0.8f, 0.5f);
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
    glEnd();
  }



  //mouse stuff

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
    case 119: _drawWalls = !_drawWalls; break;

    case 101: _toggleEdit(); break;

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
  switch(glutGetModifiers()) {
    case GLUT_ACTIVE_SHIFT:
      _ctrl_edit_mode = 1; break;
    case GLUT_ACTIVE_CTRL:
      _ctrl_edit_mode = 2; break;
    default:
      _ctrl_edit_mode = 0;
  }
  switch (button) {
    case 0:
      if (gui_manager_event_click(_guiManager, x, y, state)) break;

      _move_emitter = (_ctrl_edit && state == GLUT_DOWN);
      if (_move_emitter && _ctrl_edit_mode == 0) vector3f_copy(
        &_mouseWorldPos,
        &((struct emitter*)_pSystem->emitters->elements[_ctrl_mode])->position
      );

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
  e1->force = 40.8f;
  e1->base_particle = particle_new();
  initialise_particle(e1->base_particle);
  e1->base_particle->color = (struct vector3f){0.0f, 1.0f, 0.0f};
  e1->base_particle->collision_chaos = 0.0f;
  e1->frequency = 100;
  particle_system_add_emitter(_pSystem, e1);

  for (int i=0; i<7; ++i) {
    struct emitter *e = emitter_new(NULL);
    e->pitch = 20.0f + i*10;
    e->yaw += i*40;
    //e->orientation = (struct vector3f) {-1.0f * myRandom(), 1.0f - 0.8f *myRandom(), 1.0f * myRandom()};
    //vector3f_normalise(&e->orientation);
    e->force = 45.8f  - 1.5f * myRandom();
    e->base_particle = particle_new();
    initialise_particle(e->base_particle);
    e->base_particle->color = (struct vector3f){1.0f, 0.0f, 0.0f};
    e->frequency = 1;
    e->horiz_angle = i * 10.0f;
    e->vert_angle = i * 10.0f;
    particle_system_add_emitter(_pSystem, e);
  }

  struct emitter *e2 = emitter_new(NULL);
  e2->position = (struct vector3f) {50.0f, 0.0f, 50.0f};
  //e2->orientation = (struct vector3f) {1.0f, 0.5f, 0.0f};
  e2->pitch = 45.0f;
  e2->yaw = 0.0f;
  //vector3f_normalise(&e2->orientation);
  e2->force = 25.9f;
  e2->base_particle = particle_new();
  initialise_particle(e2->base_particle);
  e2->base_particle->mass = 1.0f;
  e2->base_particle->color = (struct vector3f){1.0f, 1.0f, 0.0f};
  e2->frequency = 1;
  particle_system_add_emitter(_pSystem, e2);

  e2 = emitter_new(NULL);
  e2->position = (struct vector3f) {-50.0f, 0.0f, 50.0f};
  //e2->orientation = (struct vector3f) {1.0f, 0.5f, 1.0f};
  e2->pitch = 45.0f;
  e2->yaw = 45.0f;
  //vector3f_normalise(&e2->orientation);
  e2->force = 40.0f;
  e2->base_particle = particle_new();
  initialise_particle(e2->base_particle);
  e2->base_particle->color = (struct vector3f){0.0f, 1.0f, 1.0f};
  e2->base_particle->collision_chaos = 0.01f;
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
  gui_manager_new_element(_guiManager,NULL,NULL);
  txt_ctrlMode  = gui_manager_new_element(_guiManager, "Emitter: 0", NULL);
  btn_autoFire  = gui_manager_new_element(_guiManager, "Auto Emit", NULL);
  btn_fire      = gui_manager_new_element(_guiManager, "Emit", myCb);
  txt_rate      = gui_manager_new_element(_guiManager, "Rate: 1000", NULL);
  btn_decRate   = gui_manager_new_element(_guiManager, "-", NULL);
  btn_incRate   = gui_manager_new_element(_guiManager, "+", NULL);
  gui_manager_new_element(_guiManager,NULL,NULL);
  btn_type      = gui_manager_new_element(_guiManager, "GL_POINT", NULL);
  txt_bounce    = gui_manager_new_element(_guiManager, "Bounce: 0.000", NULL);
  btn_decBounce = gui_manager_new_element(_guiManager, "-", NULL);
  btn_incBounce = gui_manager_new_element(_guiManager, "+", NULL);
  txt_mass      = gui_manager_new_element(_guiManager, "Mass: 1.00", NULL);
  btn_decMass   = gui_manager_new_element(_guiManager, "-", NULL);
  btn_incMass   = gui_manager_new_element(_guiManager, "+", NULL);
}

//------------------------------------------------------------------------------

static void mouse_move(int x, int y) {
  _mouse_x = x;
  _mouse_y = y;

  int dx = (x - _mouse_x_p) * 0.5f;
  int dy = (y - _mouse_y_p) * 0.5f;
  int delta = dx+dy;

  if (_move_emitter) {
    struct emitter *e = _pSystem->emitters->elements[_ctrl_mode];
    if (_ctrl_edit_mode == 0) {
      vector3f_copy(&_mouseWorldPos, &e->position);
    } else if (_ctrl_edit_mode == 1) {
      e->yaw += delta;
    } else if (_ctrl_edit_mode == 2) {
      e->pitch -= delta;
    }
  }

  if(_dragging) {
    camera_inc_yaw(&_camera, -dy);
    camera_inc_pitch(&_camera, -dx);
  }

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
  gluPerspective(60, (GLfloat)width / (GLfloat)height, 10.0, 10000.0);
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

static void makeCrosshair() {
  crosshairList = glGenLists(1);
  glNewList(crosshairList, GL_COMPILE);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
      glColor4f(0.5f, 0.5f, 0.5f, 0.0f);
      glVertex3f(-10000.0f, 0.0f,  0.0f);
      glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
      glVertex3f( 0.0f,     0.0f,  0.0f);
      glVertex3f( 0.0f,     0.0f,  0.0f);
      glColor4f(0.5f, 0.5f, 0.5f, 0.0f);
      glVertex3f( 10000.0f, 0.0f,  0.0f);

      glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
      glVertex3f( 0.0f,     0.0f,  0.0f);
      glColor4f(0.5f, 0.5f, 0.5f, 0.0f);
      glVertex3f( 0.0f,     0.0f, -10000.0f);
      glVertex3f( 0.0f,     0.0f,  10000.0f);
      glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
      glVertex3f( 0.0f,     0.0f,  0.0f);
    glEnd();
  glEndList();
}

//------------------------------------------------------------------------------

static void init_glut(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitWindowSize(_window_width, _window_height);
  glutInitWindowPosition(100, 100);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_MULTISAMPLE);
  glutCreateWindow("COMP37111 Particles");
  glutIdleFunc(step_simulation);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(mouse_move);
  glutPassiveMotionFunc(mouse_move);
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
  makeCrosshair();

  init_psys();
  _gui_update_gravtext();

  camera_set_distance(&_camera, 800);
  camera_set_pitch(&_camera, 0.0f);
  camera_set_yaw(&_camera, 0.0f);

  glEnable(GL_POINT_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_ALWAYS);
//  glDepthRange(0.0f, 1.0f);
  //glEnable(GL_CULL_FACE);
  //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &global_time_p);
  glutMainLoop();

  return 0;
}
