#ifndef FLOCKER_HH

#define FLOCKER_HH

#include "Creature.hh"
#include "Predator.hh"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define MAX_FLOCKER_SPEED           0.04

#define DRAW_MODE_HISTORY           0
#define DRAW_MODE_AXES              1
#define DRAW_MODE_POLY              2
#define DRAW_MODE_OBJ               3

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class Flocker : public Creature
{
public:

  glm::vec3 separation_force;
  glm::vec3 alignment_force;
  glm::vec3 cohesion_force;
  glm::vec3 fear_force;

  double random_force_limit;

  double separation_weight;
  double min_separation_distance, min_squared_separation_distance;
  double max_separation_distance, max_squared_separation_distance;
  double inv_range_squared_separation_distance;

  double alignment_weight;
  double min_alignment_distance, min_squared_alignment_distance;
  double max_alignment_distance, max_squared_alignment_distance;
  double inv_range_squared_alignment_distance;

  double cohesion_weight;
  double min_cohesion_distance, min_squared_cohesion_distance;
  double max_cohesion_distance, max_squared_cohesion_distance;
  double inv_range_squared_cohesion_distance;
    
  double fear_weight;
    
  float rotation_radian;

  Flocker(int,                    // index
	  double, double, double, // initial position
	  double, double, double, // initial velocity
	  double,                 // random uniform acceleration limit
	  double, double, double, // min, max separation distance, weight
	  double, double, double, // min, max alignment distance, weight
	  double, double, double, // min, max cohesion distance, weight
      double, // min, max fear distance, weight
      float,                 // random radian
	  float, float, float,    // base color
	  int = 1);               // number of past states to save

  void draw();
  void draw(glm::mat4);
  void update();
  bool compute_separation_force();
  bool compute_alignment_force();
  bool compute_cohesion_force();
  bool compute_fear_force();
  double distance_to_predator(int);
  int findIfPredatorIsNear(glm::vec3);
};

//----------------------------------------------------------------------------

void calculate_flocker_squared_distances();


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif
