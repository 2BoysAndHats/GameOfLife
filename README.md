# GameOfLife
### a compute-shader based implementation of Conway's game of life

![Demo GIF of the Game of Life](images/gameoflife.gif)

I've always wanted to dig into low-level graphics programming, and compute shaders in particular. They've always been slightly magical to me - massively parallized mini-programs that are able to handle computationally intensive tasks with ease.

Whilst [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway's_Game_of_Life) might not exactly be the heaviest possible example, it serves as a nice introduction to the basics of using compute shaders in OpenGL (while making some cool looking patterns along the way, which is always a plus!)

## Requirements
* Visual Studio 2019
* A graphics driver that supports OpenGL 4.3 or above (pretty much any GPU / internal graphics chipset released in the last decade)

All the dependencies should be included in the repository - simply open the solution file and build the project.

The project takes its inital state in from an image file (inital_setup.png). Right now, there's no way to edit the state in-app - you'll need an image editor like [GIMP](https://www.gimp.org/) to make changed to the inital state.

## Implementation
The real meat and potatoes of the Game of Life implementation is in [gameoflife.comp](GameOfLife/gameoflife.comp). This is the shader that runs for every cell in the grid, calculates the number of neighbours they currently have, then outputting the new state based on the number of neighbours. As such, it should be easy to adapt to similar Cellular Automatons that rely on the number of neighbours to calculate the next state.

[GameOfLife.cpp](GameOfLife/GameOfLife.cpp) contains all of the boilerplate code required to instansiate a window, load in the shaders, and call the compute shader every frame.

[quad.vert](GameOfLife/quad.vert) and [quad.frag](GameOfLife/quad.frag) are a simple vertex and fragment shader responsible for rendering the grid texture to the screen.