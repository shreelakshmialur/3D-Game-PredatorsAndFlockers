#ifndef BULLET_UTILS_HH

#define BULLET_UTILS_HH

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <sstream>
#include <iostream>

#include <unistd.h>
#include <sys/times.h>
#include <sys/time.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

using namespace std;
using namespace glm;

//----------------------------------------------------------------------------

int bullet_hello_main(int, char **);

void initialize_bullet_simulator();
void delete_bullet_simulator();

void copy_flocker_states_to_graphics_objects();
void copy_graphics_objects_to_flocker_states();
void update_physics_simulation(float);

void bullet_add_dynamic_objects();

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif
