#ifndef PREDATOR_HH

#define PREDATOR_HH


#include "Creature.hh"
#include "Flocker.hh"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define MAX_PREDATOR_SPEED                           0.04
#define DRAW_MODE_HISTORY           0
#define DRAW_MODE_AXES              1
#define DRAW_MODE_POLY              2
#define DRAW_MODE_OBJ               3

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class Predator : public Creature
{
public:
    glm::vec3 hunger_force;
    
    double random_force_limit;
    double hunger_weight;
  
    Predator(int,                    // index
             double, double, double, // initial position
             double, double, double, // initial velocity
             double,                 // random uniform acceleration limit
             double,                 // hunger weight
             float, float, float,    // base color
             int);               // number of past states to save
    
    void draw();
    void draw(glm::mat4);
    void update();
    bool compute_hunger_force();
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif
