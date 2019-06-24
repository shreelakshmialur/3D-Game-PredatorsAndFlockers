// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// Bullet-specific stuff
#include "Bullet_Utils.hh"

// for loading GLSL shaders
#include <common/shader.hpp>

// creature-specific stuff
#include "Flocker.hh"
#include "Predator.hh"

#include <common/text2D.hpp>

//----------------------------------------------------------------------------

// to avoid gimbal lock issues...
#define MAX_LATITUDE_DEGS     89.0
#define MIN_LATITUDE_DEGS    -89.0

#define CAMERA_MODE_ORBIT    1
#define CAMERA_MODE_CREATURE 2

#define MIN_ORBIT_CAM_RADIUS    (7.0)
#define MAX_ORBIT_CAM_RADIUS    (30.0)

#define DEFAULT_ORBIT_CAM_RADIUS            27.0
#define DEFAULT_ORBIT_CAM_LATITUDE_DEGS     15.0
#define DEFAULT_ORBIT_CAM_LONGITUDE_DEGS    60.0

//----------------------------------------------------------------------------

// some convenient globals
GLFWwindow* window;

GLuint programID;
GLuint objprogramID;

GLuint MatrixID;
GLuint ViewMatrixID;
GLuint ModelMatrixID;

GLuint objMatrixID;
GLuint objViewMatrixID;
GLuint objModelMatrixID;
GLuint objTextureID;
GLuint objLightID;

GLuint VertexArrayID;

float box_width  = 9.0;
float box_height = 5.0;
float box_depth =  7.0;

// used for obj files
vector<glm::vec3> obj_vertices;
vector<glm::vec2> obj_uvs;
vector<glm::vec3> obj_normals;
GLuint obj_Texture;
vector<unsigned short> obj_indices;
vector<glm::vec3> obj_indexed_vertices;
vector<glm::vec2> obj_indexed_uvs;
vector<glm::vec3> obj_indexed_normals;
GLuint obj_vertexbuffer;
GLuint obj_uvbuffer;
GLuint obj_normalbuffer;
GLuint obj_elementbuffer;

// these along with Model matrix make MVP transform
glm::mat4 ProjectionMat;
glm::mat4 ViewMat;

// sim-related globals
double target_FPS = 60.0;

bool using_obj_program = false;

bool is_paused = false;
bool is_physics_active = false;

bool isRight = true;
bool isLeft = false;
bool isUp = false;
bool isDown = false;
bool isFront = false;
bool isBack = false;
bool isGameOver = false;

double orbit_cam_radius = DEFAULT_ORBIT_CAM_RADIUS;
double orbit_cam_delta_radius = 0.1;

double orbit_cam_latitude_degs = DEFAULT_ORBIT_CAM_LATITUDE_DEGS;
double orbit_cam_longitude_degs = DEFAULT_ORBIT_CAM_LONGITUDE_DEGS;
double orbit_cam_delta_theta_degs = 1.0;

int win_scale_factor = 60;
int win_w = win_scale_factor * 16.0;
int win_h = win_scale_factor * 9.0;
int camera_mode = CAMERA_MODE_ORBIT;

int num_flockers = 10;   // 400 is "comfortable" max on my machine
int num_predators = 1;

extern int flocker_history_length;
extern int predator_history_length;
extern int flocker_draw_mode;
extern int predator_draw_mode;
extern int flockNumber;
extern vector <Flocker *> flocker_array;
extern vector <vector <double> > flocker_squared_distance;
extern vector <Predator *> predator_array;
extern vector <vector <double> > predator_squared_distance;

#define PI 3.1415927
#define Cos(th) cos(PI/180*(th))
#define Sin(th) sin(PI/180*(th))

GLuint box_vertexbuffer;
GLuint box_colorbuffer;
GLuint quad_vertexbuffer;
GLuint quad_colorbuffer;
GLuint cube_vertexbuffer;
GLuint cube_colorbuffer;
GLuint circle1_vertexbuffer;
GLuint circle1_colorbuffer;
GLuint circle2_vertex_buffer;
GLuint circle2_colorbuffer;

GLuint billprogramID;
// Vertex shader
GLuint CameraRight_worldspace_ID ;
GLuint CameraUp_worldspace_ID;
GLuint ViewProjMatrixID ;
GLuint BillboardPosID ;
GLuint BillboardSizeID ;
GLuint LifeLevelID ;

GLuint billTextureID ;
GLuint billboard_vertex_buffer;

GLuint billTexture ;

GLfloat centerX, centerY, centerZ;

vector<bool> dead_flocker_array(num_flockers);
int score = 0;

float LifeLevel;

GLuint TextureID;
GLuint uvbuffer;
GLuint textureprogramID;
GLuint vertexbuffer;
GLuint Texture;
glm::mat4 TMVP;
//----------------------------------------------------------------------------

void end_program()
{
  // Cleanup VBOs and shader
  glDeleteBuffers(1, &box_vertexbuffer);
  glDeleteBuffers(1, &box_colorbuffer);
  glDeleteBuffers(1, &vertexbuffer);
  glDeleteProgram(programID);
  glDeleteProgram(objprogramID);
  glDeleteProgram(billprogramID);
  glDeleteBuffers(1, &uvbuffer);
  glDeleteProgram(textureprogramID);
  glDeleteTextures(1, &TextureID);
  glDeleteVertexArrays(1, &VertexArrayID);
  
  // Close OpenGL window and terminate GLFW
  glfwTerminate();

  exit(1);
}

void draw_billboard(glm::mat4 Model)
{
    // The VBO containing the 4 vertices of the particles.
    
    static const GLfloat g_vertex_buffer_data[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
    };
    
    glGenBuffers(1, &billboard_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_DYNAMIC_DRAW);
    
    glUseProgram(billprogramID);
    
    glm::mat4 MVP = ProjectionMat * ViewMat * Model;
    
    // We will need the camera's position in order to sort the particles
    // w.r.t the camera's distance.
    // There should be a getCameraPosition() function in common/controls.cpp,
    // but this works too.
    glm::vec3 CameraPosition(glm::inverse(ViewMat)[3]);
    
    glm::mat4 VP = ProjectionMat * ViewMat;
    
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use our shader
    //glUseProgram(billprogramID);
    
    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, billTexture);
    // Set our "myTextureSampler" sampler to use Texture Unit 0
    glUniform1i(billTextureID, 3);
    
    glUniform3f(CameraRight_worldspace_ID, ViewMat[0][0], ViewMat[1][0], ViewMat[2][0]);
    glUniform3f(CameraUp_worldspace_ID   , ViewMat[0][1], ViewMat[1][1], ViewMat[2][1]);
    
    //glUniform3f(BillboardPosID, box_width/5, box_height-1, 0.0f); // The billboard will be just above the cube
    glUniform3f(BillboardPosID, 0.0f, 0.0f, 0.0f);
    glUniform2f(BillboardSizeID, 2.0f, 0.25f);     // and 1m*12cm, because it matches its 256*32 resolution =)
    
    double currentTime = glfwGetTime();
    float scorer = 0.4f;
    // Generate some life level and send it to glsl
    if (score < 1)
    {
        LifeLevel = 1.0f - 0.05f * currentTime;
    }
    else if(score>=1){
        LifeLevel = 1.0f - 0.05f * currentTime + scorer;
        scorer = scorer*2;
    }
//    LifeLevel = 1.0f - 0.05f * currentTime;
    glUniform1f(LifeLevelID, LifeLevel);
    
    glUniformMatrix4fv(ViewProjMatrixID, 1, GL_FALSE, &MVP[0][0]);
    
    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glVertexAttribPointer(
                          0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                          3,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );
    
    
    // Draw the billboard !
    // This draws a triangle_strip which looks like a quad.
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glDisableVertexAttribArray(0);
}

//----------------------------------------------------------------------------
void draw_box(glm::mat4 Model)
{
  // Our ModelViewProjection : multiplication of our 3 matrices
  glm::mat4 MVP = ProjectionMat * ViewMat * Model;

  // make this transform available to shaders
  glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // 1st attribute buffer : vertices
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, box_vertexbuffer);
  glVertexAttribPointer(0,                  // attribute. 0 to match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
  
  // 2nd attribute buffer : colors
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, box_colorbuffer);
  glVertexAttribPointer(1,                                // attribute. 1 to match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);

  // Draw the box!
  glDrawArrays(GL_LINES, 0, 24); 
  
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}

void draw_quad(glm::mat4 Model)
{
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 MVP = ProjectionMat * ViewMat * Model;
    
    // make this transform available to shaders
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    
    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
    glVertexAttribPointer(0,                  // attribute. 0 to match the layout in the shader.
                          3,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );
    
    // 2nd attribute buffer : colors
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, quad_colorbuffer);
    glVertexAttribPointer(1,                                // attribute. 1 to match the layout in the shader.
                          3,                                // size
                          GL_FLOAT,                         // type
                          GL_FALSE,                         // normalized?
                          0,                                // stride
                          (void*)0                          // array buffer offset
                          );
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 92);
    
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void draw_bottom_circle(glm::mat4 Model)
{
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 MVP = ProjectionMat * ViewMat * Model;
    
    // make this transform available to shaders
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    
    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, circle1_vertexbuffer);
    glVertexAttribPointer(0,                  // attribute. 0 to match the layout in the shader.
                          3,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );
    
    // 2nd attribute buffer : colors
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, circle1_colorbuffer);
    glVertexAttribPointer(1,                                // attribute. 1 to match the layout in the shader.
                          3,                                // size
                          GL_FLOAT,                         // type
                          GL_FALSE,                         // normalized?
                          0,                                // stride
                          (void*)0                          // array buffer offset
                          );
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 47);
    
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void draw_top_circle(glm::mat4 Model)
{
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 MVP = ProjectionMat * ViewMat * Model;
    
    // make this transform available to shaders
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    
    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, circle2_vertex_buffer);
    glVertexAttribPointer(0,                  // attribute. 0 to match the layout in the shader.
                          3,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );
    
    // 2nd attribute buffer : colors
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, circle2_colorbuffer);
    glVertexAttribPointer(1,                                // attribute. 1 to match the layout in the shader.
                          3,                                // size
                          GL_FLOAT,                         // type
                          GL_FALSE,                         // normalized?
                          0,                                // stride
                          (void*)0                          // array buffer offset
                          );
    
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 47);
    
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

//----------------------------------------------------------------------------

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  // quit
  if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    end_program();

  // pause
  else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    is_paused = !is_paused;

  // toggle physics/flocking dynamics modes
  else if (key == GLFW_KEY_P && action == GLFW_PRESS) {
    is_physics_active = !is_physics_active;

    // spin up physics simulator
    if (is_physics_active) {
      copy_flocker_states_to_graphics_objects();
      initialize_bullet_simulator();
    }

    // stop physics simulator
    else {
      delete_bullet_simulator();
    }
  }
  
  // orbit rotate
  else if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT)) 
    orbit_cam_longitude_degs -= orbit_cam_delta_theta_degs;
  else if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
    orbit_cam_longitude_degs += orbit_cam_delta_theta_degs;
  else if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if (orbit_cam_latitude_degs + orbit_cam_delta_theta_degs <= MAX_LATITUDE_DEGS)
      orbit_cam_latitude_degs += orbit_cam_delta_theta_degs;
  }
  else if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if (orbit_cam_latitude_degs - orbit_cam_delta_theta_degs >= MIN_LATITUDE_DEGS)
      orbit_cam_latitude_degs -= orbit_cam_delta_theta_degs;
  }

  // orbit zoom in/out
  else if (key == GLFW_KEY_Z && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if (orbit_cam_radius + orbit_cam_delta_radius <= MAX_ORBIT_CAM_RADIUS)
      orbit_cam_radius += orbit_cam_delta_radius;
  }
  else if (key == GLFW_KEY_C && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    if (orbit_cam_radius - orbit_cam_delta_radius >= MIN_ORBIT_CAM_RADIUS)
      orbit_cam_radius -= orbit_cam_delta_radius;
  }

  // orbit pose reset
  else if (key == GLFW_KEY_X && action == GLFW_PRESS) {
    orbit_cam_radius = DEFAULT_ORBIT_CAM_RADIUS;
    orbit_cam_latitude_degs = DEFAULT_ORBIT_CAM_LATITUDE_DEGS;
    orbit_cam_longitude_degs = DEFAULT_ORBIT_CAM_LONGITUDE_DEGS;
  }

  // flocker drawing options
  else if (key == GLFW_KEY_7 && action == GLFW_PRESS) {
    flocker_draw_mode = DRAW_MODE_OBJ;
    predator_draw_mode = DRAW_MODE_OBJ;
    using_obj_program = true;
  }
  else if (key == GLFW_KEY_8 && action == GLFW_PRESS) {
    flocker_draw_mode = DRAW_MODE_POLY;
    predator_draw_mode = DRAW_MODE_POLY;
    using_obj_program = false;
  }
  else if (key == GLFW_KEY_9 && action == GLFW_PRESS) {
    flocker_draw_mode = DRAW_MODE_AXES;
    predator_draw_mode = DRAW_MODE_AXES;
    using_obj_program = false;
  }
  else if (key == GLFW_KEY_0 && action == GLFW_PRESS) {
    flocker_draw_mode = DRAW_MODE_HISTORY;
    predator_draw_mode = DRAW_MODE_HISTORY;
    using_obj_program = false;
  }
  else if(key == GLFW_KEY_UP && action == GLFW_PRESS){
      isRight = false;
      isLeft = false;
      isUp = true;
      isDown = false;
      isFront = false;
      isBack = false;
  }
  else if(key == GLFW_KEY_DOWN && action == GLFW_PRESS){
      isRight = false;
      isLeft = false;
      isUp = false;
      isDown = true;
      isFront = false;
      isBack = false;
  }
  else if(key == GLFW_KEY_RIGHT && action == GLFW_PRESS){
      isRight = true;
      isLeft = false;
      isUp = false;
      isDown = false;
      isFront = false;
      isBack = false;
  }
  else if(key == GLFW_KEY_LEFT && action == GLFW_PRESS){
      isRight = false;
      isLeft = true;
      isUp = false;
      isDown = false;
      isFront = false;
      isBack = false;
  }
  else if(key == GLFW_KEY_G && action == GLFW_PRESS){
      isRight = false;
      isLeft = false;
      isUp = false;
      isDown = false;
      isFront = true;
      isBack = false;
  }
  else if(key == GLFW_KEY_H && action == GLFW_PRESS){
      isRight = false;
      isLeft = false;
      isUp = false;
      isDown = false;
      isFront = false;
      isBack = true;
  }
}

void gameOver(){
    if(isGameOver){
        copy_flocker_states_to_graphics_objects();
        initialize_bullet_simulator();
    }
}

//----------------------------------------------------------------------------

// allocate simulation data structures and populate them
void initialize_flocking_simulation()
{
    static const GLfloat quad_vertex_buffer_data[] = {
        1.0f, box_height / 4, 0.0f,
        1.0f, box_height/3, 0.0f,
        0.99f, box_height / 4, 0.1f,
        0.99f, box_height/3, 0.1f,
        0.97f, box_height / 4, 0.2f,
        0.97f, box_height/3, 0.2f,
        0.95f, box_height / 4, 0.3f,
        0.95f, box_height/3, 0.3f,
        0.91f, box_height / 4, 0.4f,
        0.91f, box_height/3, 0.4f,
        0.86f, box_height / 4, 0.5f,
        0.86f, box_height/3, 0.5f,
        0.79f, box_height / 4, 0.6f,
        0.79f, box_height/3, 0.6f,
        0.71f, box_height / 4, 0.7f,
        0.71f, box_height/3, 0.7f,
        0.59f,box_height / 4, 0.8f,
        0.59f, box_height/3, 0.8f,
        0.43f, box_height / 4, 0.9f,
        0.43f, box_height/3, 0.9f,
        0.21f,box_height / 4, 1.0f,
        0.21f, box_height/3, 1.0f,
        0.0f, box_height / 4, 1.0f,
        0.0f, box_height/3, 1.0f,
        -0.21f,box_height / 4, 1.0f,
        -0.21f, box_height/3, 1.0f,
        -0.43f, box_height / 4, 0.9f,
        -0.43f, box_height/3, 0.9f,
        -0.59f, box_height / 4, 0.8f,
        -0.59f, box_height/3, 0.8f,
        -0.71f, box_height / 4, 0.7f,
        -0.71f, box_height/3, 0.7f,
        -0.79f, box_height / 4, 0.6f,
        -0.79f, box_height/3, 0.6f,
        -0.86f, box_height / 4, 0.5f,
        -0.86f, box_height/3, 0.5f,
        -0.91f, box_height / 4, 0.4f,
        -0.91f, box_height/3, 0.4f,
        -0.95f, box_height / 4, 0.3f,
        -0.95f, box_height/3, 0.3f,
        -0.97f, box_height / 4, 0.2f,
        -0.97f, box_height/3, 0.2f,
        -0.99f, box_height / 4, 0.1f,
        -0.99f, box_height/3, 0.1f,
        -1.0f, box_height / 4, 0.0f,
        -1.0f, box_height/3, 0.0f,
        -1.0f, box_height / 4, -0.0f,
        -1.0f, box_height/3, -0.0f,
        -0.99f, box_height / 4, -0.0f,
        -0.99f, box_height/3, -0.0f,
        -0.97f, box_height / 4, -0.2f,
        -0.97f, box_height/3, -0.2f,
        -0.95f, box_height / 4, -0.3f,
        -0.95f, box_height/3, -0.3f,
        -0.91f, box_height / 4, -0.4f,
        -0.91f, box_height/3, -0.4f,
        -0.86f, box_height / 4, -0.5f,
        -0.86f, box_height/3, -0.5f,
        -0.79f, box_height / 4, -0.6f,
        -0.79f, box_height/3, -0.6f,
        -0.71f, box_height / 4, -0.7f,
        -0.71f, box_height/3, -0.7f,
        -0.59f, box_height / 4, -0.8f,
        -0.59f, box_height/3, -0.8f,
        -0.43f, box_height / 4, -0.9f,
        -0.43f, box_height/3, -0.9f,
        -0.21f, box_height / 4, -1.0f,
        -0.21f, box_height/3, -1.0f,
        0.0f, box_height / 4, -1.0f,
        0.0f, box_height/3, -0.1f,
        0.21f, box_height / 4, -1.0f,
        0.21f, box_height/3, -1.0f,
        0.43f, box_height / 4, -0.9f,
        0.43f, box_height/3, -0.9f,
        0.59f, box_height / 4, -0.8f,
        0.59f, box_height/3, -0.8f,
        0.71f,box_height / 4, -0.7f,
        0.71f, box_height/3, -0.7f,
        0.79f, box_height / 4, -0.6f,
        0.79f, box_height/3, -0.6f,
        0.86f, box_height / 4, -0.5f,
        0.86f, box_height/3, -0.5f,
        0.91f, box_height / 4, -0.4f,
        0.91f, box_height/3, -0.4f,
        0.95f,box_height / 4, -0.3f,
        0.95f, box_height/3, -0.3f,
        0.97f, box_height / 4, -0.2f,
        0.97f, box_height/3, -0.2f,
        0.99f, box_height / 4, -0.1f,
        0.99f, box_height/3, -0.1f,
        1.0f, box_height / 4, 0.0f,
        1.0f, box_height/3, 0.0f
    };
    
    static const GLfloat circle1_vertex_buffer_data[] = {
        //0.0f, box_height / 4, 0.0f,
        0.0f, 0.5f, 0.0f,
        1.0f, box_height / 4, 0.0f,
        0.99f, box_height / 4, 0.1f,
        0.97f, box_height / 4, 0.2f,
        0.95f, box_height / 4, 0.3f,
        0.91f, box_height / 4, 0.4f,
        0.86f, box_height / 4, 0.5f,
        0.79f, box_height / 4, 0.6f,
        0.71f, box_height / 4, 0.7f,
        0.59f, box_height / 4, 0.8f,
        0.43f, box_height / 4, 0.9f,
        0.21f, box_height / 4, 1.0f,
        0.0f, box_height / 4, 1.0f,
        -0.21f, box_height / 4, 1.0f,
        -0.43f, box_height / 4, 0.9f,
        -0.59f, box_height / 4, 0.8f,
        -0.71f, box_height / 4, 0.7f,
        -0.79f, box_height / 4, 0.6f,
        -0.86f, box_height / 4, 0.5f,
        -0.91f, box_height / 4, 0.4f,
        -0.95f, box_height / 4, 0.3f,
        -0.97f, box_height / 4, 0.2f,
        -0.99f, box_height / 4, 0.1f,
        -1.0f, box_height / 4, 0.0f,
        -1.0f, box_height / 4, -0.0f,
        -0.99f, box_height / 4, -0.1f,
        -0.97f, box_height / 4, -0.2f,
        -0.95f, box_height / 4, -0.3f,
        -0.91f, box_height / 4, -0.4f,
        -0.86f, box_height / 4, -0.5f,
        -0.79f, box_height / 4, -0.6f,
        -0.71f, box_height / 4, -0.7f,
        -0.59f, box_height / 4, -0.8f,
        -0.43f, box_height / 4, -0.9f,
        -0.21f, box_height / 4, -1.0f,
        0.0f, box_height / 4, -1.0f,
        0.21f, box_height / 4, -1.0f,
        0.43f, box_height / 4, -0.9f,
        0.59f, box_height / 4, -0.8f,
        0.71f, box_height / 4, -0.7f,
        0.79f, box_height / 4, -0.6f,
        0.86f, box_height / 4, -0.5f,
        0.91f, box_height / 4, -0.4f,
        0.95f, box_height / 4, -0.3f,
        0.97f, box_height / 4, -0.2f,
        0.99f, box_height / 4, -0.1f,
        1.0f, box_height / 4, 0.0f
    };
    
    static const GLfloat circle2_vertex_buffer_data[] = {
        //0.0f, box_height / 2, 0.0f,
        0.0f, box_height-2.5, 0.0f,
        1.0f, box_height/3, 0.0f,
        0.99f, box_height/3, 0.1f,
        0.97f, box_height/3, 0.2f,
        0.95f, box_height/3, 0.3f,
        0.91f, box_height/3, 0.4f,
        0.86f, box_height/3, 0.5f,
        0.79f, box_height/3, 0.6f,
        0.71f, box_height/3, 0.7f,
        0.59f, box_height/3, 0.8f,
        0.43f, box_height/3, 0.9f,
        0.21f, box_height/3, 1.0f,
        0.0f, box_height/3, 1.0f,
        -0.21f, box_height/3, 1.0f,
        -0.43f, box_height/3, 0.9f,
        -0.59f, box_height/3, 0.8f,
        -0.71f, box_height/3, 0.7f,
        -0.79f, box_height/3, 0.6f,
        -0.86f, box_height/3, 0.5f,
        -0.91f, box_height/3, 0.4f,
        -0.95f, box_height/3, 0.3f,
        -0.97f, box_height/3, 0.2f,
        -0.99f, box_height/3, 0.1f,
        -1.0f, box_height/3, 0.0f,
        -1.0f, box_height/3, -0.0f,
        -0.99f, box_height/3, -0.1f,
        -0.97f, box_height/3, -0.2f,
        -0.95f, box_height/3, -0.3f,
        -0.91f, box_height/3, -0.4f,
        -0.86f, box_height/3, -0.5f,
        -0.79f, box_height/3, -0.6f,
        -0.71f, box_height/3, -0.7f,
        -0.59f, box_height/3, -0.8f,
        -0.43f, box_height/3, -0.9f,
        -0.21f, box_height/3, -1.0f,
        0.0f, box_height/3, -1.0f,
        0.21f, box_height/3, -1.0f,
        0.43f, box_height/3, -0.9f,
        0.59f, box_height/3, -0.8f,
        0.71f, box_height/3, -0.7f,
        0.79f, box_height/3, -0.6f,
        0.86f, box_height/3, -0.5f,
        0.91f, box_height/3, -0.4f,
        0.95f, box_height/3, -0.3f,
        0.97f, box_height/3, -0.2f,
        0.99f, box_height/3, -0.1f,
        1.0f, box_height/3, 0.0f,
    };

  static const GLfloat box_vertex_buffer_data[] = {
    0.0f, 0.0f, 0.0f,                      // X axis
    box_width, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,                      // Y axis
    0.0f, box_height, 0.0f,
    0.0f, 0.0f, 0.0f,                      // Z axis
    0.0f, 0.0f, box_depth,
    box_width, box_height, box_depth,      // other edges
    0.0f, box_height, box_depth,
    box_width, box_height, 0.0f,
    0.0f, box_height, 0.0f,
    box_width, 0.0f, box_depth,
    0.0f, 0.0f, box_depth,
    box_width, box_height, box_depth,
    box_width, 0.0f, box_depth,
    0.0f, box_height, box_depth,
    0.0f, 0.0f, box_depth,
    box_width, box_height, 0.0f,
    box_width, 0.0f, 0.0f,
    box_width, box_height, box_depth,
    box_width, box_height, 0.0f,
    0.0f, box_height, box_depth,
    0.0f, box_height, 0.0f,
    box_width, 0.0f, box_depth,
    box_width, 0.0f, 0.0f,
  };
 
    glGenBuffers(1, &quad_vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_buffer_data), quad_vertex_buffer_data, GL_STATIC_DRAW);
    
    glGenBuffers(1, &circle1_vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, circle1_vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(circle1_vertex_buffer_data), circle1_vertex_buffer_data, GL_STATIC_DRAW);
    
    glGenBuffers(1, &circle2_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, circle2_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(circle2_vertex_buffer_data), circle2_vertex_buffer_data, GL_STATIC_DRAW);
    
    glGenBuffers(1, &box_vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, box_vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(box_vertex_buffer_data), box_vertex_buffer_data, GL_STATIC_DRAW);

  // "axis" edges are colored, rest are gray
  static const GLfloat box_color_buffer_data[] = { 
    1.0f, 0.0f, 0.0f,       // X axis is red 
    1.0f, 0.0f, 0.0f,        
    0.0f, 1.0f, 0.0f,       // Y axis is green 
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f,       // Z axis is blue
    0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.5f,       // all other edges gray
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
  };
    
    static const GLfloat quad_color_buffer_data[] = {
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f
    };
    
    static const GLfloat circle1_color_buffer_data[] = {
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f
    };
    
    static const GLfloat circle2_color_buffer_data[] = {
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

  glGenBuffers(1, &quad_colorbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, quad_colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad_color_buffer_data), quad_color_buffer_data, GL_STATIC_DRAW);
    
  glGenBuffers(1, &circle1_colorbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, circle1_colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(circle1_color_buffer_data), circle1_color_buffer_data, GL_STATIC_DRAW);
    
  glGenBuffers(1, &circle2_colorbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, circle2_colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(circle2_color_buffer_data), circle2_color_buffer_data, GL_STATIC_DRAW);
    
  glGenBuffers(1, &box_colorbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, box_colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(box_color_buffer_data), box_color_buffer_data, GL_STATIC_DRAW);

  //  initialize_random();
  flocker_squared_distance.resize(num_flockers);
  flocker_array.clear();

  for (int i = 0; i < num_flockers; i++) {
    float num = rand() % 360;
    flocker_array.push_back(new Flocker(i,
                      uniform_random(0, box_width), uniform_random(0, box_height), uniform_random(0, box_depth),
                      uniform_random(-0.01, 0.01), uniform_random(-0.01, 0.01), uniform_random(-0.01, 0.01),
                      0.002,            // randomness
                      0.05, 0.5, uniform_random(0.01, 0.03),  // min, max separation distance, weight
                      0.5,  1.0, uniform_random(0.0005, 0.002), // min, max alignment distance, weight
                      1.0,  1.5, uniform_random(0.0005, 0.002), // min, max cohesion distance, weight
                      5.0, // min, max fear distance, weight
                      num,
                      1.0,  1.0, 1.0,
                      flocker_history_length));

    flocker_squared_distance[i].resize(num_flockers);
  }
    
    // For predator
    predator_array.clear();
    
    predator_array.push_back(new Predator(0,
                        uniform_random(0, box_width), uniform_random(0, box_height), uniform_random(0, box_depth),
                        uniform_random(-0.01, 0.01), uniform_random(-0.01, 0.01), uniform_random(-0.01, 0.01),
                        0.002,            // randomness
                        2.9,              // hunger weight
                        1.0,  0.0, 1.0,
                        predator_history_length));
    
    
 
}
//----------------------------------------------------------------------------

// move creatures around -- no drawing
void update_flocking_simulation()
{
  int i;

  // precalculate inter-flocker distances
  calculate_flocker_squared_distances();

  // get new_position, new_velocity for each flocker
  for (i = 0; i < flocker_array.size(); i++)
    flocker_array[i]->update();
    
  for (i = 0; i < predator_array.size(); i++)
    predator_array[i]->update();

  // handle wrapping and make new position, velocity into current
  for (i = 0; i < flocker_array.size(); i++)
    flocker_array[i]->finalize_update(box_width, box_height, box_depth);

  for (i = 0; i < predator_array.size(); i++)
    predator_array[i]->finalize_update(box_width, box_height, box_depth);
    
}

//----------------------------------------------------------------------------

// place the camera here
void setup_camera()
{
  ProjectionMat = glm::perspective(50.0f, (float) win_w / (float) win_h, 0.1f, 35.0f);

  if (camera_mode == CAMERA_MODE_ORBIT) {
    
    double orbit_cam_azimuth = glm::radians(orbit_cam_longitude_degs);
    double orbit_cam_inclination = glm::radians(90.0 - orbit_cam_latitude_degs);
    
    double x_cam = orbit_cam_radius * sin(orbit_cam_inclination) * cos(orbit_cam_azimuth); // 0.5 * box_width;
    double z_cam = orbit_cam_radius * sin(orbit_cam_inclination) * sin(orbit_cam_azimuth); // 0.5 * box_height;
    double y_cam = orbit_cam_radius * cos(orbit_cam_inclination); // 15.0;

    ViewMat = glm::lookAt(glm::vec3(x_cam, y_cam, z_cam),   // Camera location in World Space
			  glm::vec3(0,0,0),                 // and looks at the origin
			  glm::vec3(0,-1,0)                  // Head is up (set to 0,-1,0 to look upside-down)
			  );
  }
  else {
    printf("only orbit camera mode currently supported\n");
    exit(1);
  }
}

//----------------------------------------------------------------------------

// no error-checking -- you have been warned...
void load_objects_and_textures(int argc, char **argv)
{
  if (argc == 1) {
    loadOBJ("cube.obj", obj_vertices, obj_uvs, obj_normals);
    obj_Texture = loadBMP_custom("flame.bmp");
  }
  else if (!strcmp("cube", argv[1])) {
    loadOBJ("cube.obj", obj_vertices, obj_uvs, obj_normals);
    obj_Texture = loadDDS("uvmap.DDS");
  }
  else if (!strcmp("suzanne", argv[1])) {
    loadOBJ("suzanne.obj", obj_vertices, obj_uvs, obj_normals);
    obj_Texture = loadDDS("uvmap.DDS");
  }
  else if (!strcmp("banana", argv[1])) {
    loadOBJ("banana.obj", obj_vertices, obj_uvs, obj_normals);
    obj_Texture = loadBMP_custom("banana.bmp");
  }
  else {
    printf("unsupported object\n");
    exit(1);
  }

  // if not doing bullet demo, scale down objects to creature size -- kind of eye-balled this
  if (argc < 3) {
    for (int i = 0; i < obj_vertices.size(); i++) {
      obj_vertices[i].x *= 0.15;
      obj_vertices[i].y *= 0.15;
      obj_vertices[i].z *= 0.15;
    }
  }

  indexVBO(obj_vertices, obj_uvs, obj_normals, obj_indices, obj_indexed_vertices, obj_indexed_uvs, obj_indexed_normals);

  // Load into array buffers
  glGenBuffers(1, &obj_vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, obj_vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, obj_indexed_vertices.size() * sizeof(glm::vec3), &obj_indexed_vertices[0], GL_STATIC_DRAW);
  
  glGenBuffers(1, &obj_uvbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, obj_uvbuffer);
  glBufferData(GL_ARRAY_BUFFER, obj_indexed_uvs.size() * sizeof(glm::vec2), &obj_indexed_uvs[0], GL_STATIC_DRAW);
  
  glGenBuffers(1, &obj_normalbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, obj_normalbuffer);
  glBufferData(GL_ARRAY_BUFFER, obj_indexed_normals.size() * sizeof(glm::vec3), &obj_indexed_normals[0], GL_STATIC_DRAW);
  
  // Generate a buffer for the indices as well
  glGenBuffers(1, &obj_elementbuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj_elementbuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj_indices.size() * sizeof(unsigned short), &obj_indices[0] , GL_STATIC_DRAW);
}

void generate_textured_cube(){
    // Create and compile our GLSL program from the shaders
    textureprogramID = LoadShaders( "TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader" );
    
    // Get a handle for our "MVP" uniform
    GLuint MatrixID = glGetUniformLocation(textureprogramID, "MVP");
    
    // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(50.0f, (float) win_w / (float) win_h, 0.1f, 35.0f);
    
    // Camera matrix
    glm::mat4 View       = glm::lookAt(
                                       glm::vec3(14,13,13), // Camera is at (4,3,3), in World Space
                                       glm::vec3(0,0,0), // and looks at the origin
                                       glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                                       );
    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model      = glm::mat4(1.0f);
    
    glm::mat4 RotationMatrix = glm::mat4(); // identity    -- glm::toMat4(quat_orientations[i]);
    RotationMatrix = glm::rotate(RotationMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 1.0f));
//    TranslationMatrix = glm::translate(glm::vec3(box_width/2, box_height-0.25, box_depth/2))
    Model = Model * RotationMatrix * glm::translate(glm::vec3(3.0, 1.0, 0.0));
    
    // Our ModelViewProjection : multiplication of our 3 matrices
    TMVP        = Projection * View * Model; // Remember, matrix multiplication is the other way around
    
    // Load the texture using any two methods
    Texture = loadBMP_custom("stonewall.bmp");
//    Texture = loadDDS("uvtemplate.DDS");
    
    // Get a handle for our "myTextureSampler" uniform
    TextureID  = glGetUniformLocation(textureprogramID, "myTextureSampler");
    
    // Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
    // A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
    static const GLfloat g_vertex_buffer_data[] = {
        -0.5,-0.5,-0.5,
        -0.5,-0.5, 0.5,
        -0.5, 0.5, 0.5,
        0.5, 0.5,-0.5,
        -0.5,-0.5,-0.5,
        -0.5, 0.5,-0.5,
        0.5,-0.5, 0.5,
        -0.5,-0.5,-0.5,
        0.5,-0.5,-0.5,
        0.5, 0.5,-0.5,
        0.5,-0.5,-0.5,
        -0.5,-0.5,-0.5,
        -0.5,-0.5,-0.5,
        -0.5, 0.5, 0.5,
        -0.5, 0.5,-0.5,
        0.5,-0.5, 0.5,
        -0.5,-0.5, 0.5,
        -0.5,-0.5,-0.5,
        -0.5, 0.5, 0.5,
        -0.5,-0.5, 0.5,
        0.5,-0.5, 0.5,
        0.5, 0.5, 0.5,
        0.5,-0.5,-0.5,
        0.5, 0.5,-0.5,
        0.5,-0.5,-0.5,
        0.5, 0.5, 0.5,
        0.5,-0.5, 0.5,
        0.5, 0.5, 0.5,
        0.5, 0.5,-0.5,
        -0.5, 0.5,-0.5,
        0.5, 0.5, 0.5,
        -0.5, 0.5,-0.5,
        -0.5, 0.5, 0.5,
        0.5, 0.5, 0.5,
        -0.5, 0.5, 0.5,
        0.5,-0.5, 0.5
    };
    
    static const GLfloat g_uv_buffer_data[] = {
        0.000059f, 1.0f-0.000004f,
        0.000103f, 1.0f-0.336048f,
        0.335973f, 1.0f-0.335903f,
        1.000023f, 1.0f-0.000013f,
        0.667979f, 1.0f-0.335851f,
        0.999958f, 1.0f-0.336064f,
        0.667979f, 1.0f-0.335851f,
        0.336024f, 1.0f-0.671877f,
        0.667969f, 1.0f-0.671889f,
        1.000023f, 1.0f-0.000013f,
        0.668104f, 1.0f-0.000013f,
        0.667979f, 1.0f-0.335851f,
        0.000059f, 1.0f-0.000004f,
        0.335973f, 1.0f-0.335903f,
        0.336098f, 1.0f-0.000071f,
        0.667979f, 1.0f-0.335851f,
        0.335973f, 1.0f-0.335903f,
        0.336024f, 1.0f-0.671877f,
        1.000004f, 1.0f-0.671847f,
        0.999958f, 1.0f-0.336064f,
        0.667979f, 1.0f-0.335851f,
        0.668104f, 1.0f-0.000013f,
        0.335973f, 1.0f-0.335903f,
        0.667979f, 1.0f-0.335851f,
        0.335973f, 1.0f-0.335903f,
        0.668104f, 1.0f-0.000013f,
        0.336098f, 1.0f-0.000071f,
        0.000103f, 1.0f-0.336048f,
        0.000004f, 1.0f-0.671870f,
        0.336024f, 1.0f-0.671877f,
        0.000103f, 1.0f-0.336048f,
        0.336024f, 1.0f-0.671877f,
        0.335973f, 1.0f-0.335903f,
        0.667969f, 1.0f-0.671889f,
        1.000004f, 1.0f-0.671847f,
        0.667979f, 1.0f-0.335851f
    };
    
    vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    
    uvbuffer;
    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);

}

void draw_textured_cube(){
    glUseProgram(textureprogramID);
    
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &TMVP[0][0]);
    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    // Set our "myTextureSampler" sampler to user Texture Unit 0
    glUniform1i(TextureID, 0);
    
    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
                          0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                          3,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );
    
    // 2nd attribute buffer : UVs
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glVertexAttribPointer(
                          1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                          2,                                // size : U+V => 2
                          GL_FLOAT,                         // type
                          GL_FALSE,                         // normalized?
                          0,                                // stride
                          (void*)0                          // array buffer offset
                          );
    
    // Draw the triangle !
    glDrawArrays(GL_TRIANGLES, 0, 12*3); // 12*3 indices starting at 0 -> 12 triangles
    
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

//----------------------------------------------------------------------------

int main(int argc, char **argv)
{  
  // Initialise GLFW
  if( !glfwInit() )
    {
      fprintf( stderr, "Failed to initialize GLFW\n" );
      getchar();
      return -1;
    }
  
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 
  
  // Open a window and create its OpenGL context
  window = glfwCreateWindow(win_w, win_h, "creatures", NULL, NULL);
  if( window == NULL ){
    fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
    getchar();
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  
  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    getchar();
    glfwTerminate();
    return -1;
  }

  // background color, depth testing
  glClearColor(0.0f, 0.0f, 0.1f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS); 
  //  glEnable(GL_CULL_FACE);  // we don't want this enabled when drawing the creatures as floating triangles

  // vertex arrays
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  // Create and compile our GLSL program from the shaders

  // obj rendering mode uses shaders that came with bullet demo program
  objprogramID = LoadShaders( "StandardShading.vertexshader", "StandardShading.fragmentshader" );

  // all other rendering modes
  programID = LoadShaders( "Creatures.vertexshader", "Creatures.fragmentshader" );

  // Get handles for our uniform variables
  MatrixID = glGetUniformLocation(programID, "MVP");
  ViewMatrixID = glGetUniformLocation(programID, "V");
  ModelMatrixID = glGetUniformLocation(programID, "M");

  objMatrixID = glGetUniformLocation(objprogramID, "MVP");
  objViewMatrixID = glGetUniformLocation(objprogramID, "V");
  objModelMatrixID = glGetUniformLocation(objprogramID, "M");

  objTextureID  = glGetUniformLocation(objprogramID, "myTextureSampler");
  objLightID = glGetUniformLocation(objprogramID, "LightPosition_worldspace");
    
    //for billboard purpose
    
    // Create and compile our GLSL program from the shaders
    billprogramID = LoadShaders("Billboard.vertexshader", "Billboard.fragmentshader");
    
    // Vertex shader
    CameraRight_worldspace_ID = glGetUniformLocation(billprogramID, "CameraRight_worldspace");
    CameraUp_worldspace_ID = glGetUniformLocation(billprogramID, "CameraUp_worldspace");
    ViewProjMatrixID = glGetUniformLocation(billprogramID, "VP");
    BillboardPosID = glGetUniformLocation(billprogramID, "BillboardPos");
    BillboardSizeID = glGetUniformLocation(billprogramID, "BillboardSize");
    LifeLevelID = glGetUniformLocation(billprogramID, "LifeLevel");
    
    billTextureID = glGetUniformLocation(billprogramID, "myTextureSampler");
    
    billTexture = loadDDS("ExampleBillboard.DDS");
    
// ----------------------------------------------------------------------------------------------------------------------------------
    generate_textured_cube();
//----------------------------------------------------------------------------------------------------------------------------------
    
  // Use our shader
  glUseProgram(programID);

  // register all callbacks
  glfwSetKeyCallback(window, key_callback);

  // objects, textures
  load_objects_and_textures(argc, argv);
    
  for (int i = 0; i < num_flockers; ++i) {
      dead_flocker_array[i] = false;
  }

  // simulation
  initialize_random();
  initialize_flocking_simulation();

  // run a whole different program if bullet demo option selected
  if (argc == 3) { 
    bullet_hello_main(argc, argv);
    return 1;
  }
  
  // timing stuff
  double currentTime, lastTime;
  double target_period = 1.0 / target_FPS;
  
  lastTime = glfwGetTime();
  // enter simulate-render loop (with event handling)
    
  int nbFrames = 0;
    
  initText2D("Holstein.DDS");
  
  do {
        // STEP THE SIMULATION -- EITHER FLOCKING OR PHYSICS
        currentTime = glfwGetTime();
        nbFrames++;
        if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1sec ago
            // printf and reset
            printf("%f ms/frame\n", 1000.0/double(nbFrames));
            nbFrames = 0;
            lastTime += 1.0;
        }
      
        if (!is_paused) {
          if (!is_physics_active)
        update_flocking_simulation();
          else
        update_physics_simulation(target_period);
        }
      
        // RENDER IT

        // Clear the screen
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      
        // set model transform and color for each triangle and draw it offscreen
        setup_camera();

        glm::mat4 M = glm::translate(glm::vec3(-0.5f * box_width, -0.5f * box_height, -0.5f * box_depth));

        // draw box and creatures
        glUseProgram(programID);
      
        draw_quad(M * glm::translate(glm::vec3(box_width/4, 0.0f, box_depth/2)));
        draw_bottom_circle(M * glm::translate(glm::vec3(box_width/4, 0.0f, box_depth/2)));
        draw_top_circle(M * glm::translate(glm::vec3(box_width/4, 0.0f, box_depth/2)));
      
        draw_box(M);
      
        draw_textured_cube();

//        glm::mat4 RotationMatrix = glm::mat4(); // identity    -- glm::toMat4(quat_orientations[i]);
//        RotationMatrix = glm::rotate(RotationMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
//        draw_quad(M * glm::translate(glm::vec3(box_width/2, box_height/2, box_depth/2)) * RotationMatrix);
      
        glUseProgram(billprogramID);
        draw_billboard(M * glm::translate(glm::vec3(box_width/2, box_height-0.25, box_depth/2)));

      if (using_obj_program){
          glUseProgram(objprogramID);
      }
      else{
          glUseProgram(programID);
      }

        for (int i = 0; i < flocker_array.size(); i++){
            if(!dead_flocker_array[i]){
                flocker_array[i]->draw(M);
            }
        }
      
        for (int i = 0; i < predator_array.size(); i++)
          predator_array[i]->draw(M);

        // busy wait if we are going too fast
        do {
          currentTime = glfwGetTime();
        } while (currentTime - lastTime < target_period);

        lastTime = currentTime;
     
        char score_text[3];
        sprintf(score_text, "Score: %d", score);
        printText2D(score_text, 320, 520, 20);
        //cleanupText2D();
      
        if(LifeLevel <= 0.0){
            char game_over_text[256];
            sprintf(game_over_text, "Game Over!");
            printText2D(game_over_text, 120, 300, 50);
            isGameOver = true;
            gameOver();
            is_physics_active = true;
//            update_physics_simulation(target_period);
        }
      
        // Swap buffers
        glfwSwapBuffers(window);
      
        glfwPollEvents();

  } while ( glfwWindowShouldClose(window) == 0);
    cleanupText2D();
  
  end_program();
  
  return 0;
}

//----------------------------------------------------------------------------
