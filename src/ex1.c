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

static double _avg(double a[], size_t n) {
  double s = 0;
  for (size_t i=0; i<n; ++i) {
    s += a[i];
  }
  return s/n;
}


static struct gui_manager *_guiManager = NULL;
static struct particle_system *_pSystem = NULL;

static struct camera _camera;

static double myRandom(void)
{
  return ((-RAND_MAX/2 + rand())/(double)RAND_MAX);
}

//#define NUM_PARTICLES 300000
#define DEG_TO_RAD 0.017453293
//#define NUM_PARTICLES 1000000
#define NUM_PARTICLES 300000
#define NUM_EMITTERS 2
#define NSEC_DIV 1000000 / 1000

static int _window_width = 800;
static int _window_height = 600;

static struct vector3f _mouseWorldPos =  {0.0, 0.0, 0.0};

static int _ctrl_mode = 1;
static int _ctrl_edit = 0;
static int _ctrl_edit_mode = 0;

static int _mouse_x = 0;
static int _mouse_y = 0;
static int _mouse_x_p = 0;
static int _mouse_y_p = 0;
static int _dragging = 0;
static int _move_emitter = 0;
static int _drawWalls = 0;
static int _drawFloor = 0;
static int _draw_ui = 1;
static int _do_phys = 1;

static char _gui_buffer[256];

GLdouble _modelview[16];
GLdouble _projection[16];
GLint    _viewport[4];


struct {
  double lastRenderTime;
  double lastRenderDuration;
  double lastRenderDurations[10];
  size_t rdi;

  double lastSimTime;
  double lastSimDuration;
  double lastSimDurations[10];
  size_t sdi;

  size_t maxidx;
} _statistics = {0,0,{0,0,0,0,0,0,0,0,0,0},0, 0,0,{0,0,0,0,0,0,0,0,0,0},0,10};

// Display list for coordinate axis 
GLuint axisList;
GLuint crosshairList;
GLuint floorList;
GLuint gradFloorList;
GLuint wallsList;

int AXIS_SIZE= 200;
static int axisEnabled= 1;

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
static struct gui_element *txt_sfps     = NULL;
static struct gui_element *txt_smspf    = NULL;
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
static struct gui_element *txt_airDens  = NULL;
static struct gui_element *txt_friction = NULL;

static struct gui_element *txt_rate = NULL;
static struct gui_element *btn_incRate = NULL;
static struct gui_element *btn_decRate = NULL;

static struct gui_element *txt_mass = NULL;
static struct gui_element *btn_decMass = NULL;
static struct gui_element *btn_incMass = NULL;

static struct gui_element *txt_force = NULL;

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

  char *str = malloc(15);
  snprintf(str, 11, "Emitter: %d", _ctrl_mode);
  gui_element_set_str(txt_ctrlMode, str, 0);

  struct emitter *e = _pSystem->emitters->elements[mode];
  snprintf(str, 11, "Rate: %f", e->frequency);
  gui_element_set_str(txt_rate, str, 0);

  snprintf(str, 15, "Bounce: %f", e->base_particle->bounce);
  gui_element_set_str(txt_bounce, str, 0);

  snprintf(str, 11, "Mass: %f", e->base_particle->mass);
  gui_element_set_str(txt_mass, str, 0);

  snprintf(str, 11, "Force: %f", e->force);
  gui_element_set_str(txt_force, str, 0);

  free(str);
}

//------------------------------------------------------------------------------

static void initialise_particle(struct particle *p) {
  //p->mass = 0.5f + myRandom();
  p->mass = 40;
  p->velocity[0] = 0.0f;//myRandom() * -0.8f;
  p->velocity[1] = 0.0f;
  p->velocity[2] = 0.0f;

  p->bounce = 0.6f;

  p->pos[0] = 0.0f;
  p->pos[1] = 0.0f;
  p->pos[2] = 0.0f;
  //vector3f_init(&p->pos);
  //p->tod_usec = 24000;
  p->tod_usec = 300;
  p->tod_max = p->tod_usec;

  p->color[0] = 50;
  p->color[1] = 50;
  p->color[2] = 50;
  p->color[3] = 255;

  p->size = 10.0f;
  //p->color = {50, 50, 50, 255};
  //p->color = (struct vector3f) {0.2f, 0.2f, 0.2};
  //p->color_alpha = 0.7f;
}

//------------------------------------------------------------------------------
struct timespec global_time_p;

double t = 0.0;
double accumulator = 0.0;
static void _step_simulation(void) {
//  struct timespec t;
//  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
//  float dt = (t.tv_nsec - global_time_p.tv_nsec)/1000000;
//  global_time_p = t;
//

  struct timespec _t;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &_t);
  double newTime = _t.tv_sec + (_t.tv_nsec/1000000000.0f);
  double dt = newTime - _statistics.lastSimTime;
  if (dt > 0.15) dt = 0.15;
  if (_statistics.sdi == _statistics.maxidx-1) _statistics.sdi = 0;
  else ++_statistics.sdi;

  _statistics.lastSimDurations[_statistics.sdi] = dt;
  _statistics.lastSimDuration = _avg(_statistics.lastSimDurations, _statistics.maxidx);
  _statistics.lastSimTime = newTime;

  if (_do_phys) {
    accumulator += dt;

    //TODO: FIX TIMESTEP -- is idlefunc doing some dt shit??
    while (accumulator >= dt) {
      particle_system_step(_pSystem, t, 1);//1.0);
      accumulator -= dt;
      t += dt;
    }
  }
  

//  clock_t t = clock() / (CLOCKS_PER_SEC / 1000);
//  float dt = t - gt;
//  gt = t;

//  int t = glutGet(GLUT_ELAPSED_TIME);
//  _statistics.lastSimDuration = t - _statistics.lastSimTime;
//  _statistics.lastSimTime = t;

//  float dt = 1000/_statistics.lastSimDuration / 60.0f;
  //msq += _statistics.lastSimDuration;

//  while (msq >= 10) {
    //particle_system_step(_pSystem, t, dt);
//    msq = 0;
//    msq -= 10;
//  }

  glutPostRedisplay();
}

//------------------------------------------------------------------------------

static double _clampedRand(double min, double max) {
  return min + rand() / ((double)RAND_MAX/(max-min));
}
static int count = NUM_PARTICLES/10;
static inline void render_particles(void) {
  int active = 0;

  glPointSize(1.0f);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, _pSystem->particle_pos);
  glColorPointer(4, GL_UNSIGNED_BYTE, 0, _pSystem->particle_col);
  glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
  //glDrawRangeElements(GL_POINTS, 0, count, count, GL_UNSIGNED_BYTE, _pSystem->particle_idx);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  //glBegin(GL_POINTS);
//    for (size_t i=0; i<_pSystem->particles->size; ++i) {
//      struct particle *p = (struct particle*)_pSystem->particles->elements[i];
//      if (!p->active) continue;
//
//      glPointSize(_clampedRand(1.0f, 5.0f));
//  glBegin(GL_POINTS);
//      glColor4ubv(p->color);
//      glVertex3fv(p->pos);
//      active++;
//  glEnd();
//    }
//  glEnd();

  snprintf(_gui_buffer, 256, "Particles: %d", active);
  gui_element_set_str(txt_pcount, _gui_buffer, 1);
}

//------------------------------------------------------------------------------

static void _render(void)
{
  int t = glutGet(GLUT_ELAPSED_TIME);
  double dt = t - _statistics.lastRenderTime;
  if (_statistics.rdi == _statistics.maxidx-1) _statistics.rdi = 0;
  else ++_statistics.rdi;
  _statistics.lastRenderDurations[_statistics.rdi] = dt;
  _statistics.lastRenderDuration = _avg(_statistics.lastRenderDurations, _statistics.maxidx);
  _statistics.lastRenderTime = t;

  snprintf(_gui_buffer, 256, "MSPF: %f", _statistics.lastRenderDuration);
  gui_element_set_str(txt_mspf, _gui_buffer, 0);

  snprintf(_gui_buffer, 256, "FPS: %f", 1000/(_statistics.lastRenderDuration+1));
  gui_element_set_str(txt_fps, _gui_buffer, 0);

  snprintf(_gui_buffer, 256, "SIM MSPF: %f", _statistics.lastSimDuration);
  gui_element_set_str(txt_smspf, _gui_buffer, 0);

  snprintf(_gui_buffer, 256, "SIM FPS: %f", 1/(_statistics.lastSimDuration+1));
  gui_element_set_str(txt_sfps, _gui_buffer, 0);

  snprintf(_gui_buffer, 256, "air: %f", _pSystem->air_density);
  gui_element_set_str(txt_airDens, _gui_buffer, 0);

  snprintf(_gui_buffer, 256, "friction: %f", _pSystem->friction);
  gui_element_set_str(txt_friction, _gui_buffer, 0);

  glLoadIdentity();
  gluLookAt(_camera.pos.x, _camera.pos.y, _camera.pos.z,
            0.0, _camera.distance/8, 0.0,
            0.0, 1.0, 0.0);

  // Clear the screen
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  double x,y,z;
  GLfloat _z;

  if (_drawFloor) {
    glCallList(floorList);

    if (_ctrl_edit) {
      glGetDoublev(GL_MODELVIEW_MATRIX, _modelview);
      glGetDoublev(GL_PROJECTION_MATRIX, _projection);
      glGetIntegerv(GL_VIEWPORT, _viewport);

      glReadPixels(_mouse_x, _viewport[3]-_mouse_y,1,1,GL_DEPTH_COMPONENT, GL_FLOAT, &_z);
      gluUnProject(
        _mouse_x, _viewport[3] - _mouse_y, _z,
        _modelview, _projection, _viewport,
        &x, &y, &z
      );
    }
  }

  if (_drawFloor) glCallList(gradFloorList);

  if (_ctrl_edit) {
    _mouseWorldPos.x = x;
    _mouseWorldPos.y = y;
    _mouseWorldPos.z = z;

    if(axisEnabled) glCallList(axisList);

    for (size_t i=0; i<_pSystem->emitters->size; ++i) {
      struct emitter *e = _pSystem->emitters->elements[i];

      GLubyte *col = e->base_particle->color;

      glPointSize(10.0f);
      if (i == _ctrl_mode) glColor3ubv(col);
      else glColor3ub(col[0]/2, col[1]/2, col[2]/2);

      double pitch = e->pitch * DEG_TO_RAD;
      double yaw   = e->yaw * DEG_TO_RAD;
      double pmod = e->vert_angle /2* DEG_TO_RAD;
      double ymod = e->horiz_angle /2* DEG_TO_RAD;

      struct vector3f angle = (struct vector3f) {
        e->position.x + 100 * -cos(pitch) * sin(yaw),
        e->position.y + 100 *  sin(pitch),
        e->position.z + 100 *  cos(pitch) * cos(yaw)
      };

      glBegin(GL_POINTS);
        glVertex3f(e->position.x, e->position.y, e->position.z);
      glEnd();

      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glBegin(GL_TRIANGLES);
        glVertex3f(e->position.x, e->position.y, e->position.z);
        glVertex3f(angle.x, angle.y, angle.z);
        glVertex3f(angle.x, e->position.y, angle.z);
      glEnd();

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      if (i == _ctrl_mode) glColor4ub(col[0], col[1], col[2], 200);
      else glColor4ub(col[0]/2, col[1]/2, col[2]/2, 200);

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

  glPointSize(10);
  glBegin(GL_POINTS);
  for (size_t i=0; i<3; ++i) {
  glColor3f(1-_pSystem->attractors[i][3]/10,0,1.0);
    glVertex3f(
      _pSystem->attractors[i][0],
      _pSystem->attractors[i][1],
      _pSystem->attractors[i][2]
    );
  }
  glEnd();

  if (_drawWalls) {
    glCallList(wallsList);
  }

  glPointSize(1);
  if (_draw_ui) gui_manager_draw(_guiManager);

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

    //case 32: emitter_fire(_pSystem->emitters->elements[_ctrl_mode], 1); break;

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
    case 87: _drawFloor = !_drawFloor; break;

    case 65: _pSystem->friction += 0.01f; break;
    case 83: _pSystem->friction -= 0.01f; break;
    case 68: _pSystem->air_density += 0.01f; break;
    case 70: _pSystem->air_density -= 0.01f; break;

    case 99: _pSystem->collideWalls = !_pSystem->collideWalls; break;
    case 67: _pSystem->collideFloor = !_pSystem->collideFloor; break;

    case 117: _draw_ui = !_draw_ui; break;
    case 101: _toggleEdit(); break;

    case 32: _do_phys = !_do_phys;

    default: break;
  }

  glutPostRedisplay();
}


//------------------------------------------------------------------------------

static void myCb(void) {
  emitter_fire(_pSystem->emitters->elements[_ctrl_mode], 1);
}

//------------------------------------------------------------------------------

static void mouse(int button, int state, int x, int y) {
  int scroll_amt = 100;
  switch(glutGetModifiers()) {
    case GLUT_ACTIVE_SHIFT:
      _ctrl_edit_mode = 1;
      scroll_amt = 10000;
      break;
    case GLUT_ACTIVE_CTRL:
      _ctrl_edit_mode = 2; break;
    default:
      scroll_amt = 100;
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

    case 3: camera_inc_distance(&_camera, -scroll_amt); break;
    case 4: camera_inc_distance(&_camera,  scroll_amt); break;

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
  e1->force = 600.8f;
  e1->yaw = 80.0f;
  e1->base_particle = particle_new();
  initialise_particle(e1->base_particle);
  e1->base_particle->color[0] = 0;
  e1->base_particle->color[1] = 255;
  e1->base_particle->color[2] = 0;
  e1->base_particle->color[3] = 255;
  //e1->base_particle->color = (struct vector3f){0.0f, 1.0f, 0.0f};
  e1->base_particle->collision_chaos = 1.4f;
  e1->base_particle->mass = 1.0f;
  e1->base_particle->bounce = 0.99f;
  e1->frequency = 0.5;
  particle_system_add_emitter(_pSystem, e1);

  for (int i=0; i<7; ++i) {
    struct emitter *e = emitter_new(NULL);
    e->pitch = 20.0f + i*10;
    e->yaw += i*40;
    //e->orientation = (struct vector3f) {-1.0f * myRandom(), 1.0f - 0.8f *myRandom(), 1.0f * myRandom()};
    //vector3f_normalise(&e->orientation);
    e->force = 1.0f + i * 5.0f;
    //e->force = 45.8f  - 1.5f * myRandom();
    e->base_particle = particle_new();
    initialise_particle(e->base_particle);
    e->position = (struct vector3f) {-500.0f*i, 500.0f, i*500.0f};
    e->base_particle->color[0] = 2550;
    e->base_particle->color[1] = 40 * i;
    e->base_particle->color[2] = 20 * i;
    e->base_particle->color[3] = 255;
    e->base_particle->base_color[0] = 255;
    e->base_particle->base_color[1] = 40 * i;
    e->base_particle->base_color[2] = 20 * i;
    e->base_particle->base_color[3] = 255;
    e->base_particle->mass = 0.5f;
    e->base_particle->bounce = 0.9999f;
    e->base_particle->collision_chaos = 0.4f;
    //e->base_particle->color = (struct vector3f){1.0f, 0.0f, 0.0f};
    e->base_particle->tod_usec = 5000;
    e->base_particle->tod_max = e->base_particle->tod_usec;
    e->emission_count = 10;
    e->frequency = 0;
    e->horiz_angle = i * 10.0f;
    e->vert_angle = i * 10.0f;
    particle_system_add_emitter(_pSystem, e);
  }

  struct emitter *e2 = emitter_new(NULL);
////  //e2->position[0] = 50.0f;
////  //e2->position[1] = 0.0f;
////  //e2->position[2] = 50.0f;
////  e2->position = (struct vector3f) {50.0f, 0.0f, 50.0f};
////  //e2->orientation = (struct vector3f) {1.0f, 0.5f, 0.0f};
////  e2->pitch = 45.0f;
////  e2->yaw = 0.0f;
////  //vector3f_normalise(&e2->orientation);
////  e2->force = 25.9f;
////  e2->base_particle = particle_new();
////  initialise_particle(e2->base_particle);
////  e2->base_particle->mass = 1.0f;
////  e2->base_particle->color[0] = 255;
////  e2->base_particle->color[1] = 255;
////  e2->base_particle->color[2] = 0;
////  //e2->base_particle->color = (struct vector3f){1.0f, 1.0f, 0.0f};
////  e2->frequency = 0;
////  particle_system_add_emitter(_pSystem, e2);
////
////  e2 = emitter_new(NULL);
  //e2->position[0] = -50.0f;
  //e2->position[1] = 50.0f;
  //e2->position[2] = 50.0f;
  e2->position = (struct vector3f) {-50.0f, 500.0f, 50.0f};
  //e2->orientation = (struct vector3f) {1.0f, 0.5f, 1.0f};
  e2->pitch = 0.0f;
  e2->yaw = 0.0f;
  e2->horiz_angle = 720.0f;
  e2->vert_angle = 720.0f;
  //vector3f_normalise(&e2->orientation);
  e2->force = 2000;//60.01f;
  e2->base_particle = particle_new();
  initialise_particle(e2->base_particle);
  e2->base_particle->color[0] = 255;
  e2->base_particle->color[1] = 175;
  e2->base_particle->color[2] = 105;
  e2->base_particle->color[3] = 100;
  e2->base_particle->base_color[0] = 255;
  e2->base_particle->base_color[1] = 115;
  e2->base_particle->base_color[2] = 105;
  e2->base_particle->base_color[3] = 55;
  e2->base_particle->mass = 0.8f;
  e2->base_particle->bounce = 0.9f;
  e2->base_particle->size = 3.0f;
  e2->base_particle->tod_usec = 5000;
  e2->base_particle->tod_max = e2->base_particle->tod_usec;
  //e2->base_particle->color = (struct vector3f){0.0f, 1.0f, 1.0f};
  e2->base_particle->collision_chaos = 0.01f;
  e2->frequency = 2;
  e2->emission_count = 50000;
  particle_system_add_emitter(_pSystem, e2);

  e2 = emitter_new(NULL);
  e2->position = (struct vector3f) {-5000.0f, 5000.0f, 5000.0f};
  //e2->orientation = (struct vector3f) {1.0f, 0.5f, 1.0f};
  e2->pitch = 0.0f;
  e2->yaw = 0.0f;
  e2->horiz_angle = 720.0f;
  e2->vert_angle = 720.0f;
  //vector3f_normalise(&e2->orientation);
  e2->force = 900;//60.01f;
  e2->base_particle = particle_new();
  initialise_particle(e2->base_particle);
  e2->base_particle->color[0] = 255;
  e2->base_particle->color[1] = 175;
  e2->base_particle->color[2] = 105;
  e2->base_particle->color[3] = 0;
  e2->base_particle->base_color[0] = 255;
  e2->base_particle->base_color[1] = 165;
  e2->base_particle->base_color[2] = 105;
  e2->base_particle->base_color[3] = 50;
  e2->base_particle->mass = 0.8f;
  e2->base_particle->bounce = 0.9f;
  e2->base_particle->size = 3.0f;
  //e2->base_particle->color = (struct vector3f){0.0f, 1.0f, 1.0f};
  e2->base_particle->collision_chaos = 0.01f;
  e2->frequency = 0.0;
  e2->emission_count = 500;
  e2->base_particle->tod_usec = 1000;
  e2->base_particle->tod_max = e2->base_particle->tod_usec;
  particle_system_add_emitter(_pSystem, e2);
}

//------------------------------------------------------------------------------

static void init_gui(void) {
  _guiManager = gui_manager_new();
  gui_manager_set_dimensions(_guiManager, _window_width, _window_height, 200, 10);

  txt_fps    = gui_manager_new_element(_guiManager, "FPS: 1000", NULL);
  txt_mspf   = gui_manager_new_element(_guiManager, "MSPF: 1000", NULL);
  txt_sfps   = gui_manager_new_element(_guiManager, "SIM FPS: 1000", NULL);
  txt_smspf  = gui_manager_new_element(_guiManager, "SIM MSPF: 1000", NULL);
  txt_pcount = gui_manager_new_element(_guiManager,"Particles: 0", NULL);

  gui_manager_new_element(_guiManager,NULL,NULL);
  txt_grav = gui_manager_new_element(_guiManager, "Gravity 0.0000", NULL);
  btn_decG = gui_manager_new_element(_guiManager, "-", _cb_decGrav);
  btn_incG = gui_manager_new_element(_guiManager, "+", _cb_incGrav);
  txt_airDens = gui_manager_new_element(_guiManager, "air: 0.0000", NULL);
  txt_friction = gui_manager_new_element(_guiManager, "friction: 0.0000", NULL);

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
  txt_force     = gui_manager_new_element(_guiManager, "Force: 000.0", NULL);
}

//------------------------------------------------------------------------------

static void mouse_move(int x, int y) {
  _mouse_x = x;
  _mouse_y = y;

  double dx = (x - _mouse_x_p) * 0.5f;
  double dy = (y - _mouse_y_p) * 0.5f;
  double delta = dx+dy;

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
  //glClearColor(0.9, 0.9, 0.9, 1.0);
  //glClearColor(0.2, 0.2, 0.2, 1.0);
  glClearColor(0,0,0,1);
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, (GLfloat)width / (GLfloat)height, 10.0, 1000000000.0);
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

static void makeWalls() {
  int height = 800;
  int width = 400;
  int depth = 400;
  GLubyte c_dark[4]  ={50, 50, 50, 0};
  GLubyte c_light[4] = {160, 160, 160, 50};

  wallsList = glGenLists(1);
  glNewList(wallsList, GL_COMPILE);
    glBegin(GL_QUADS);
    glColor4ubv(c_light);
      glVertex3f(width, 0.0f, depth);
      glVertex3f(width, 0.0f, -depth);
    glColor4ubv(c_dark);
      glVertex3f(width, height, -depth);
      glVertex3f(width, height, depth);

    glColor4ubv(c_light);
      glVertex3f(width, 0.0f, depth);
      glVertex3f(-width, 0.0f, depth);
    glColor4ubv(c_dark);
      glVertex3f(-width, height, depth);
      glVertex3f(width, height, depth);

    glColor4ubv(c_light);
      glVertex3f(-width, 0.0f, depth);
      glVertex3f(-width, 0.0f, -depth);
    glColor4ubv(c_dark);
      glVertex3f(-width, height, -depth);
      glVertex3f(-width, height, depth);

    glColor4ubv(c_light);
      glVertex3f(width, 0.0f, -depth);
      glVertex3f(-width, 0.0f, -depth);
    glColor4ubv(c_dark);
      glVertex3f(-width, height, -depth);
      glVertex3f(width, height, -depth);
    glEnd();
  glEndList();
}

static void makeFloor() {
  int width = 400;
  int depth = 400;
  floorList = glGenLists(1);
  glNewList(floorList, GL_COMPILE);
    glBegin(GL_TRIANGLES);
    glColor3ub(150,150,150);
      glVertex3f( width, 0.0f,  depth);
      glVertex3f(-width, 0.0f,  depth);
      glVertex3f(-width, 0.0f, -depth);
      glVertex3f(-width, 0.0f, -depth);
      glVertex3f( width, 0.0f,  depth);
      glVertex3f( width, 0.0f, -depth);
    glEnd();
  glEndList();
}

static void makeGradFloor() {
  int height = 400;
  int width = 400;
  int depth = 400;
  GLubyte c_dark[4]  ={50, 50, 50, 0};
  GLubyte c_light[4] = {130, 130, 130, 127};

  gradFloorList = glGenLists(1);
  glNewList(gradFloorList, GL_COMPILE);
    glBegin(GL_TRIANGLES);
    glColor4ubv(c_light);
    glVertex3f(width, 0.0f, depth);
    glVertex3f(-width, 0.0f, depth);
    glColor4ubv(c_dark);
    glVertex3f(-width, 0.0f, depth + 500.0f);

    glVertex3f(-width, 0.0f, depth + 500.0f);
    glVertex3f(width, 0.0f, depth + 500.0f);
    glColor4ubv(c_light);
    glVertex3f(width, 0.0f, depth);

    glVertex3f(width, 0.0f, depth);
    glVertex3f(width, 0.0f, -depth);
    glColor4ubv(c_dark);
    glVertex3f(width + 500.0f, 0.0f, -depth);

    glVertex3f(width + 500.0f, 0.0f, -depth);
    glVertex3f(width + 500.0f, 0.0f, depth);
    glColor4ubv(c_light);
    glVertex3f(width, 0.0f, depth);

    glVertex3f(-width, 0.0f, -depth);
    glColor4ubv(c_dark);
    glVertex3f(-500-width, 0.0f, -depth);
    glVertex3f(-500-width, 0.0f, depth);

    glVertex3f(-500-width, 0.0f, depth);
    glColor4ubv(c_light);
    glVertex3f(-width, 0.0f, -depth);
    glVertex3f(-width, 0.0f, depth);

    glVertex3f(-width, 0.0f, -depth);
    glColor4ubv(c_dark);
    glVertex3f(-width, 0.0f, -500-depth);
    glVertex3f(width, 0.0f, -500-depth);

    glVertex3f(width, 0.0f, -500-depth);
    glColor4ubv(c_light);
    glVertex3f(-width, 0.0f, -depth);
    glVertex3f(width, 0.0f, -depth);
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
  glutIdleFunc(_step_simulation);
  glutDisplayFunc(_render);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(mouse_move);
  glutPassiveMotionFunc(mouse_move);
  glutReshapeFunc(reshape);
}

//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  srand(time(NULL));
  init_glut(argc, argv);

  init_gui();
  makeAxes();
  makeCrosshair();
  makeFloor();
  makeGradFloor();
  makeWalls();

  init_psys();
  _gui_update_gravtext();

  camera_set_distance(&_camera, 20000);
  camera_set_pitch(&_camera, 0.0f);
  camera_set_yaw(&_camera, 0.0f);

  glEnable(GL_POINT_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDisable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_ALWAYS);

  glutMainLoop();

  return 0;
}
