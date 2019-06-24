//Done by Shreelakshmi Alur

#include "Bullet_Utils.hh"
#include "Flocker.hh"
#include "Predator.hh"

using namespace std;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// created in main.cpp
extern GLFWwindow* window;
extern GLuint VertexArrayID;
extern GLuint objprogramID;
extern GLuint objMatrixID; 
extern GLuint objViewMatrixID;
extern GLuint objModelMatrixID;
extern GLuint objTextureID;
extern GLuint objLightID;

extern vector<glm::vec3> obj_vertices;
extern vector<glm::vec2> obj_uvs;
extern vector<glm::vec3> obj_normals;
extern GLuint obj_Texture;
extern vector<unsigned short> obj_indices;
extern vector<glm::vec3> obj_indexed_vertices;
extern vector<glm::vec2> obj_indexed_uvs;
extern vector<glm::vec3> obj_indexed_normals;
extern GLuint obj_vertexbuffer;
extern GLuint obj_uvbuffer;
extern GLuint obj_normalbuffer;
extern GLuint obj_elementbuffer;
extern int num_flockers;
extern vector <Flocker *> flocker_array;
extern vector <Predator *> predator_array;

extern float box_width;
extern float box_height;
extern float box_depth; 

btDiscreteDynamicsWorld* bullet_dynamicsWorld;
btSequentialImpulseConstraintSolver* bullet_solver;
btBroadphaseInterface* bullet_broadphase;
btDefaultCollisionConfiguration* bullet_collisionConfiguration;
btCollisionDispatcher* bullet_dispatcher;
vector<btRigidBody*> bullet_rigidbodies;   // bullet side

vector<glm::vec3> xyz_positions;        // ogl side
vector<glm::quat> quat_orientations;     // ogl side

float initialCameraZ = 25.0;

// cut these out of common/controls.cpp

glm::mat4 myViewMatrix;
glm::mat4 myProjectionMatrix;

glm::vec3 myPosition = glm::vec3( 0, 0, initialCameraZ ); ;
float myHorizontalAngle = 3.14f;
float myVerticalAngle = 0.0f;
float myInitialFoV = 45.0f;

float scaleFactor = 0.1;

bool isRestarting = false;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// get information FROM flocker objects
// note that orientations are NOT currently taken from flocker -- they are random

void copy_flocker_states_to_graphics_objects()
{
  xyz_positions.resize(num_flockers+1);
  quat_orientations.resize(num_flockers+1);
    int i ;

  for (i= 0; i < num_flockers; i++) {
    xyz_positions[i] = glm::vec3(flocker_array[i]->position.x, flocker_array[i]->position.y, flocker_array[i]->position.z);
    quat_orientations[i] = glm::normalize(glm::quat(glm::vec3((flocker_array[i]->rotation_radian),(flocker_array[i]->rotation_radian), (flocker_array[i]->rotation_radian))));
//    quat_orientations[i] = glm::normalize(glm::quat(glm::vec3(drand48() * 360.0, drand48() * 360.0, drand48() * 360.0)));
  }
    xyz_positions[i] = glm::vec3(predator_array[0]->position.x, predator_array[0]->position.y, predator_array[0]->position.z);
    quat_orientations[i] = quat_orientations[i] = glm::normalize(glm::quat(glm::vec3(drand48() * 360.0, drand48() * 360.0, drand48() * 360.0)));
}

//----------------------------------------------------------------------------

// move information TO flocker objects
// orientations are not touched

void copy_graphics_objects_to_flocker_states()
{
    int i;
  for (i = 0; i < num_flockers; i++) {
    flocker_array[i]->position.x = xyz_positions[i].x;
    flocker_array[i]->position.y = xyz_positions[i].y;
    flocker_array[i]->position.z = xyz_positions[i].z;
    
  }
    
    predator_array[0]->position.x = xyz_positions[i].x;
    predator_array[0]->position.y = xyz_positions[i].y;
    predator_array[0]->position.z = xyz_positions[i].z;
}

//----------------------------------------------------------------------------

// only the ground plane for now

void bullet_add_obstacles(btVector3 normal,btVector3 position)
{
  // add the ground plane (not drawn)

  btCollisionShape* groundShape = new btStaticPlaneShape(normal, 0);
  btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), position));
  btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
  btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
  bullet_dynamicsWorld->addRigidBody(groundRigidBody);
    
}

void addWalls(){
    bullet_add_obstacles(btVector3(1,0,0),btVector3(0,0,0));
    bullet_add_obstacles(btVector3(-1,0,0),btVector3(box_width,0,0));
    bullet_add_obstacles(btVector3(0,0,1),btVector3(0,0,0));
    bullet_add_obstacles(btVector3(0,0,-1),btVector3(0,0,box_depth));
    bullet_add_obstacles(btVector3(0,1,0),btVector3(0,0,0));
    bullet_add_obstacles(btVector3(0,-1,0),btVector3(0,box_height,0));
}

void addCylinder(){
//    btCollisionShape* cylinderShape = new btCylinderShape(btVector3(1.0,box_height/4,1.0));
//    btDefaultMotionState* cylinderMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(box_width/2,0.0,box_depth/2)));
//    btRigidBody::btRigidBodyConstructionInfo cylinderRigidBodyCI(0, cylinderMotionState, cylinderShape, btVector3(0, 0, 0));
//    btRigidBody* cylinderRigidBody = new btRigidBody(cylinderRigidBodyCI);
//    bullet_dynamicsWorld->addRigidBody(cylinderRigidBody);
    
    btCollisionShape* cylinder = new btCylinderShape(btVector3(1.0,  box_height / 12, 1.0));
    btDefaultMotionState* cylinder_motion_state = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(box_width*0.5, box_height / 4, box_depth*0.5)));
    btRigidBody* cylinder_body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(btScalar(1.0f), cylinder_motion_state, cylinder, btVector3(0, 0, 0)));
    bullet_dynamicsWorld->addRigidBody(cylinder_body);
}

void addCube(){
    btCollisionShape* cube = new btBoxShape(btVector3(1.0,  box_height / 4, 1.0));
    btDefaultMotionState* cube_motion_state = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(box_width*0.5, box_height / 4, box_depth*0.5)));
    btRigidBody* cube_body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(btScalar(1.0f), cube_motion_state, cube, btVector3(0, 0, 0)));
    bullet_dynamicsWorld->addRigidBody(cube_body);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// uniform distribution of positions and orientations within box
// only used in demo mode

void randomize_graphics_objects()
{
  for (int i = 0; i < num_flockers; i++) {
    xyz_positions[i] = glm::vec3(drand48() * box_width, drand48() * box_height, drand48() * box_depth);
    quat_orientations[i] = glm::normalize(glm::quat(glm::vec3(drand48() * 360.0, drand48() * 360.0, drand48() * 360.0)));
  }
}

//----------------------------------------------------------------------------

void myComputeMatricesFromInputs()
{
  // Direction : Spherical coordinates to Cartesian coordinates conversion
  glm::vec3 direction(
		      cos(myVerticalAngle) * sin(myHorizontalAngle), 
		      sin(myVerticalAngle),
		      cos(myVerticalAngle) * cos(myHorizontalAngle)
		      );
	
  // Right vector
  glm::vec3 right = glm::vec3(sin(myHorizontalAngle - 3.14f/2.0f), 
			      0,
			      cos(myHorizontalAngle - 3.14f/2.0f));
	
  // Up vector
  glm::vec3 up = glm::cross( right, direction );
  
  // Move forward
  if (glfwGetKey( window, GLFW_KEY_W ) == GLFW_PRESS){
    myPosition += direction * scaleFactor;
  }
  // Move backward
  if (glfwGetKey( window, GLFW_KEY_S ) == GLFW_PRESS){
    myPosition -= direction * scaleFactor;
  }
  // Move up
  if (glfwGetKey( window, GLFW_KEY_UP ) == GLFW_PRESS){
    myPosition += up * scaleFactor;
  }
  // Move down
  if (glfwGetKey( window, GLFW_KEY_DOWN ) == GLFW_PRESS){
    myPosition -= up * scaleFactor;
  }
  // Strafe right
  if (glfwGetKey( window, GLFW_KEY_D ) == GLFW_PRESS){
    myPosition += right * scaleFactor;
  }
  // Strafe left
  if (glfwGetKey( window, GLFW_KEY_A ) == GLFW_PRESS){
    myPosition -= right * scaleFactor;
  }

  if (glfwGetKey( window, GLFW_KEY_5 ) == GLFW_PRESS) {
    if (!isRestarting) {
      isRestarting = true;
      printf("randomize!\n");
      delete_bullet_simulator();
      randomize_graphics_objects();
      initialize_bullet_simulator();
    }
  }

  if (isRestarting && glfwGetKey( window, GLFW_KEY_5 ) == GLFW_RELEASE) {
    isRestarting = false;

    //    printf("RELEASE!!!!!\n"); fflush(stdout);
  }
  
  // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
  myProjectionMatrix = glm::perspective(glm::radians(myInitialFoV), 4.0f / 3.0f, 0.1f, 100.0f);
  // Camera matrix
  myViewMatrix       = glm::lookAt(
				 myPosition,           // Camera is here
				 myPosition+direction, // and looks here : at the same position, plus "direction"
				 up                  // Head is up (set to 0,-1,0 to look upside-down)
				 ); 
}

//----------------------------------------------------------------------------

// add all of the moveable objects.
// quat_orientations, xyz_positions should already be set

void bullet_add_dynamic_objects()
{
  // from here: http://bulletphysics.org/mediawiki-1.5.8/index.php/Simple_RigidBody_loaded_from_an_obj_file
  
  btConvexHullShape* chShape = new btConvexHullShape();
  for (int i = 0; i < obj_vertices.size(); i++) 
    chShape->addPoint(btVector3(obj_vertices[i].x, obj_vertices[i].y, obj_vertices[i].z));
  chShape->initializePolyhedralFeatures();
  
  // optimizeConvexHull() from link above does not work here, so simplifying in the following way:
  // http://www.bulletphysics.org/mediawiki-1.5.8/index.php?title=BtShapeHull_vertex_reduction_utility
  
  btShapeHull *hull = new btShapeHull(chShape);
  btScalar margin = chShape->getMargin();
  hull->buildHull(margin);
  btConvexHullShape* simplifiedShape = new btConvexHullShape();
  printf("%i before, %i after\n", (int) obj_vertices.size(), (int) hull->numVertices());
  for (int i = 0; i < hull->numVertices(); i++) 
    simplifiedShape->addPoint(hull->getVertexPointer()[i]);
  
  btScalar shapeMass = 1;  // in kg.  0 -> static object aka never moves
  btVector3 simplifiedShapeInertia(0, 0, 0);   // if we leave it as all 0's -> treat like point mass -> no spin
  simplifiedShape->calculateLocalInertia(shapeMass, simplifiedShapeInertia);
  
  // put objects into physics simulation
  
  for (int i = 0; i < num_flockers+1; i++) {
    
    btDefaultMotionState* motionstate = new btDefaultMotionState(btTransform(btQuaternion(quat_orientations[i].x, quat_orientations[i].y, quat_orientations[i].z, quat_orientations[i].w), 
									     btVector3(xyz_positions[i].x, xyz_positions[i].y, xyz_positions[i].z)));
    
    btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(shapeMass,                
							 motionstate,
							 simplifiedShape,   // collision shape of body
							 simplifiedShapeInertia);
    
    btRigidBody *rigidBody = new btRigidBody(rigidBodyCI);
    
      if (i < num_flockers)
      {
          rigidBody->setLinearVelocity(btVector3(flocker_array[i]->new_velocity.x * 50, flocker_array[i]->new_velocity.y * 50, flocker_array[i]->new_velocity.z * 50));      //Adding velocity to each flocker
      }
      else{
          rigidBody->setLinearVelocity(btVector3(predator_array[0]->new_velocity.x * 50, predator_array[0]->new_velocity.y * 50, predator_array[0]->new_velocity.z * 50));
      }
    
    bullet_rigidbodies.push_back(rigidBody);
    bullet_dynamicsWorld->addRigidBody(rigidBody);    
  }
}

//----------------------------------------------------------------------------

// This strictly follows http://bulletphysics.org/mediawiki-1.5.8/index.php/Hello_World, 

void initialize_bullet_simulator()
{  
  // Build the broadphase
  
  bullet_broadphase = new btDbvtBroadphase();
  
  // Set up the collision configuration and dispatcher
  
  bullet_collisionConfiguration = new btDefaultCollisionConfiguration();
  bullet_dispatcher = new btCollisionDispatcher(bullet_collisionConfiguration);
  
  // The actual physics solver
  
  bullet_solver = new btSequentialImpulseConstraintSolver;
  
  // The world.
  
  bullet_dynamicsWorld = new btDiscreteDynamicsWorld(bullet_dispatcher, bullet_broadphase, bullet_solver, bullet_collisionConfiguration);
  bullet_dynamicsWorld->setGravity(btVector3(0,-9.81f,0));

  // the stuff in the world
  
  addWalls();
  bullet_add_dynamic_objects();
  addCylinder();
}

//----------------------------------------------------------------------------

// Clean up behind ourselves like good little programmers

void delete_bullet_simulator()
{
  // remove all of the objects

  for (int i = 0; i < bullet_rigidbodies.size(); i++){
    bullet_dynamicsWorld->removeRigidBody(bullet_rigidbodies[i]);
    delete bullet_rigidbodies[i]->getMotionState();
    delete bullet_rigidbodies[i];
  }
  bullet_rigidbodies.clear();
  
  delete bullet_dynamicsWorld;
  delete bullet_solver;
  delete bullet_collisionConfiguration;
  delete bullet_dispatcher;
  delete bullet_broadphase;
}

//----------------------------------------------------------------------------

void update_physics_simulation(float deltaTime)
{
  // actually simulate motion and collisions
  
  bullet_dynamicsWorld->stepSimulation(deltaTime, 7);

  // query new positions and orientations
  
  for (int i = 0; i < num_flockers+1; i++) {

    btTransform trans;
    bullet_rigidbodies[i]->getMotionState()->getWorldTransform(trans);
    
    //      printf("%i height = %f\n", i, trans.getOrigin().getY()); fflush(stdout);
    xyz_positions[i].x = trans.getOrigin().getX();
    xyz_positions[i].y = trans.getOrigin().getY();
    xyz_positions[i].z = trans.getOrigin().getZ();
    
    quat_orientations[i].x = trans.getRotation().getX();
    quat_orientations[i].y = trans.getRotation().getY();
    quat_orientations[i].z = trans.getRotation().getZ();
    quat_orientations[i].w = trans.getRotation().getW();

  }

  // put information back in flocker objects
  
  copy_graphics_objects_to_flocker_states();
}

//----------------------------------------------------------------------------

// only called in demo mode, not flocking

int bullet_hello_main( int argc, char **argv )
{  
  //  randomize_graphics_objects();
  copy_flocker_states_to_graphics_objects();
  
  // Initialize Bullet library.

  initialize_bullet_simulator();
  
  // For speed computation
  double lastTime = glfwGetTime();
  int nbFrames = 0;

  do {

    // Measure speed
    double currentTime = glfwGetTime();
    nbFrames++;
    if ( currentTime - lastTime >= 1.0 ){ // If last printf() was more than 1sec ago
      // printf and reset
      printf("%f ms/frame\n", 1000.0/double(nbFrames));
      nbFrames = 0;
      lastTime += 1.0;
    }
    float deltaTime = currentTime - lastTime;

    // Step the simulation
    bullet_dynamicsWorld->stepSimulation(deltaTime, 7);

    // Compute the MVP matrix from keyboard and mouse input
    myComputeMatricesFromInputs();

    glm::mat4 ProjectionMatrix = myProjectionMatrix;
    glm::mat4 ViewMatrix = myViewMatrix;
    
    // Clear the screen for real rendering
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
    // Use our shader
    glUseProgram(objprogramID);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    for (int i = 0; i < num_flockers+1; i++) {

      btTransform trans;
      bullet_rigidbodies[i]->getMotionState()->getWorldTransform(trans);

      xyz_positions[i].x = trans.getOrigin().getX();
      xyz_positions[i].y = trans.getOrigin().getY();
      xyz_positions[i].z = trans.getOrigin().getZ();

      quat_orientations[i].x = trans.getRotation().getX();
      quat_orientations[i].y = trans.getRotation().getY();
      quat_orientations[i].z = trans.getRotation().getZ();
      quat_orientations[i].w = trans.getRotation().getW();

      glm::mat4 RotationMatrix = glm::toMat4(quat_orientations[i]);
      glm::mat4 TranslationMatrix = translate(mat4(), xyz_positions[i]);
      glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix;
      
      glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

      // Send our transformation to the currently bound shader, 
      // in the "MVP" uniform
      glUniformMatrix4fv(objMatrixID, 1, GL_FALSE, &MVP[0][0]);
      glUniformMatrix4fv(objModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
      glUniformMatrix4fv(objViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
      
      // Bind this object's texture in Texture Unit 0
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, obj_Texture);
      // Set our "myTextureSampler" sampler to use Texture Unit 0
      glUniform1i(objTextureID, 0);
      
      // 1st attribute buffer : vertices
      glBindBuffer(GL_ARRAY_BUFFER, obj_vertexbuffer);
      glVertexAttribPointer(
			    0,                  // attribute
			    3,                  // size
			    GL_FLOAT,           // type
			    GL_FALSE,           // normalized?
			    0,                  // stride
			    (void*)0            // array buffer offset
			    );
      
      // 2nd attribute buffer : UVs
      glBindBuffer(GL_ARRAY_BUFFER, obj_uvbuffer);
      glVertexAttribPointer(
			    1,                                // attribute
			    2,                                // size
			    GL_FLOAT,                         // type
			    GL_FALSE,                         // normalized?
			    0,                                // stride
			    (void*)0                          // array buffer offset
			    );

      // 3rd attribute buffer : normals
      glBindBuffer(GL_ARRAY_BUFFER, obj_normalbuffer);
      glVertexAttribPointer(
			    2,                                // attribute
			    3,                                // size
			    GL_FLOAT,                         // type
			    GL_FALSE,                         // normalized?
			    0,                                // stride
			    (void*)0                          // array buffer offset
			    );
      
      // Index buffer
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj_elementbuffer);

      // Draw the triangles !
      glDrawElements(
		     GL_TRIANGLES,      // mode
		     obj_indices.size(),    // count
		     GL_UNSIGNED_SHORT,   // type
		     (void*)0           // element array buffer offset
		     );
      
    }  // end loop over objects


    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    
    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
    
  } // Check if the ESC key was pressed or the window was closed
  //  while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
  while( glfwGetKey(window, GLFW_KEY_Q ) != GLFW_PRESS &&
	 glfwWindowShouldClose(window) == 0 );
  
  // Cleanup VBO and shader
  glDeleteBuffers(1, &obj_vertexbuffer);
  glDeleteBuffers(1, &obj_uvbuffer);
  glDeleteBuffers(1, &obj_normalbuffer);
  glDeleteBuffers(1, &obj_elementbuffer);
  glDeleteProgram(objprogramID);
  glDeleteTextures(1, &obj_Texture);
  glDeleteVertexArrays(1, &VertexArrayID);
  
  // Close OpenGL window and terminate GLFW
  glfwTerminate();
  
  delete_bullet_simulator();
  
  return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
