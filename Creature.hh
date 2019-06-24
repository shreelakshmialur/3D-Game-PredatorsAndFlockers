#ifndef CREATURE_HH

#define CREATURE_HH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <vector>
#include <deque>
#include <string>

#include <sys/time.h>
#include <unistd.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/random.hpp>

using namespace std;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void initialize_random();
double uniform_random(double, double);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class Creature
{
public:

  int index;

  glm::vec3 position;                       // current position
  glm::vec3 velocity;                       // current velocity
  glm::vec3 acceleration;                   // acceleration

  glm::vec3 new_position;                   // what position will be in next time step
  glm::vec3 new_velocity;                   // what velocity will be in next time step

  glm::vec3 up;                             // just like for glm::lookat()
  
  glm::vec3 frame_x;                        // local axes
  glm::vec3 frame_y;
  glm::vec3 frame_z;

  deque <glm::vec3> position_history;
  int max_history;
	
  glm::vec3 base_color;
  glm::vec3 draw_color;

  GLuint vertexbuffer;
  GLuint colorbuffer;

  Creature(int,                    // index
	   double, double, double, // initial position
	   double, double, double, // initial velocity
	   float, float, float,    // base color
	   int = 1);               // number of past states to save

  virtual void draw(glm::mat4) = 0;
  virtual void update() = 0;
  void finalize_update(double, double, double);

};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif
