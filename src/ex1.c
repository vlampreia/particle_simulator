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

//------------------------------------------------------------------------------


static struct gui_manager *_guiManager;



static double myRandom(void)
{
  return ((-RAND_MAX/2 + rand())/(double)RAND_MAX);
}

#define NUM_PARTICLES 100
#define NUM_EMITTERS 1
static int _window_width = 800;
static int _window_height = 600;
static struct _text;

struct particle **particle_list;
struct emitter **emitter_list;

//------------------------------------------------------------------------------

static void initialise_particle(struct particle *p) {
  p->mass = 0.2f + myRandom();
  p->velocity.x = 0.0f;//myRandom() * -0.8f;
  p->velocity.y = 0.55f;
  p->velocity.z = -0.1f;

  p->bounce = 0.75f;

  vector3f_init(&p->pos);
  p->tod_usec = 14000;
}

//------------------------------------------------------------------------------

static void initialiseParticles(size_t n) {
  if (particle_list == NULL) {
    particle_list = malloc(sizeof(*particle_list) * n);
  }

  for (int i=0; i<n; ++i) {
    struct particle *p = particle_new();
    //initialise_particle(p);
    particle_list[i] = p;
  }
}

//------------------------------------------------------------------------------

static void update_particle(struct particle *p, long dt) {
  if (p->tod_usec <= 0) {
    p->active = 0;
    return;
  }
  p->tod_usec -= dt;
//  if (p->tod_usec <= global_time.tv_nsec/1000) {
//    p->active = 0;
//    return;
//  }

  /**
   * Here we compute basic gravitational force using euler computations:
   * a = F/m
   * v = v' + a * dT
   * p = p' + v * dT
   *
   * a = acceleration
   * F = force
   * m = mass
   * v = velocity
   * p = position
   */
//  printf(
//    "%lu -- a%s, v%s, vp%s, p%s\n",
//    dt,
//    vector3f_to_str(p->acceleration),
//    vector3f_to_str(p->velocity),
//    vector3f_to_str(p->velocity_p),
//    vector3f_to_str(p->pos)
//  );

  double force = -0.02f * p->mass;
  double friction = 0.995f;

  //force is static down 1 for now
  p->acceleration.x = 0;
  p->acceleration.y = force / p->mass;
  p->acceleration.z = 0;

  //vector3f_normalise(p->acceleration);
  
  p->velocity.x *= friction;
  p->velocity.z *= friction;

  p->velocity.x += p->acceleration.x;
  p->velocity.y += p->acceleration.y;
  p->velocity.z += p->acceleration.z;

  //vector3f_normalise(p->velocity);

  p->pos.x += p->velocity.x * dt;
  p->pos.y += p->velocity.y * dt;
  p->pos.z += p->velocity.z * dt;


  if (p->pos.y <= 0.0f) {
    p->pos.y = 0.0f;
    p->velocity.y = -p->velocity.y * p->bounce;
    p->velocity.z += 0.001 * myRandom();
    p->velocity.x += 0.001 * myRandom();
  }

  if (p->pos.x >= 400.0f) {
    p->pos.x = 400.0f;
    p->velocity.x = -p->velocity.x * p->bounce;
    p->velocity.z += 0.01 * myRandom();
  }
}

//int pgentime = 0;
static void update_particles(int t, int dt) {
  for (int i=0; i<NUM_PARTICLES; ++i) {
    update_particle(particle_list[i], dt);
  }

  //printf("%lu\n", global_time.tv_nsec/1000 - pgentime);
  //if (global_time.tv_nsec/1000 - pgentime > 5) {
  
  // REPLACED BY emitter_step
  //if (t - pgentime > 5) {
  //  for (int i=0; i<NUM_PARTICLES; ++i) {
  //    if (!particle_list[i]->active) {
  //      initialise_particle(particle_list[i]);
  //      particle_list[i]->active = 1;
  //      break;
  //    }
  //  }
  //  //pgentime = global_time.tv_nsec/1000;
  //  pgentime = t;
  //}
}

//------------------------------------------------------------------------------

static int _auto = 0;

void update_emitters(int t, int dt) {
  if (!_auto) return;
  for (int i=0; i<NUM_EMITTERS; ++i) {
    emitter_step(emitter_list[i], t);
  }
}


// Display list for coordinate axis 
GLuint axisList;

int AXIS_SIZE= 200;
static int axisEnabled= 1;

int ptime = 0;
int msq = 0;

//------------------------------------------------------------------------------

static void step_simulation(void) {
  //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &global_time);
  //long dt = global_time.tv_nsec - global_time_p.tv_nsec;
  //global_time_p = global_time;

  int t = glutGet(GLUT_ELAPSED_TIME);
  int dt = t - ptime;
  ptime = t;

  msq += dt;

  //if (dt > 500000) return;

  // 1ns/1k = 1us
  //update_particles(dt/1000);
  while (msq >= 10) {
    //printf("update phys %d, %d\n", dt, msq);
    update_emitters(t, dt);
    update_particles(t, dt);
    msq -= 10;
  }

  glutPostRedisplay();
}

//------------------------------------------------------------------------------

static void render_particles(void) {
  glBegin(GL_POINTS);
    for (int i=0; i<NUM_PARTICLES; ++i) {
      if (!particle_list[i]->active) continue;
      struct vector3f *v = &particle_list[i]->pos;
      glVertex3f(v->x, v->y, v->z);
    }
  glEnd();
}

struct vector3f *_camera_pos;

//------------------------------------------------------------------------------

static void display(void)
{
  glLoadIdentity();
  gluLookAt(_camera_pos->x, _camera_pos->y, _camera_pos->z,
            0.0, 0.0, 0.0,
            0.0, 1.0, 0.0);
  // Clear the screen
  glClear(GL_COLOR_BUFFER_BIT);
  // If enabled, draw coordinate axis
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
    case 102: _auto = !_auto; break;
    case 32: emitter_fire(emitter_list[0]); break;
    default: break;
  }

  glutPostRedisplay();
}

static int _ctrl_mode = 0;

static int _distance = 800;
#define DEG_TO_RAD 0.017453293
static int _mouse_x_p = 0;
static int _mouse_y_p = 0;
static double _yaw = 0.0f;
static double _pitch = 0.0f;
static int _dragging = 0;

//------------------------------------------------------------------------------

static void update_cam() {
  _camera_pos->x = _distance * -sin(_pitch*DEG_TO_RAD) * cos(_yaw*DEG_TO_RAD);
  _camera_pos->y = _distance * -sin(_yaw*DEG_TO_RAD);
  _camera_pos->z = -_distance * cos(_pitch*DEG_TO_RAD) * cos(_yaw*DEG_TO_RAD);
}

//------------------------------------------------------------------------------

static void _zoom(int amount) {
  _distance += amount;
  update_cam();
}

static void myCb(void) {
  emitter_fire(emitter_list[0]);
}

static void _ui_mouse(int x, int y) {
  if (x > 0 && x < 10 && y > 0 && y < 10) {
    emitter_fire(emitter_list[0]);
  }
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

    case 3: _zoom(-10); break;
    case 4: _zoom(10); break;

    case 97: axisEnabled = !axisEnabled; break;

    case 96: _ctrl_mode = 0; break;
    case 49: _ctrl_mode = 1; break;
    case 50: _ctrl_mode = 2; break;
    case 51: _ctrl_mode = 3; break;
    case 52: _ctrl_mode = 4; break;
    case 53: _ctrl_mode = 5; break;
    case 54: _ctrl_mode = 6; break;
    case 55: _ctrl_mode = 7; break;
    case 56: _ctrl_mode = 8; break;
    case 57: _ctrl_mode = 9; break;

    default: break;
  }
}

//------------------------------------------------------------------------------

static void init_gui(void) {
  _guiManager = gui_manager_new();
  gui_manager_set_dimensions(_guiManager, _window_width, _window_height);

  struct gui_element *e1 = gui_element_new(50, 50, 0, 0, "Fire", myCb);
  gui_manager_add_element(_guiManager, e1);
}

//------------------------------------------------------------------------------

static void mouse_move(int x, int y) {
  if (!_dragging) return;

  int dx = (x - _mouse_x_p) * 0.5f;
  int dy = (y - _mouse_y_p) * 0.5f;

  _yaw += dy;
  _pitch += dx;

  if (_yaw >= 89.0f) _yaw = 89.0f;
  if (_yaw <= -89.0f) _yaw = -89.0f;

  update_cam();

  _mouse_x_p = x;
  _mouse_y_p = y;
}

//------------------------------------------------------------------------------

static void mouse_wheel(int wheel, int direction, int x, int y) {
  if (direction > 0) _distance += 10;
  else _distance -= 10;
}

//------------------------------------------------------------------------------

static void reshape(int width, int height)
{
  _window_width = width;
  _window_height = height;
  gui_manager_set_dimensions(_guiManager, width, height);
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

void init_glut(int argc, char *argv[])
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
  //glEnable(GL_LIGHT0);
  //glEnable(GL_COLOR_MATERIAL);
}

//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  _camera_pos = vector3f_new(700.0f, 400.0f, 800.0f);
  particle_list = NULL;
  initialiseParticles(NUM_PARTICLES);


  emitter_list = malloc(sizeof(*emitter_list));
  emitter_list[0] = emitter_new(particle_list);
  emitter_list[0]->orientation = (struct vector3f) {0.2f, 0.8f, 0.1f};
  vector3f_normalise(&emitter_list[0]->orientation);
  emitter_list[0]->force = 0.8f;
  emitter_list[0]->base_particle = particle_new();
  initialise_particle(emitter_list[0]->base_particle);

  emitter_list[0]->frequency = 100;

  srand(time(NULL));
  init_glut(argc, argv);

  init_gui();

  makeAxes();

  glEnable(GL_POINT_SMOOTH);
  //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &global_time_p);
  glutMainLoop();

  return 0;
}
