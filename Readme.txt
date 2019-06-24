
Game:
# It is a chase and run game. User could control the predator by keyboard inputs and attempt to eat the flocker. User could move in all X, Y and Z directions.
# If the predator eats a flocker the score will increase.
# By eating a flocker the predator will stay alive (life bar) for a longer time. Otherwise along the time life of the predator will decrease. When it's 0, the game will be over.
  
Our game has the following characteristics:
- Our game environment consists primarily of 3-D objects (for example, a complicated 3-d obstacle drawn by meshing up one quadrilateral and two cones) . The view of the environment is closer to isometric.
- User control through keyboard input (moves the predator)
- Objects move smoothly, camera views change smoothy 
- At least one texture-mapped element (stone mapped cube as an obstacle)


Additional elements:

- Bullet physics: A cylindrical 3-D object and stone wall mapped cube are added as obstacles using bullet physics.
- Billboards is implemented as a life bar for predator.
- A scoreboard to count the number of flockers eaten by the predator.
- "Complicated" 3-D objects (A cylindrical 3-D object with cones is added)
