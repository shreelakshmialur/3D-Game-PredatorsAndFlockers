#include "Flocker.hh"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int flocker_history_length = 30;
int flocker_draw_mode = DRAW_MODE_POLY;
vector <Flocker *> flocker_array;
extern vector <Predator *> predator_array;
vector <vector <double> > flocker_squared_distance;
vector <double> flocker_predator_distance;
float minDistanceForPredators = 10.0f;

extern glm::mat4 ViewMat;
extern glm::mat4 ProjectionMat;

extern GLuint ModelMatrixID;
extern GLuint ViewMatrixID;
extern GLuint MatrixID;

extern GLuint objModelMatrixID;
extern GLuint objViewMatrixID;
extern GLuint objMatrixID;
extern GLuint objLightID;
extern GLuint objTextureID;

extern GLuint obj_Texture;
extern GLuint obj_vertexbuffer;
extern GLuint obj_uvbuffer;
extern GLuint obj_normalbuffer;
extern GLuint obj_elementbuffer;
extern vector<unsigned short> obj_indices;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// attempt to be slightly efficient by pre-calculating all of the distances between
// pairs of flockers exactly once

void calculate_flocker_squared_distances()
{
  int i, j;
  glm::vec3 diff;

  for (i = 0; i < flocker_array.size(); i++)
    for (j = i + 1; j < flocker_array.size(); j++) {
      diff = flocker_array[i]->position - flocker_array[j]->position;
      flocker_squared_distance[i][j] = glm::length2(diff);
    }
}

void calculate_flocker_predator_distance()
{
    int i;
    glm::vec3 diff;
    
    for (i = 0; i < flocker_array.size(); i++){
        diff = predator_array[0]->position - flocker_array[i]->position;
        flocker_predator_distance[i] = glm::length(diff);
    }
}

//----------------------------------------------------------------------------

Flocker::Flocker(int _index,
		 double init_x, double init_y, double init_z,
		 double init_vx, double init_vy, double init_vz,
		 double rand_force_limit,
		 double min_separate_distance, double max_separate_distance,  double separate_weight,
		 double min_align_distance, double max_align_distance, double align_weight,
		 double min_cohere_distance, double max_cohere_distance, double cohere_weight,
         double fear_weight,
         float random,
		 float r, float g, float b,
		 int max_hist) : Creature(_index, init_x, init_y, init_z, init_vx, init_vy, init_vz, r, g, b, max_hist)
{ 
  random_force_limit = rand_force_limit;

  separation_weight = separate_weight;

  min_separation_distance = min_separate_distance;
  min_squared_separation_distance = min_separation_distance * min_separation_distance;

  max_separation_distance = max_separate_distance;
  max_squared_separation_distance = max_separation_distance * max_separation_distance;

  inv_range_squared_separation_distance = 1.0 / (max_squared_separation_distance - min_squared_separation_distance);

  alignment_weight = align_weight;

  min_alignment_distance = min_align_distance;
  min_squared_alignment_distance = min_alignment_distance * min_alignment_distance;

  max_alignment_distance = max_align_distance;
  max_squared_alignment_distance = max_alignment_distance * max_alignment_distance;

  inv_range_squared_alignment_distance = 1.0 / (max_squared_alignment_distance - min_squared_alignment_distance);

  cohesion_weight = cohere_weight;

  min_cohesion_distance = min_cohere_distance;
  min_squared_cohesion_distance = min_cohesion_distance * min_cohesion_distance;

  max_cohesion_distance = max_cohere_distance;
  max_squared_cohesion_distance = max_cohesion_distance * max_cohesion_distance;

  inv_range_squared_cohesion_distance = 1.0 / (max_squared_cohesion_distance - min_squared_cohesion_distance);
    
  rotation_radian = random;

}

//----------------------------------------------------------------------------

// Model allows arbitrary transform on Flocker when drawing -- should be the same for all of them
// (don't use it for position)

void Flocker::draw(glm::mat4 Model)
{
  if (flocker_draw_mode == DRAW_MODE_OBJ) {

    // set light position
    
    glm::vec3 lightPos = glm::vec3(4,4,4);
    glUniform3f(objLightID, lightPos.x, lightPos.y, lightPos.z);

    glm::mat4 RotationMatrix = glm::mat4(); // identity    -- glm::toMat4(quat_orientations[i]);
    
    RotationMatrix = glm::rotate(RotationMatrix, glm::radians(rotation_radian), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 TranslationMatrix = translate(glm::mat4(), glm::vec3(position.x, position.y, position.z));
    glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix;

    glm::mat4 MVP = ProjectionMat * ViewMat * Model * ModelMatrix;

    // Send our transformation to the currently bound shader, 
    // in the "MVP" uniform
    glUniformMatrix4fv(objMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(objModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
    glUniformMatrix4fv(objViewMatrixID, 1, GL_FALSE, &ViewMat[0][0]);

    // Bind this object's texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, obj_Texture);
    // Set our "myTextureSampler" sampler to use Texture Unit 0
    glUniform1i(objTextureID, 0);

    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
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
    glEnableVertexAttribArray(1);
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
    glEnableVertexAttribArray(2);
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
      

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    return;
  }


  int draw_mode, num_vertices;
  
  GLfloat *vertex_buffer_data;
  GLfloat *color_buffer_data;
   
  // position "trail" using history

  if (flocker_draw_mode == DRAW_MODE_HISTORY) {

    num_vertices = position_history.size();
    draw_mode = GL_POINTS;

    vertex_buffer_data = (GLfloat *) malloc(3 * num_vertices * sizeof(GLfloat));
    color_buffer_data = (GLfloat *) malloc(3 * num_vertices * sizeof(GLfloat));

    glPointSize(5);

    float inv_size = 1.0 / position_history.size();
    
    float index = position_history.size();
    int i = 0;
    for (deque<glm::vec3>::iterator it = position_history.begin(); it!=position_history.end(); ++it) {

      color_buffer_data[3 * i]     = draw_color.r * index * inv_size;
      color_buffer_data[3 * i + 1] = draw_color.g * index * inv_size;
      color_buffer_data[3 * i + 2] = draw_color.b * index * inv_size;

      vertex_buffer_data[3 * i]     = (*it).x;
      vertex_buffer_data[3 * i + 1] = (*it).y;
      vertex_buffer_data[3 * i + 2] = (*it).z;

      index--;
      i++;
    }

  }
  else if (flocker_draw_mode == DRAW_MODE_AXES) {

    num_vertices = 6;
    draw_mode = GL_LINES;

    vertex_buffer_data = (GLfloat *) malloc(3 * num_vertices * sizeof(GLfloat));
    color_buffer_data = (GLfloat *) malloc(3 * num_vertices * sizeof(GLfloat));

    double axis_scale = 0.25;

    // vertices

    vertex_buffer_data[0] = position.x;
    vertex_buffer_data[1] = position.y;
    vertex_buffer_data[2] = position.z;

    vertex_buffer_data[3] = position.x + axis_scale * frame_x.x;
    vertex_buffer_data[4] = position.y + axis_scale * frame_x.y;
    vertex_buffer_data[5] = position.z + axis_scale * frame_x.z;

    vertex_buffer_data[6] = position.x;
    vertex_buffer_data[7] = position.y;
    vertex_buffer_data[8] = position.z;

    vertex_buffer_data[9] = position.x + axis_scale * frame_y.x;
    vertex_buffer_data[10] = position.y + axis_scale * frame_y.y;
    vertex_buffer_data[11] = position.z + axis_scale * frame_y.z;

    vertex_buffer_data[12] = position.x;
    vertex_buffer_data[13] = position.y;
    vertex_buffer_data[14] = position.z;

    vertex_buffer_data[15] = position.x + axis_scale * frame_z.x;
    vertex_buffer_data[16] = position.y + axis_scale * frame_z.y;
    vertex_buffer_data[17] = position.z + axis_scale * frame_z.z;

    // color

    color_buffer_data[0] = 1.0f;
    color_buffer_data[1] = 0.0f;
    color_buffer_data[2] = 0.0f;

    color_buffer_data[3] = 1.0f;
    color_buffer_data[4] = 0.0f;
    color_buffer_data[5] = 0.0f;

    color_buffer_data[6] = 0.0f;
    color_buffer_data[7] = 1.0f;
    color_buffer_data[8] = 0.0f;

    color_buffer_data[9] = 0.0f;
    color_buffer_data[10] = 1.0f;
    color_buffer_data[11] = 0.0f;

    color_buffer_data[12] = 0.0f;
    color_buffer_data[13] = 0.0f;
    color_buffer_data[14] = 1.0f;

    color_buffer_data[15] = 0.0f;
    color_buffer_data[16] = 0.0f;
    color_buffer_data[17] = 1.0f;
    
  }
  else if (flocker_draw_mode == DRAW_MODE_POLY) {

    num_vertices = 12;
    draw_mode = GL_TRIANGLES;

    vertex_buffer_data = (GLfloat *) malloc(3 * num_vertices * sizeof(GLfloat));
    color_buffer_data = (GLfloat *) malloc(3 * num_vertices * sizeof(GLfloat));
    
    double width = 0.2f;
    double height = 0.2f;
    double length = 0.3f;
  
    // horizontal
    
    vertex_buffer_data[0] = position.x - 0.5f * width * frame_x.x; 
    vertex_buffer_data[1] = position.y - 0.5f * width * frame_x.y; 
    vertex_buffer_data[2] = position.z - 0.5f * width * frame_x.z;
    
    vertex_buffer_data[3] = position.x - 0.8f * length * frame_z.x; 
    vertex_buffer_data[4] = position.y - 0.8f * length * frame_z.y; 
    vertex_buffer_data[5] = position.z - 0.8f * length * frame_z.z;
        
    vertex_buffer_data[6] = position.x + 0.2f * length * frame_z.x; 
    vertex_buffer_data[7] = position.y + 0.2f * length * frame_z.y; 
    vertex_buffer_data[8] = position.z + 0.2f * length * frame_z.z;
    

    vertex_buffer_data[9] = position.x + 0.5f * width * frame_x.x; 
    vertex_buffer_data[10] = position.y + 0.5f * width * frame_x.y; 
    vertex_buffer_data[11] = position.z + 0.5f * width * frame_x.z;

    vertex_buffer_data[12] = position.x + 0.2f * length * frame_z.x; 
    vertex_buffer_data[13] = position.y + 0.2f * length * frame_z.y; 
    vertex_buffer_data[14] = position.z + 0.2f * length * frame_z.z;

    vertex_buffer_data[15] = position.x - 0.8f * length * frame_z.x; 
    vertex_buffer_data[16] = position.y - 0.8f * length * frame_z.y; 
    vertex_buffer_data[17] = position.z - 0.8f * length * frame_z.z;

    // vertical
    
    vertex_buffer_data[18] = position.x + 0.5f * height * frame_y.x;
    vertex_buffer_data[19] = position.y + 0.5f * height * frame_y.y; 
    vertex_buffer_data[20] = position.z + 0.5f * height * frame_y.z;
    
    vertex_buffer_data[21] = position.x - 0.8f * length * frame_z.x; 
    vertex_buffer_data[22] = position.y - 0.8f * length * frame_z.y; 
    vertex_buffer_data[23] = position.z - 0.8f * length * frame_z.z;

    vertex_buffer_data[24] = position.x + 0.2f * length * frame_z.x; 
    vertex_buffer_data[25] = position.y + 0.2f * length * frame_z.y; 
    vertex_buffer_data[26] = position.z + 0.2f * length * frame_z.z;

    
    vertex_buffer_data[27] = position.x - 0.5f * height * frame_y.x;
    vertex_buffer_data[28] = position.y - 0.5f * height * frame_y.y; 
    vertex_buffer_data[29] = position.z - 0.5f * height * frame_y.z;

    vertex_buffer_data[30] = position.x + 0.2f * length * frame_z.x; 
    vertex_buffer_data[31] = position.y + 0.2f * length * frame_z.y; 
    vertex_buffer_data[32] = position.z + 0.2f * length * frame_z.z;
   
    vertex_buffer_data[33] = position.x - 0.8f * length * frame_z.x; 
    vertex_buffer_data[34] = position.y - 0.8f * length * frame_z.y; 
    vertex_buffer_data[35] = position.z - 0.8f * length * frame_z.z;
    
    // color

    int i;

    for (i = 0; i < 6; i++) {
      color_buffer_data[3 * i]     = 1.0f;
      color_buffer_data[3 * i + 1] = 0.0f;
      color_buffer_data[3 * i + 2] = 0.0f;
    }

    for (i = 6; i < 9; i++) {
      color_buffer_data[3 * i]     = 0.0f;
      color_buffer_data[3 * i + 1] = 0.0f;
      color_buffer_data[3 * i + 2] = 1.0f;
    }

    for (i = 9; i < 12; i++) {
      color_buffer_data[3 * i]     = 0.0f;
      color_buffer_data[3 * i + 1] = 1.0f;
      color_buffer_data[3 * i + 2] = 0.0f;
    }
  }

  // Our ModelViewProjection : multiplication of our 3 matrices
  
  glm::mat4 MVP = ProjectionMat * ViewMat * Model;
  
  // make this transform available to shaders  
  
  glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
  
  // 1st attribute buffer : vertices
  
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, 3 * num_vertices * sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW);
  
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glVertexAttribPointer(0,                  // attribute. 0 to match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
  
  // 2nd attribute buffer : colors
  
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, 3 * num_vertices * sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);
  
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glVertexAttribPointer(1,                                // attribute. 1 to match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);
  
  // Draw the flocker!
  
  glDrawArrays(draw_mode, 0, num_vertices); 
  
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  
  free(vertex_buffer_data);
  free(color_buffer_data);
}

//----------------------------------------------------------------------------

// based on:
// http://processing.org/examples/flocking
// http://libcinder.org/docs/dev/flocking_chapter2.html

// side effect is putting values into SEPARATION_FORCE vector

bool Flocker::compute_separation_force()
{
  int j;
  glm::vec3 direction;
  int count = 0;
  double mag;
  double F;

  separation_force = glm::vec3(0, 0, 0);

  for (j = index + 1; j < flocker_squared_distance.size(); j++)
    if (flocker_squared_distance[index][j] >= min_squared_separation_distance &&
	flocker_squared_distance[index][j] <= max_squared_separation_distance) {

      // set (unweighted) force magnitude

      F = max_squared_separation_distance / flocker_squared_distance[index][j] - 1.0;

      // set force direction

      direction = (float) F * glm::normalize(position - flocker_array[j]->position);
      separation_force += direction;
      count++;
    }
  for (j = index - 1; j >= 0; j--)
    if (flocker_squared_distance[j][index] >= min_squared_separation_distance && 
	flocker_squared_distance[j][index] <= max_squared_separation_distance) {

      // set (unweighted) force magnitude

      F = max_squared_separation_distance / flocker_squared_distance[j][index] - 1.0;

      // set force direction

      direction = (float) F * glm::normalize(position - flocker_array[j]->position);
      separation_force += direction;
      count++;
    }

  if (count > 0) {
    separation_force *= (float) separation_weight;
    return true;
  }
  else
    return false;
}

//----------------------------------------------------------------------------

// based on:
// http://processing.org/examples/flocking
// http://libcinder.org/docs/dev/flocking_chapter4.html

// side effect is putting values into ALIGNMENT_FORCE vector

bool Flocker::compute_alignment_force()
{
  int j;
  glm::vec3 direction;
  int count = 0;
  double mag, percent;
  double F;

  alignment_force = glm::vec3(0, 0, 0);

  for (j = index + 1; j < flocker_squared_distance.size(); j++)
    if (flocker_squared_distance[index][j] >= min_squared_alignment_distance &&
	flocker_squared_distance[index][j] <= max_squared_alignment_distance) {

      // set (unweighted) force magnitude

      percent = (flocker_squared_distance[index][j] - max_squared_alignment_distance) * inv_range_squared_alignment_distance;
      F = 0.5 + -0.5 * cos(percent * 2.0 * M_PI);

      // set force direction

      direction = (float) F * glm::normalize(flocker_array[j]->velocity);
      alignment_force += direction;
      count++;
    }
  for (j = index - 1; j >= 0; j--)
    if (flocker_squared_distance[j][index] >= min_squared_alignment_distance &&
	flocker_squared_distance[j][index] <= max_squared_alignment_distance) {

      // set (unweighted) force magnitude

      percent = (flocker_squared_distance[index][j] - max_squared_alignment_distance) * inv_range_squared_alignment_distance;
      F = 0.5 + -0.5 * cos(percent * 2.0 * M_PI);

      // set force direction

      direction = (float) F * glm::normalize(flocker_array[j]->velocity);
      alignment_force += direction;
      count++;
    }

  if (count > 0) {
    alignment_force *= (float) alignment_weight;
    return true;
  }
  else
    return false;
}

//----------------------------------------------------------------------------

// based on:
// http://processing.org/examples/flocking 
// http://libcinder.org/docs/dev/flocking_chapter3.html

// side effect is putting values into COHESION_FORCE vector

bool Flocker::compute_cohesion_force()
{
  int j;
  glm::vec3 direction;
  int count = 0;
  double mag, percent;
  double F;

  cohesion_force = glm::vec3(0, 0, 0);

  for (j = index + 1; j < flocker_squared_distance.size(); j++)
    if (flocker_squared_distance[index][j] >= min_squared_cohesion_distance &&
	flocker_squared_distance[index][j] <= max_squared_cohesion_distance) {

      // set (unweighted) force magnitude

      percent = (flocker_squared_distance[index][j] - max_squared_cohesion_distance) * inv_range_squared_cohesion_distance;
      F = 0.5 + -0.5 * cos(percent * 2.0 * M_PI);

      // set force direction

      direction = (float) F * glm::normalize(flocker_array[j]->position - position);   // opposite direction of separation
      cohesion_force += direction;
      count++;
    }
  for (j = index - 1; j >= 0; j--)
    if (flocker_squared_distance[j][index] >= min_squared_cohesion_distance &&
	flocker_squared_distance[j][index] <= max_squared_cohesion_distance) {

      // set (unweighted) force magnitude

      percent = (flocker_squared_distance[index][j] - max_squared_cohesion_distance) * inv_range_squared_cohesion_distance;
      F = 0.5 + -0.5 * cos(percent * 2.0 * M_PI);

      // set force direction

      direction = (float) F * glm::normalize(flocker_array[j]->position - position);   // opposite direction of separation
      cohesion_force += direction;
      count++;
    }

  if (count > 0) {
    cohesion_force *= (float) cohesion_weight;
    return true;
  }
  else
    return false;
}

int Flocker::findIfPredatorIsNear(glm::vec3 flockPosition){
    if(distance(flockPosition, predator_array[0]->position) < minDistanceForPredators){
        return 1;
    }
    return -1;
}

bool Flocker::compute_fear_force()
{
    glm::vec3 direction;
    int count = 0;
    
    fear_force = glm::vec3(0, 0, 0);
 
    float dist = distance(position, predator_array[0]->position);
    direction = (float) 0.1/dist * glm::normalize(position - predator_array[0]->position);
    if(abs(dist)<3){
        fear_force += direction;
        count++;
    }
    return true;
}

//----------------------------------------------------------------------------

// apply physics

void Flocker::update()
{
  // set accelerations (aka forces)

  acceleration = glm::vec3(0, 0, 0);
  
  // deterministic behaviors

//  compute_separation_force();
//  acceleration += separation_force;

//  compute_alignment_force();
//  acceleration += alignment_force;

//  compute_cohesion_force();
//  acceleration += cohesion_force;

//  compute_fear_force();
//  acceleration += fear_force;

  draw_color.r = glm::length(separation_force);
  draw_color.g = glm::length(alignment_force);
  draw_color.b = glm::length(cohesion_force);
  if (draw_color.r > 0 || draw_color.g > 0 || draw_color.b > 0)
    draw_color = glm::normalize(draw_color);
  else 
    draw_color = base_color;

  // randomness

  if (random_force_limit > 0.0) {
    acceleration.x += uniform_random(-random_force_limit, random_force_limit);
    acceleration.y += uniform_random(-random_force_limit, random_force_limit);
    acceleration.z += uniform_random(-random_force_limit, random_force_limit);
  }

  // update velocity

  new_velocity = velocity + acceleration;   // scale acceleration by dt?

  // limit velocity

  double mag = glm::length(new_velocity);
  if (mag > MAX_FLOCKER_SPEED)
    new_velocity *= (float) (MAX_FLOCKER_SPEED / mag); 

  // update position

  new_position = position + new_velocity;   // scale new_velocity by dt?
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
