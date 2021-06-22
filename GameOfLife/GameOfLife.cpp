/*
* GameOfLife.cpp
* 
* A simple implmentation of Conway's Game of Life (https://en.wikipedia.org/wiki/Conway's_Game_of_Life)
* written in C++ and OpenGL using compute shaders.
*
* Keyboard controls:
* - Space: play / pause simulation
* - T: single-step the simulation
* - W/A/S/D: pan the viewport
* - - / =: zoom the viewport
* 
* by ConorB
*/

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

// Constants
// The image file we load our inital board state from. This must have dimensions gridWidth x gridHeight
const char* INITAL_SETUP_PATH = "inital_setup.png";

// The dimensions of the window we render to
const int width = 800;
const int height = 800;

// The dimensions of the board we simulate
const int gridWidth = 400;
const int gridHeight = 400;

// Game state variables
// The zoom factor for our current viewport
float currentScale = 1;

// The current viewport offset
float currentXOffset = 0;
float currentYOffset = 0;

// A flag determining whether or not we automatically tick the simulation forward every frame
bool simulationIsRunning = true;

GLuint computeProgram, inputTexture, outputTexture;

// The vertices and UV coordinates of a quad
// Used to render the game state texture to the screen
float vertices[] = {
    // positions            // texture coordinates
     1.0f,  1.0f,  0.0f,    1.0f, 1.0f, // top right    
     1.0f, -1.0f,  0.0f,    1.0f, 0.0f, // bottom right
    -1.0f,  1.0f,  0.0f,    0.0f, 1.0f, // top left

     1.0f, -1.0f,  0.0f,    1.0f, 0.0f, // bottom right
    -1.0f, -1.0f,  0.0f,    0.0f, 0.0f, // bottom left
    -1.0f,  1.0f,  0.0f,    0.0f, 1.0f  // top left
};

static void errorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

GLuint loadCompileShader(const char* shaderPath, GLenum shaderType)
{
    // Load in our shader code
    std::string shaderSource;
    std::ifstream shaderFile;

    // Make sure an exception will be thrown if an error occurs
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        // Open the file, and read in the source to a stream
        shaderFile.open(shaderPath);
        std::stringstream shaderSourceStream;
        shaderSourceStream << shaderFile.rdbuf();
        
        // Close the file, and convert to a std::string
        shaderFile.close();
        shaderSource = shaderSourceStream.str();
    }
    catch (std::ifstream::failure e)
    {
        std::cerr << "Couldn't load " << shaderPath << std::endl;
        return -1;
    }

    // From a std::string to a C string
    const char* shaderSourceString = shaderSource.c_str();

    // Create & compile a new shader
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSourceString, NULL);
    glCompileShader(shader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation error in " << shaderPath << " :\n" << infoLog << std::endl;
        return -1;
    }

    return shader;
}

GLuint createLinkProgram(GLuint shaders[], unsigned int numShaders)
{
    GLuint program = glCreateProgram();

    // Attach all of the shaders we've been given to the new program
    for (unsigned int i = 0; i < numShaders; i++)
    {
        glAttachShader(program, shaders[i]);
    }

    // Link all of our shaders together
    glLinkProgram(program);

    int success;
    char infoLog[512];

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "Shader link error:\n" << infoLog << std::endl;
    }

    return program;
}

void simulationTick()
{
    // Generate a texture using the compute shader
    glUseProgram(computeProgram);

    // Run the compute shader!
    glDispatchCompute((GLuint)gridWidth, (GLuint)gridHeight, 1);

    // Make sure that our compute shader has finished writing to the image
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Copy the output texture to be the input texture on the next frame
    glCopyImageSubData(outputTexture, GL_TEXTURE_2D, 0, 0, 0, 0, inputTexture, GL_TEXTURE_2D, 0, 0, 0, 0, gridWidth, gridHeight, 1);

    // Make sure the copy is complete
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Don't fire on key up events - it makes it nearly impossible to pause otherwise
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    switch (key) {
        case GLFW_KEY_W:
            currentYOffset += 0.01f;
            break;

        case GLFW_KEY_S:
            currentYOffset -= 0.01f;
            break;

        case GLFW_KEY_A:
            currentXOffset -= 0.01f;
            break;

        case GLFW_KEY_D:
            currentXOffset += 0.01f;
            break;

        case GLFW_KEY_SPACE:
            simulationIsRunning = !simulationIsRunning;
            break;

        case GLFW_KEY_EQUAL:
            currentScale += 0.1f;
            break;

        case GLFW_KEY_MINUS:
            currentScale -= 0.1f;
            break;

        case GLFW_KEY_T:
            simulationTick();
            break;
    }

    if (currentScale <= 0.0) {
        currentScale = 0.1f;
    }
}

int main()
{
    // Initalize GLFW and GLAD
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initalize GLFW!" << std::endl;
        return -1;
    }

    // Specify a minimum version of OpenGL our application can use
    // Because we use glCopyImageSubData, we'll need at least OpenGL 4.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Window creation
    GLFWwindow* window = glfwCreateWindow(width, height, "GameOfLife", NULL, NULL);

    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1); // enable vsync

    // Compile all of our shaders
    GLuint quadVertexShader = loadCompileShader("quad.vert", GL_VERTEX_SHADER);
    GLuint quadFragmentShader = loadCompileShader("quad.frag", GL_FRAGMENT_SHADER);
    GLuint computeShader = loadCompileShader("gameoflife.comp", GL_COMPUTE_SHADER);

    // Link the render & compute pipelines together into programs
    GLuint renderShaders[] = { quadVertexShader, quadFragmentShader };
    GLuint renderProgram = createLinkProgram(renderShaders, 2);
    computeProgram = createLinkProgram(&computeShader, 1);

    // Cleanup the shaders - now that we have our programs, we don't need
    // them anymore
    glDeleteShader(quadVertexShader);
    glDeleteShader(quadFragmentShader);
    glDeleteShader(computeShader);

    // Grab the references to uniforms for our render program
    GLint scaleUniformLocation = glGetUniformLocation(renderProgram, "scale");
    GLint offsetUniformLocation = glGetUniformLocation(renderProgram, "offset");

    // Initalize the VBO & VAO, and bind our quad data
    GLuint quadVBO, quadVAO;
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Set up attributes
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Load in our inital setup
    int imgWidth, imgHeight, numChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(INITAL_SETUP_PATH, &imgWidth, &imgHeight, &numChannels, 0);

    if (data == NULL) {
        std::cerr << "Could not read the inital setup!" << std::endl;
        return 1;
    }

    if (imgWidth != gridWidth || imgHeight != gridHeight || numChannels != 3) {
        std::cerr << "Inital setup image has the wrong dimensions!" << std::endl;
        fprintf(stderr, "Expected %d x %d x 3, got %d x %d x %d", gridWidth, gridHeight, imgWidth, imgHeight, numChannels);
        return 1;
    }

    // Create a texture for our compute shader input from / output to to
    glGenTextures(1, &inputTexture);
    glGenTextures(1, &outputTexture);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Create & bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    
    // Set a border colour, for lookups that extend out of the usual 0-1 UV space
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Interpolation mode. We're essentially rendering pixel art, so we use GL_NEAREST
    // (essentially, no fancy interpolation whatsoever)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Copy our inital state image into this texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gridWidth, gridHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    
    // And finally, bind this texture into image texture slot zero
    // so we can access it from the compute shader
    // A note: image texture slots are distinct from texture slots
    glBindImageTexture(0, inputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    // We don't need our original image anymore; free it
    stbi_image_free(data);

    // Do it all again for the output texture!
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Leave this texture blank to begin with
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gridWidth, gridHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(1, outputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    // Called whenever a key is pressed
    glfwSetKeyCallback(window, (GLFWkeyfun)key_callback);

    while (!glfwWindowShouldClose(window)) {
        if (simulationIsRunning)
            simulationTick();

        // Render our output texture to the screen
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        glViewport(0, 0, windowWidth, windowHeight);

        // Clear the colour buffer
        // We're rendering a quad that covers the entire screen, so we could
        // *technically* get away with not doing this, but oh well, best practices.
        glClearColor(0.15f, 0.15f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // We're using our rendering program, and the VAO corresponding to our (lone) quad
        glUseProgram(renderProgram);
        glBindVertexArray(quadVAO);

        // The fragment shader expects the outputTexture to be in texture slot zero
        // so bind it there
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTexture);
        
        // Assign all our uniform values, so the shader knows the current visible viewport
        glUniform1f(scaleUniformLocation, currentScale);
        glUniform2f(offsetUniformLocation, currentXOffset, currentYOffset);
        
        // Actually draw our quad!
        // TODO: use an EBO to save on some vertices
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up everything
    glfwDestroyWindow(window);
    glfwTerminate();
}