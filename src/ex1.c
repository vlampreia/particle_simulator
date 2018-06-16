#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <GL/glut.h>

#include "vertex.h"
#include "emitter.h"
#include "particle.h"
#include "attractor.h"

#include "gui_manager.h"
#include "gui_element.h"

#include "particle_system.h"
#include "camera.h"

//------------------------------------------------------------------------------

#define KB_PERIOD 46
#define KB_ESC 27
#define KB_SPACE 32
#define KB_0 48
#define KB_1 49
#define KB_2 50
#define KB_3 51
#define KB_4 52
#define KB_5 53
#define KB_6 54
#define KB_7 55
#define KB_8 56
#define KB_9 57
#define KB_A 65
#define KB_B 66
#define KB_C 67
#define KB_D 68
#define KB_E 69
#define KB_F 70
#define KB_G 71
#define KB_H 72
#define KB_I 73
#define KB_J 74
#define KB_K 75
#define KB_L 76
#define KB_M 77
#define KB_N 78
#define KB_O 79
#define KB_P 80
#define KB_Q 81
#define KB_R 82
#define KB_S 83
#define KB_T 84
#define KB_U 85
#define KB_V 86
#define KB_W 87
#define KB_X 88
#define KB_Y 89
#define KB_Z 90
#define KB_a 97
#define KB_b 98
#define KB_c 99
#define KB_d 100
#define KB_e 101
#define KB_f 102
#define KB_g 103
#define KB_h 104
#define KB_i 105
#define KB_j 106
#define KB_k 107
#define KB_l 108
#define KB_m 109
#define KB_n 110
#define KB_o 111
#define KB_p 112
#define KB_q 113
#define KB_r 114
#define KB_s 115
#define KB_t 116
#define KB_u 117
#define KB_v 118
#define KB_w 119
#define KB_x 120
#define KB_y 121
#define KB_z 122

//------------------------------------------------------------------------------


#define NSIGN(x) ((x > 0) - (x < 0))

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
#define NUM_PARTICLES 1000000
//#define NUM_PARTICLES 300000
#define NUM_EMITTERS 2
#define NSEC_DIV 1000000 / 1000

static int _window_width = 800;
static int _window_height = 600;

static struct vertex _mouseWorldPos =  {0.0, 0.0, 0.0};

#define CTRL_CAT_NONE -1
#define CTRL_CAT_EMITTER 1
#define CTRL_CAT_ATTRACTOR 2
#define CTRL_CAT_MAXIDX 3
static int _ctrl_cat  = CTRL_CAT_NONE;
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
static int _draw_attractors = 0;

int draw_mode = GL_POINTS;

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
GLuint terrainList;

int AXIS_SIZE= 200;
static int axisEnabled= 1;

int msq = 0;


static void _toggleEdit(void) {
  _ctrl_edit = !_ctrl_edit;
  if (!_ctrl_edit) glutSetCursor(GLUT_CURSOR_INHERIT);
}

//------------------------------------------------------------------------------

/* main UI */

static struct gui_element *txt_ctrl_mode_header = NULL;
static struct gui_element *txt_ctrl_mode_element = NULL;


static struct gui_element *txt_fps      = NULL;
static struct gui_element *txt_mspf     = NULL;
static struct gui_element *txt_sfps     = NULL;
static struct gui_element *txt_smspf    = NULL;
static struct gui_element *txt_pcount   = NULL;

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
  if (mode == CTRL_CAT_NONE) {
    _ctrl_cat = CTRL_CAT_NONE;
    _ctrl_mode = 0;
    snprintf(_gui_buffer, 256, "edit none");
    gui_element_set_str(txt_ctrl_mode_header, _gui_buffer, 0);
    gui_element_set_visible(btn_autoFire, 0);
    gui_element_set_visible(txt_mass,   0);
    gui_element_set_visible(btn_decMass, 0);
    gui_element_set_visible(btn_incMass, 0);
    gui_element_set_visible(txt_rate,   0);
    gui_element_set_visible(txt_bounce, 0);
    gui_element_set_visible(txt_force,  0);
    gui_element_set_visible(btn_type,   0);
    gui_element_set_visible(btn_decBounce, 0);
    gui_element_set_visible(btn_incBounce, 0);
    gui_element_set_visible(btn_decRate, 0);
    gui_element_set_visible(btn_incRate, 0);
  } else if (_ctrl_cat == CTRL_CAT_NONE && mode < CTRL_CAT_MAXIDX) {
    _ctrl_cat = mode;
    gui_element_set_visible(btn_autoFire, 1);
    gui_element_set_visible(txt_mass,   1);
    gui_element_set_visible(btn_decMass, 1);
    gui_element_set_visible(btn_incMass, 1);
    if (_ctrl_cat == CTRL_CAT_EMITTER) {
      snprintf(_gui_buffer, 256, "edit emitter");
      gui_element_set_visible(txt_rate,   1);
      gui_element_set_visible(txt_bounce, 1);
      gui_element_set_visible(txt_force,  1);
      gui_element_set_visible(btn_type,   1);
      gui_element_set_visible(btn_decBounce, 1);
      gui_element_set_visible(btn_incBounce, 1);
      gui_element_set_visible(btn_decRate, 1);
      gui_element_set_visible(btn_incRate, 1);
    } else if (_ctrl_cat == CTRL_CAT_ATTRACTOR) {
      snprintf(_gui_buffer, 256, "edit attractor");
      gui_element_set_visible(txt_rate,   0);
      gui_element_set_visible(txt_bounce, 0);
      gui_element_set_visible(txt_force,  0);
      gui_element_set_visible(btn_type,   0);
      gui_element_set_visible(btn_decBounce, 0);
      gui_element_set_visible(btn_incBounce, 0);
      gui_element_set_visible(btn_decRate, 0);
      gui_element_set_visible(btn_incRate, 0);
    }
    gui_element_set_str(txt_ctrl_mode_header, _gui_buffer, 0);
    _ctrl_mode = 0;
  } else if (_ctrl_cat != CTRL_CAT_NONE){
    if (_ctrl_cat == CTRL_CAT_ATTRACTOR && _pSystem->attractors->size-1 < mode) return;
    if (_ctrl_cat == CTRL_CAT_EMITTER && _pSystem->emitters->size-1 < mode) return;
    _ctrl_mode = mode;
  }

  snprintf(_gui_buffer, 256, "%d", _ctrl_mode);
  gui_element_set_str(txt_ctrl_mode_element, _gui_buffer, 0);

  char *str = malloc(15);

  if (_ctrl_cat == CTRL_CAT_EMITTER) {
    struct emitter *e = _pSystem->emitters->elements[_ctrl_mode];
    snprintf(str, 11, "Rate: %f", e->frequency);
    gui_element_set_str(txt_rate, str, 0);

    snprintf(str, 15, "Bounce: %f", e->base_particle->bounce);
    gui_element_set_str(txt_bounce, str, 0);

    snprintf(str, 11, "Mass: %f", e->base_particle->mass);
    gui_element_set_str(txt_mass, str, 0);

    snprintf(str, 11, "Force: %f", e->force);
    gui_element_set_str(txt_force, str, 0);

    snprintf(_gui_buffer, 256, "Enabled: %d", e->firing);
    gui_element_set_str(btn_autoFire, _gui_buffer, 0);
  } else if (_ctrl_cat == CTRL_CAT_ATTRACTOR) {
    struct attractor *a = _pSystem->attractors->elements[_ctrl_mode];
    snprintf(_gui_buffer, 256, "Mass: %ld", a->mass);
    gui_element_set_str(txt_mass, _gui_buffer, 0);
    snprintf(_gui_buffer, 256, "Enabled: %d", a->enabled);
    gui_element_set_str(btn_autoFire, _gui_buffer, 0);
  }

  free(str);
}

//------------------------------------------------------------------------------

static void initialise_particle(struct particle *p) {
  //p->mass = 0.5f + myRandom();
  p->mass = 40;
  p->velocity.x = 0.0f;//myRandom() * -0.8f;
  p->velocity.y = 0.0f;
  p->velocity.z = 0.0f;

  p->bounce = 0.6f;

  //p->pos[0] = 0.0f;
  //p->pos[1] = 0.0f;
  //p->pos[2] = 0.0f;
  //vector3f_init(&p->pos);
  //p->tod_usec = 24000;
  p->tod_usec = 300;
  p->tod_max = p->tod_usec;

  //p->color[0] = 50;
  //p->color[1] = 50;
  //p->color[2] = 50;
  //p->color[3] = 255;

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

  _statistics.lastSimDurations[_statistics.sdi] = dt;
  _statistics.lastSimDuration = _avg(_statistics.lastSimDurations, _statistics.maxidx);
  _statistics.lastSimTime = newTime;

  if (dt > 0.15) dt = 0.15;
  if (_statistics.sdi == _statistics.maxidx-1) _statistics.sdi = 0;
  else ++_statistics.sdi;


  int active = 0;
  if (_do_phys) {
    accumulator += dt;

    //TODO: FIX TIMESTEP -- is idlefunc doing some dt shit??
    while (accumulator >= dt) {
      active = particle_system_step(_pSystem, t, 1);//1.0);
      accumulator -= dt;
      t += dt;
    }

    snprintf(_gui_buffer, 256, "Particles: %d", active);
    gui_element_set_str(txt_pcount, _gui_buffer, 1);
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
//static int count = NUM_PARTICLES/10;
static inline void render_particles(void) {
  int active = 0;

  glPointSize(1.0f);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, _pSystem->particle_pos);
  glColorPointer(4, GL_UNSIGNED_BYTE, 0, _pSystem->particle_col);
  glDrawArrays(draw_mode, 0, NUM_PARTICLES);
  //glDrawRangeElements(GL_POINTS, 0, count, count, GL_UNSIGNED_BYTE, _pSystem->particle_idx);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);


  //snprintf(_gui_buffer, 256, "Particles: %d", active);
  //gui_element_set_str(txt_pcount, _gui_buffer, 1);
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

  snprintf(_gui_buffer, 256, "RND MSPF: %f", _statistics.lastRenderDuration);
  gui_element_set_str(txt_mspf, _gui_buffer, 0);

  snprintf(_gui_buffer, 256, "RND FPS: %f", 1000/(_statistics.lastRenderDuration+0.0001f));
  gui_element_set_str(txt_fps, _gui_buffer, 0);

  snprintf(_gui_buffer, 256, "SIM MSPF: %f", _statistics.lastSimDuration*1000);
  gui_element_set_str(txt_smspf, _gui_buffer, 0);

  snprintf(_gui_buffer, 256, "SIM FPS: %f", 1/(_statistics.lastSimDuration+0.001f));
  gui_element_set_str(txt_sfps, _gui_buffer, 0);

  snprintf(_gui_buffer, 256, "air: %f", _pSystem->air_density);
  gui_element_set_str(txt_airDens, _gui_buffer, 0);

  snprintf(_gui_buffer, 256, "friction: %f", _pSystem->friction);
  gui_element_set_str(txt_friction, _gui_buffer, 0);

  glLoadIdentity();
  gluLookAt(_camera.pos.x, _camera.pos.y, _camera.pos.z,
            0.0, 0.0,0.0,//_camera.distance/8, 0.0,
            0.0, 1.0, 0.0);

  // Clear the screen
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  double x,y,z;
  GLfloat _z;


  if (_ctrl_edit) {
    glCallList(floorList);
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

  if (_drawFloor) glCallList(gradFloorList);

  if (_ctrl_edit) {
    _mouseWorldPos.x = x;
    _mouseWorldPos.y = y;
    _mouseWorldPos.z = z;

    if(axisEnabled) glCallList(axisList);

    //draw emitters
    for (size_t i=0; i<_pSystem->emitters->size; ++i) {
      struct emitter *e = _pSystem->emitters->elements[i];

      struct vertex_col *col = &e->base_particle->base_color;

      glPointSize(10.0f);
      if (i == _ctrl_mode && _ctrl_cat == CTRL_CAT_EMITTER) glColor3ub(col->r, col->g, col->b);
      else glColor3ub(col->r/2, col->g/2, col->b/2);

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
      if (i == _ctrl_mode) glColor4ub(col->r, col->g, col->b, 200);
      else glColor4ub(col->r/2, col->g/2, col->b/2, 200);

      glBegin(GL_TRIANGLES);
        glVertex3f(e->position.x, e->position.y, e->position.z);
        glVertex3f(angle.x, angle.y, angle.z);
        glVertex3f(angle.x, e->position.y, angle.z);
        glVertex3f(e->position.x, e->position.y, e->position.z);
      glEnd();
    }

    //draw attractors
    for (size_t i=0; i<_pSystem->attractors->size; ++i) {
      struct attractor *a = _pSystem->attractors->elements[i];

      size_t idx = i*4;
      double m = a->mass;
      double c = 1-m/10;
      double mid = 0.0f;
      if (i == _ctrl_mode && _ctrl_cat == CTRL_CAT_ATTRACTOR) mid = 1.0;
      if (m > 0) glColor4f(0, mid, 1-m/10000000000.0, 0.5);
      else       glColor4f(        1-m/10000000000.0, mid, 0, 0.3);
      glPointSize(NSIGN(m) * m /100000000);
      glBegin(GL_POINTS);
        glVertex3f(a->pos.x, a->pos.y, a->pos.z);
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

  if (_draw_attractors) {
  }

  if (_drawWalls) {
    glCallList(wallsList);
  }

  glCallList(terrainList);

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
    case KB_ESC: clean_exit(); break;

    case KB_f:
      if (_ctrl_cat == CTRL_CAT_EMITTER) {
        ((struct emitter*)_pSystem->emitters->elements[_ctrl_mode])->firing = !((struct emitter*)_pSystem->emitters->elements[_ctrl_mode])->firing;
      } else if (_ctrl_cat == CTRL_CAT_ATTRACTOR) {
        ((struct attractor*) _pSystem->attractors->elements[_ctrl_mode])->enabled = !((struct attractor*)_pSystem->attractors->elements[_ctrl_mode])->enabled;
      } else {
        break;
      }
      _set_ctrl_mode(_ctrl_mode);
      break;

    //case 32: emitter_fire(_pSystem->emitters->elements[_ctrl_mode], 1); break;

    case KB_a: axisEnabled = !axisEnabled; break;

    case KB_q: _set_ctrl_mode(CTRL_CAT_NONE); break;
    case KB_1: _set_ctrl_mode(1); break;
    case KB_2: _set_ctrl_mode(2); break;
    case KB_3: _set_ctrl_mode(3); break;
    case KB_4: _set_ctrl_mode(4); break;
    case KB_5: _set_ctrl_mode(5); break;
    case KB_6: _set_ctrl_mode(6); break;
    case KB_7: _set_ctrl_mode(7); break;
    case KB_8: _set_ctrl_mode(8); break;
    case KB_9: _set_ctrl_mode(9); break;
    case KB_0: _set_ctrl_mode(0); break;

    case KB_w: _drawWalls = !_drawWalls; break;
    case KB_W: _drawFloor = !_drawFloor; break;

    case KB_A: _pSystem->friction += 0.01f; break;
    case KB_S: _pSystem->friction -= 0.01f; break;
    case KB_D: _pSystem->air_density += 0.01f; break;
    case KB_F: _pSystem->air_density -= 0.01f; break;

    case KB_c: _pSystem->collideWalls = !_pSystem->collideWalls; break;
    case KB_C: _pSystem->collideFloor = !_pSystem->collideFloor; break;

    case KB_u: _draw_ui = !_draw_ui; break;
    case KB_e: _toggleEdit(); break;

    case KB_SPACE: _do_phys = !_do_phys; break;

    case KB_Q: _draw_attractors = !_draw_attractors; break;

    case KB_l: draw_mode = GL_LINES; break;
    case KB_p: draw_mode = GL_POINTS; break;

    case KB_PERIOD: {
      double step = 1.0f;
      int active = particle_system_step(_pSystem, t, step);
      t+= step;
      snprintf(_gui_buffer, 256, "Particles: %d", active);
      gui_element_set_str(txt_pcount, _gui_buffer, 1);
      break;
    }

    case KB_t: _pSystem->trip = !_pSystem->trip; break;

    case KB_x: particle_system_reset(_pSystem); break;

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
      if (_move_emitter && _ctrl_edit_mode == 0) {
        if (_ctrl_cat == CTRL_CAT_EMITTER) {
          vertex_copy(
            &_mouseWorldPos,
            &((struct emitter*)_pSystem->emitters->elements[_ctrl_mode])->position
          );
        } else if (_ctrl_cat == CTRL_CAT_ATTRACTOR) {
          vertex_copy(
            &_mouseWorldPos,
            &((struct attractor*)_pSystem->attractors->elements[_ctrl_mode])->pos
          );
        }
      }

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

  particle_system_add_attractor(_pSystem, attractor_new(
      (struct vertex) {1, -5000, 800},
      5000000000
  ));

  particle_system_add_attractor(_pSystem, attractor_new(
      (struct vertex) {-10000, -5000, 800},
      3000000000
  ));

  particle_system_add_attractor(_pSystem, attractor_new(
      (struct vertex) {1, 5000, -4100},
      4000000000
  ));

  particle_system_add_attractor(_pSystem, attractor_new(
      (struct vertex) {10000, 5000, 8000},
      -700000000
  ));


  struct emitter *e1 = emitter_new(NULL);
  //e1->orientation = (struct vector3f) {0.2f, 0.8f, 0.1f};
  //vector3f_normalise(&e1->orientation);
  e1->pitch = 0.0f;
  e1->force = 700.8f;
  e1->yaw = 90.0f;
  e1->base_particle = particle_new(e1, -1);
  e1->position = (struct vertex) {0.0f, 10000.0f, 0.0f};
  initialise_particle(e1->base_particle);
  e1->base_particle->base_color.r = 0;
  e1->base_particle->base_color.g = 255;
  e1->base_particle->base_color.b = 0;
  e1->base_particle->base_color.a = 255;
  //e1->base_particle->color = (struct vector3f){0.0f, 1.0f, 0.0f};
  e1->base_particle->collision_chaos = 1.4f;
  e1->base_particle->mass = 1.0f;
  e1->base_particle->bounce = 0.99f;
  e1->base_particle->tod_usec = 5000;
  e1->base_particle->tod_max = e1->base_particle->tod_usec;
  e1->frequency = 0.0;
  e1->horiz_angle = 0;
  e1->vert_angle = 0;
  particle_system_add_emitter(_pSystem, e1);

  for (int i=0; i<7; ++i) {
    struct emitter *e = emitter_new(NULL);
    e->pitch = 20.0f + i*10;
    e->yaw += i*40;
    //e->orientation = (struct vector3f) {-1.0f * myRandom(), 1.0f - 0.8f *myRandom(), 1.0f * myRandom()};
    //vector3f_normalise(&e->orientation);
    e->force = 10.0f + i * 5.0f;
    //e->force = 45.8f  - 1.5f * myRandom();
    e->base_particle = particle_new(e, -1);
    initialise_particle(e->base_particle);
    e->position = (struct vertex) {-500.0f*i, 500.0f, i*500.0f};
    e->base_particle->base_color.r = 255;
    e->base_particle->base_color.g = 40 * i;
    e->base_particle->base_color.b = 20 * i;
    e->base_particle->base_color.a = 255;
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
  e2->position = (struct vertex) {-50.0f, 500.0f, 50.0f};
  //e2->orientation = (struct vector3f) {1.0f, 0.5f, 1.0f};
  e2->pitch = 0.0f;
  e2->yaw = 0.0f;
  e2->horiz_angle = 720.0f;
  e2->vert_angle = 720.0f;
  //vector3f_normalise(&e2->orientation);
  e2->force = 20;//60.01f;
  e2->base_particle = particle_new(e2, -1);
  initialise_particle(e2->base_particle);
  e2->base_particle->base_color.r = 255;
  e2->base_particle->base_color.g = 115;
  e2->base_particle->base_color.b = 205;
  e2->base_particle->base_color.a = 75;
  e2->base_particle->mass = 0.8f;
  e2->base_particle->bounce = 0.9f;
  e2->base_particle->size = 3.0f;
  e2->base_particle->tod_usec = 5000;
  e2->base_particle->tod_max = e2->base_particle->tod_usec;
  //e2->base_particle->color = (struct vector3f){0.0f, 1.0f, 1.0f};
  e2->base_particle->collision_chaos = 0.01f;
  e2->frequency = 1;
  e2->emission_count = 100000;
  particle_system_add_emitter(_pSystem, e2);

  e2 = emitter_new(NULL);
  //e2->position = (struct vector3f) {-5000.0f, 5000.0f, 5000.0f};
  e2->position = (struct vertex) {0,0.1,0};
  //e2->position = (struct vector3f) {0.0f, 800000.0f, 0.0f};
  //e2->orientation = (struct vector3f) {1.0f, 0.5f, 1.0f};
  e2->pitch = 0.0f;
  e2->yaw = 0.0f;
//  e2->horiz_angle = 720.0f;
  e2->vert_angle = 360.0f;
  e2->horiz_angle = 360.0f;
  //vector3f_normalise(&e2->orientation);
  e2->force = 50;//60.01f;
  e2->base_particle = particle_new(e2, -1);
  initialise_particle(e2->base_particle);
  e2->base_particle->base_color.r = 255;
  e2->base_particle->base_color.g = 165;
  e2->base_particle->base_color.b = 15;
  e2->base_particle->base_color.a = 60;
  e2->base_particle->mass = 0.8f;
  e2->base_particle->bounce = 0.9f;
  e2->base_particle->size = 3.0f;
  //e2->base_particle->color = (struct vector3f){0.0f, 1.0f, 1.0f};
  e2->base_particle->collision_chaos = 0.01f;
  e2->frequency = 0.0;
  e2->emission_count = 500;
  e2->base_particle->tod_usec = 10000;
  e2->base_particle->tod_max = e2->base_particle->tod_usec;
  particle_system_add_emitter(_pSystem, e2);
}

//------------------------------------------------------------------------------

static void _cb_incMass(void) {
  if (_ctrl_cat == CTRL_CAT_EMITTER) {
    ((struct emitter*)_pSystem->emitters->elements[_ctrl_mode])->base_particle->mass += 100;
  } else if (_ctrl_cat == CTRL_CAT_ATTRACTOR) {
    ((struct attractor*)_pSystem->attractors->elements[_ctrl_mode])->mass += 100000000;
  }

  _set_ctrl_mode(_ctrl_mode);
}

static void _cb_decMass(void) {
  if (_ctrl_cat == CTRL_CAT_EMITTER) {
    ((struct emitter*)_pSystem->emitters->elements[_ctrl_mode])->base_particle->mass -= 100;
  } else if (_ctrl_cat == CTRL_CAT_ATTRACTOR) {
    ((struct attractor*)_pSystem->attractors->elements[_ctrl_mode])->mass -= 100000000;
  }
  _set_ctrl_mode(_ctrl_mode);
}

static void _cb_enable(void) {
  if (_ctrl_cat == CTRL_CAT_EMITTER) {
    ((struct emitter*)_pSystem->emitters->elements[_ctrl_mode])->firing = !((struct emitter*)_pSystem->emitters->elements[_ctrl_mode])->firing;
  } else if (_ctrl_cat == CTRL_CAT_ATTRACTOR) {
    ((struct attractor*)_pSystem->attractors->elements[_ctrl_mode])->enabled = !((struct attractor*)_pSystem->attractors->elements[_ctrl_mode])->enabled;
  }
  _set_ctrl_mode(_ctrl_mode);
}

static void init_gui(void) {
  _guiManager = gui_manager_new();
  gui_manager_set_dimensions(_guiManager, _window_width, _window_height, _window_height, 10);

  gui_manager_new_element(_guiManager, "Statistics", 600, 0, NULL);
  gui_manager_new_element(_guiManager, NULL,0,0,NULL);
  txt_fps    = gui_manager_new_element(_guiManager, "RND FPS: 1000",  0,0, NULL);
  txt_mspf   = gui_manager_new_element(_guiManager, "RND MSPF: 1000", 0,0, NULL);
  txt_sfps   = gui_manager_new_element(_guiManager, "SIM FPS: 1000",  0,0, NULL);
  txt_smspf  = gui_manager_new_element(_guiManager, "SIM MSPF: 1000", 0,0, NULL);
  txt_pcount = gui_manager_new_element(_guiManager, "Particles: 1000000", 0,0, NULL);

  gui_manager_new_element(_guiManager,NULL,0,0,NULL);
  txt_grav     = gui_manager_new_element(_guiManager, "Gravity 0.0000",   0,0, NULL);
  btn_decG     = gui_manager_new_element(_guiManager, "-",                0,0, _cb_decGrav);
  btn_incG     = gui_manager_new_element(_guiManager, "+",                0,0, _cb_incGrav);
  txt_airDens  = gui_manager_new_element(_guiManager, "air: 0.0000",      0,0, NULL);
  txt_friction = gui_manager_new_element(_guiManager, "friction: 0.0000", 0,0, NULL);

  gui_manager_new_element(_guiManager,NULL,0,0,NULL);
  gui_manager_new_element(_guiManager,NULL,0,0,NULL);
  txt_ctrl_mode_header = gui_manager_new_element(_guiManager, "edit none", 542,0,NULL);
  txt_ctrl_mode_element = gui_manager_new_element(_guiManager, "", 50,0,NULL);
  gui_manager_new_element(_guiManager,NULL,0,0,NULL);
  btn_autoFire  = gui_manager_new_element(_guiManager, "Enabled: 0",    0,0, _cb_enable);
  txt_mass      = gui_manager_new_element(_guiManager, "Mass: 1.0000000000",    0,0, NULL);
  btn_decMass   = gui_manager_new_element(_guiManager, "-",             0,0, _cb_decMass);
  btn_incMass   = gui_manager_new_element(_guiManager, "+",             0,0, _cb_incMass);
  txt_rate      = gui_manager_new_element(_guiManager, "Rate: 1000", 0,0, NULL);
  btn_decRate   = gui_manager_new_element(_guiManager, "-",          0,0, NULL);
  btn_incRate   = gui_manager_new_element(_guiManager, "+",          0,0, NULL);

  gui_manager_new_element(_guiManager,NULL,0,0,NULL);
  btn_type      = gui_manager_new_element(_guiManager, "GL_POINT",      0,0, NULL);
  txt_bounce    = gui_manager_new_element(_guiManager, "Bounce: 0.000", 0,0, NULL);
  btn_decBounce = gui_manager_new_element(_guiManager, "-",             0,0, NULL);
  btn_incBounce = gui_manager_new_element(_guiManager, "+",             0,0, NULL);
  txt_force     = gui_manager_new_element(_guiManager, "Force: 000.0",  0,0, NULL);

  _set_ctrl_mode(CTRL_CAT_NONE);
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
      if (_ctrl_cat == CTRL_CAT_EMITTER) {
        vertex_copy(&_mouseWorldPos, &e->position);
      } else if (_ctrl_cat == CTRL_CAT_ATTRACTOR) {
        vertex_copy(&_mouseWorldPos, &((struct attractor*)_pSystem->attractors->elements[_ctrl_mode])->pos);
      }
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
  gui_manager_set_dimensions(_guiManager, width, height, height-30, 10);
  //glClearColor(0.9, 0.9, 0.9, 1.0);
  //glClearColor(0.2, 0.2, 0.2, 1.0);
  glClearColor(0.06,0.06,0.10,1);
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, (GLfloat)width / (GLfloat)height, 10.0, 1000000000.0);
  glMatrixMode(GL_MODELVIEW);
}

//------------------------------------------------------------------------------

struct QuadTree {
  struct QuadTree *topLeft;
  struct QuadTree *topRight;
  struct QuadTree *bottomLeft;
  struct QuadTree *bottomRight;

  struct QuadTree *top;
  struct QuadTree *bottom;
  struct QuadTree *left;
  struct QuadTree *right;

  struct vertex pos_topLeft;
  struct vertex pos_topRight;
  struct vertex pos_bottomLeft;
  struct vertex pos_bottomRight;
};

struct QuadTree *QuadTree_new(
  struct vertex topleft,
  struct vertex topright,
  struct vertex bottomleft,
  struct vertex bottomright
) {
  struct QuadTree *qt = malloc(sizeof(*qt));
  if (!qt) return NULL;

  qt->top = NULL;
  qt->bottom = NULL;
  qt->left = NULL;
  qt->right = NULL;

  qt->topLeft = NULL;
  qt->topRight = NULL;
  qt->bottomLeft = NULL;
  qt->bottomRight = NULL;

  qt->pos_topLeft = topleft;
  qt->pos_topRight = topright;
  qt->pos_bottomLeft = bottomleft;
  qt->pos_bottomRight = bottomright;

  return qt;
}

static void subdivide_num(struct QuadTree *qt, int num) {
  if (num == 0) return;

  float horizMid = qt->pos_topLeft.x +
                  (qt->pos_topRight.x - qt->pos_topLeft.x)/2;

  float vertMid = qt->pos_topLeft.z +
                  (qt->pos_bottomLeft.z - qt->pos_topLeft.z)/2;

  /*
   * If we have adjacent blocks, we will take their midpoints if they have been
   * subdivided, otherwise, use random offset.
   *
   *
   * EDIT: TODO: this won't work if we subdivide more than once as we don't
   * drill down into deepr subdivisions....
   */

  float midYt = myRandom()*1000/(num*2);
  if (qt->top) {
    if (qt->top->bottomLeft) midYt = qt->top->bottomLeft->pos_bottomRight.y;
    else if (qt->top->bottomRight) midYt = qt->top->bottomRight->pos_bottomLeft.y;
  }

  float midYb = myRandom()*1000/(num*2);
  if (qt->bottom) {
    if (qt->bottom->topLeft) midYb = qt->bottom->topLeft->pos_topRight.y;
    else if (qt->bottom->topRight) midYb = qt->bottom->topRight->pos_topLeft.y;
  }

  float midYl = myRandom()*1000/(num*2);
  if (qt->left) {
    if (qt->left->topRight) midYl = qt->left->topRight->pos_bottomRight.y;
    else if (qt->left->bottomRight) midYl = qt->left->bottomRight->pos_topRight.y;
  }

  float midYr = myRandom()*1000/(num*2);
  if (qt->right) {
    if (qt->right->topLeft) midYr = qt->right->topLeft->pos_bottomLeft.y;
    else if (qt->right->bottomLeft) midYr = qt->right->bottomLeft->pos_topLeft.y;
  }

  float midYc = myRandom()*1000/(num*2);

  qt->topLeft     = QuadTree_new(
    qt->pos_topLeft,
    (struct vertex) {horizMid, midYt, qt->pos_topRight.z},
    (struct vertex) {qt->pos_topLeft.x, midYl, vertMid},
    (struct vertex) {horizMid, midYc, vertMid}
  );

  qt->topRight    = QuadTree_new(
    (struct vertex) {horizMid, midYt, qt->pos_topRight.z},
    qt->pos_topRight,
    (struct vertex) {horizMid, midYc, vertMid},
    (struct vertex) {qt->pos_topRight.x, midYr, vertMid}
  );

  qt->bottomLeft  = QuadTree_new(
    (struct vertex) {qt->pos_bottomLeft.x, midYl, vertMid},
    (struct vertex) {horizMid, midYc, vertMid},
    qt->pos_bottomLeft,
    (struct vertex) {horizMid, midYb, qt->pos_bottomLeft.z}
  );

  qt->bottomRight = QuadTree_new(
    (struct vertex) {horizMid, midYc, vertMid},
    (struct vertex) {qt->pos_bottomRight.x, midYr, vertMid},
    (struct vertex) {horizMid, midYb, qt->pos_bottomRight.z},
    qt->pos_bottomRight
  );

  qt->topLeft->right = qt->topRight;
  qt->topLeft->bottom = qt->bottomLeft;
  qt->topRight->left = qt->topLeft;
  qt->topRight->bottom = qt->bottomRight;
  qt->bottomLeft->top = qt->topLeft;
  qt->bottomLeft->right = qt->bottomRight;
  qt->bottomRight->left = qt->bottomLeft;
  qt->bottomRight->top = qt->topRight;

  subdivide_num(qt->topLeft, num-1);
  subdivide_num(qt->topRight, num-1);
  //subdivide_num(qt->bottomLeft, num-1);
  //subdivide_num(qt->bottomRight, num-1);
}

static void _glVertex(struct vertex v) {
  glVertex3f(v.x, v.y, v.z);
}

static void render_qt(struct QuadTree *qt) {
  float horizMid = qt->pos_topLeft.x +
                  (qt->pos_topRight.x - qt->pos_topLeft.x)/2;

  float vertMid = qt->pos_topLeft.z +
                  (qt->pos_bottomLeft.z - qt->pos_topLeft.z)/2;

  if (qt->topLeft) {
    render_qt(qt->topLeft);
  } else {
    _glVertex(qt->pos_topLeft);
    if (qt->topRight) _glVertex(qt->topRight->pos_topLeft);
    else _glVertex(qt->pos_topRight);

    _glVertex(qt->pos_topLeft);
    if (qt->bottomLeft) _glVertex(qt->bottomLeft->pos_topLeft);
    else _glVertex(qt->pos_bottomLeft);
  }

  if (!qt->topRight && !qt->right) {
    _glVertex(qt->pos_topRight);
    _glVertex(qt->pos_bottomRight);
  }

  if (!qt->bottomRight && !qt->bottom) {
    _glVertex(qt->pos_bottomLeft);
    _glVertex(qt->pos_bottomRight);
  }

  if (qt->topRight) render_qt(qt->topRight);
  if (qt->bottomLeft) render_qt(qt->bottomLeft);
  if (qt->bottomRight) render_qt(qt->bottomRight);
}


static void makeTerrain(int iterations) {
  terrainList = glGenLists(1);

  int width = 5000;
  int height = 5000;

  struct QuadTree *terrain = QuadTree_new(
    (struct vertex) {-width, 0, -height},
    (struct vertex) { width, 0, -height},
    (struct vertex) {-width, 0,  height},
    (struct vertex) { width, 0,  height}
  );

  subdivide_num(terrain, iterations);

  glNewList(terrainList, GL_COMPILE);
    glLineWidth(2.0f);
    glColor3f(1.0, 1.0, 1.0);

    glBegin(GL_LINES);
      render_qt(terrain);
    glEnd();
  glEndList();
}


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
  int width = 100000;
  crosshairList = glGenLists(1);
  glNewList(crosshairList, GL_COMPILE);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
      glColor4f(0.5f, 0.5f, 0.5f, 0.0f);
      glVertex3f(-width, 0.0f,  0.0f);

      glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
      glVertex3f( 0.0f,     0.0f,  0.0f);
      glVertex3f( 0.0f,     0.0f,  0.0f);

      glColor4f(0.5f, 0.5f, 0.5f, 0.0f);
      glVertex3f( width, 0.0f,  0.0f);

      glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
      glVertex3f( 0.0f,     0.0f,  0.0f);

      glColor4f(0.5f, 0.5f, 0.5f, 0.0f);
      glVertex3f( 0.0f,     0.0f, -width);
      glVertex3f( 0.0f,     0.0f,  width);

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
  int width = 1000000;
  int depth = 1000000;
  floorList = glGenLists(1);
  glNewList(floorList, GL_COMPILE);
    glColor4ub(0,0,0,0);
    glBegin(GL_TRIANGLES);
      glVertex3f( width, 0.0f,  depth);
      glVertex3f(-width, 0.0f,  depth);
      glVertex3f(-width, 0.0f, -depth);
      glVertex3f(-width, 0.0f, -depth);
      glVertex3f( width, 0.0f,  depth);
      glVertex3f( width, 0.0f, -depth);
    glEnd();

    glColor4ub(200, 200, 200, 200);
    glBegin(GL_LINE_LOOP);
      glVertex3f(-width, 0.0f, -depth);
      glVertex3f( width, 0.0f, -depth);
      glVertex3f( width, 0.0f,  depth);
      glVertex3f(-width, 0.0f,  depth);
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
  makeTerrain(4);

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
