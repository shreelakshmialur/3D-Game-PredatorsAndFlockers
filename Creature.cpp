#include "Creature.hh"

//----------------------------------------------------------------------------
extern int flockNumber;
//----------------------------------------------------------------------------

// set seed based on time so no two runs are alike
void initialize_random()
{
  struct timeval tp;

  gettimeofday(&tp, NULL);
  srand48(tp.tv_sec);
}

//----------------------------------------------------------------------------

// random float in range [lower, upper]

double uniform_random(double lower, double upper)
{
  double result;
  double range_size;

  range_size = upper - lower;
  result = range_size * drand48();
  result += lower;

  return result;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// construct a generic creature -- Flockers and Predators will specialize

Creature::Creature(int _index,
		   double init_x, double init_y, double init_z,
		   double init_vx, double init_vy, double init_vz,
		   float r, float g, float b,
		   int max_hist)
{ 
  index = _index;

  base_color = glm::vec3(r, g, b);

  max_history = max_hist;

  up = glm::vec3(0, 1, 0);

  position = glm::vec3(init_x, init_y, init_z);

  position_history.push_front(position);

  velocity = glm::vec3(init_vx, init_vy, init_vz);

  acceleration = glm::vec3(0.0, 0.0, 0.0);

  glGenBuffers(1, &vertexbuffer);
  glGenBuffers(1, &colorbuffer);
}

//----------------------------------------------------------------------------

// this should be called AFTER ALL UPDATES ARE COMPLETE

void Creature::finalize_update(double wrap_width, double wrap_height, double wrap_depth)
{
  // handle wrapping
  if (new_position.x > wrap_width)
    new_position.x -= wrap_width;
  else if (new_position.x < 0)
    new_position.x += wrap_width;

  if (new_position.y > wrap_height)
    new_position.y -= wrap_height;
  else if (new_position.y < 0)
    new_position.y += wrap_height;

  if (new_position.z > wrap_depth)
    new_position.z -= wrap_depth;
  else if (new_position.z < 0)
    new_position.z += wrap_depth;

  // make position = new_position, velocity = new_velocity for each creature

  velocity = new_velocity;
  position = new_position;

  // update frame

  frame_z = -1.0f * glm::normalize(velocity);
  frame_x = glm::cross(up, frame_z);
  frame_y = glm::cross(frame_z, frame_x);

  // to make sure these are unit vectors
  
  frame_x = glm::normalize(frame_x);
  frame_y = glm::normalize(frame_y);
    
  // keep track of recent positions

  position_history.push_front(position);

  if (position_history.size() > max_history) 
    position_history.pop_back();

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
