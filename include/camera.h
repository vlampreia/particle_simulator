#ifndef _CAMERA_H__
#define _CAMERA_H__

#include "vector3f.h"


/**
 * Basic orbiting camera.
 */


struct camera {
  struct vector3f pos;
  double pitch, yaw;
  int distance;
};

void camera_set_yaw       (struct camera *c, double yaw);
void camera_inc_yaw       (struct camera *c, double amount);

void camera_set_pitch     (struct camera *c, double pitch);
void camera_inc_pitch     (struct camera *c, double amount);

void camera_set_distance  (struct camera *c, int distance);
void camera_inc_distance  (struct camera *c, int amount);

void camera_set_position  (struct camera *c, struct vector3f pos);


#endif
