#include "camera.h"

#include <math.h>

#define DEG_TO_RAD 0.017453293

static void _compute_pos(struct camera *c);

void camera_set_yaw(struct camera *c, double yaw) {
  c->yaw = yaw;
  _compute_pos(c);
}

void camera_inc_yaw(struct camera *c, double amount) {
  c->yaw += amount;
  if (c->yaw >= 89.0f) c->yaw = 89.0f;
  else if (c->yaw <= -89.0f) c->yaw = -89.0f;
  _compute_pos(c);
}

void camera_set_pitch(struct camera *c, double pitch) {
  c->pitch = pitch;
  _compute_pos(c);
}

void camera_inc_pitch(struct camera *c, double amount) {
  c->pitch += amount;
  _compute_pos(c);
}

void camera_set_distance(struct camera *c, int distance) {
  c->distance = distance;
  _compute_pos(c);
}

void camera_inc_distance(struct camera *c, int amount) {
  c->distance += amount;
  if (c->distance < 0) c->distance = 100;
  _compute_pos(c);
}

void camera_set_position(struct camera *c, struct vector3f pos) {
  vector3f_copy(&pos, &c->pos);
}

static void _compute_pos(struct camera *c) {
  c->pos.x =  c->distance * -sin(c->pitch * DEG_TO_RAD) * cos(c->yaw * DEG_TO_RAD);
  c->pos.y =  c->distance * -sin(c->yaw   * DEG_TO_RAD);
  c->pos.z = -c->distance *  cos(c->pitch * DEG_TO_RAD) * cos(c->yaw * DEG_TO_RAD);
}
