#ifdef GFLAGS_NAMESPACE_GOOGLE
#define GLUTILS_GFLAGS_NAMESPACE google
#else
#define GLUTILS_GFLAGS_NAMESPACE gflags
#endif

// Include first C-Headers.
#define _USE_MATH_DEFINES  // For using M_PI.
#include <cmath>
// Include second C++-Headers.
#include <iostream>
#include <string>
#include <vector>

// Include library headers.
// Include CImg library to load textures.
// The macro below disables the capabilities of displaying images in CImg.
#define cimg_display 0
#include <CImg.h>

// The macro below tells the linker to use the GLEW library in a static way.
// This is mainly for compatibility with Windows.
// Glew is a library that "scans" and knows what "extensions" (i.e.,
// non-standard algorithms) are available in the OpenGL implementation in the
// system. This library is crucial in determining if some features that our
// OpenGL implementation uses are not available.
#define GLEW_STATIC
#include <GL/glew.h>
// The header of GLFW. This library is a C-based and light-weight library for
// creating windows for OpenGL rendering.
// See http://www.glfw.org/ for more information.
#include <GLFW/glfw3.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <gflags/gflags.h>
#include <glog/logging.h>

// Include system headers.
#include "shader_program.h"
#include "camera_utils.h"
#include "model.h"
#include "transformations.h"


// Google flags.
// (<name of the flag>, <default value>, <Brief description of flat>)
// These will define global variables w/ the following format
// FLAGS_vertex_shader_filepath and 
// FLAGS_fragment_shader_filepath.
// DEFINE_<type>(name of flag, default value, brief description.)
// types: string, int32, bool.
DEFINE_string(vertex_shader_filepath, "vertex_shader.glsl",
              "Filepath of the vertex shader.");
DEFINE_string(fragment_shader_filepath, "fragment_shader.glsl",
              "Filepath of the fragment shader.");
DEFINE_string(texture1_filepath, "texture1.jpg",
              "Filepath of the texture.");
DEFINE_string(texture2_filepath, "texture2.jpg",
              "Filepath of the texture.");
DEFINE_string(texture3_filepath, "texture3.jpg",
              "Filepath of the texture.");
DEFINE_string(texture4_filepath, "texture4.jpg",
              "Filepath of the texture.");
// Annonymous namespace for constants and helper functions.
namespace {
using wvu::Model;

// Window dimensions.
constexpr int kWindowWidth = 640;
constexpr int kWindowHeight = 480;

// GLSL shaders.
// Every shader should declare its version.
// Vertex shader follows standard 3.3.0.
// This shader declares/expexts an input variable named position. This input
// should have been loaded into GPU memory for its processing. The shader
// essentially sets the gl_Position -- an already defined variable -- that
// determines the final position for a vertex.
// Note that the position variable is of type vec3, which is a 3D dimensional
// vector. The layout keyword determines the way the VAO buffer is arranged in
// memory. This way the shader can read the vertices correctly.
    const std::string vertex_shader_src =
            "#version 330 core\n"
                    "layout (location = 0) in vec3 position;\n"
                    "layout (location = 0) in vec3 passed_color;\n"
                    "layout (location = 0) in vec2 passed_texel;\n"
                    "uniform mat4 model;\n"
                    "uniform mat4 view;\n"
                    "uniform mat4 projection;\n"
                    "\n"
                    "out vec4 vertex_color;\n"
                    "out vec2 texel;\n"
                    "void main() {\n"
                    "gl_Position = projection * view * model * vec4(position, 1.0f);\n"
                    "vertex_color = vec4(passed_color, 1.0f);\n"
                    "texel = passed_texel;\n"
                    "}\n";

// Fragment shader follows standard 3.3.0. The goal of the fragment shader is to
// calculate the color of the pixel corresponding to a vertex. This is why we
// declare a variable named color of type vec4 (4D vector) as its output. This
// shader sets the output color to a (1.0, 0.5, 0.2, 1.0) using an RGBA format.
const std::string fragment_shader_src =
      "#version 330 core\n"
     "in vec4 vertex_color;\n"
    "out vec4 color;\n"
    "in vec2 texel;\n"

    "uniform sampler2D texture_sampler;\n"
                    "void main() {\n"
                    "color = texture(texture_sampler, texel);\n"
                    "}\n";

// Error callback function. This function follows the required signature of
// GLFW. See http://www.glfw.org/docs/3.0/group__error.html for more
// information.
static void ErrorCallback(int error, const char* description) {
  std::cerr << "ERROR: " << description << std::endl;
}

// Key callback. This function follows the required signature of GLFW. See
// http://www.glfw.org/docs/latest/input_guide.html fore more information.
static void KeyCallback(GLFWwindow* window,
                        int key,
                        int scancode,
                        int action,
                        int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
}

// Configures glfw.
void SetWindowHints() {
  // Sets properties of windows and have to be set before creation.
  // GLFW_CONTEXT_VERSION_{MAJOR|MINOR} sets the minimum OpenGL API version
  // that this program will use.
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  // Sets the OpenGL profile.
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  // Sets the property of resizability of a window.
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
}



// Configures the view port.
// Note: All the OpenGL functions begin with gl, and all the GLFW functions
// begin with glfw. This is because they are C-functions -- C does not have
// namespaces.
void ConfigureViewPort(GLFWwindow* window) {
  int width;
  int height;
  // We get the frame buffer dimensions and store them in width and height.
  glfwGetFramebufferSize(window, &width, &height);
  // Tells OpenGL the dimensions of the window and we specify the coordinates
  // of the lower left corner.
  glViewport(0, 0, width, height);
}

// Clears the frame buffer.
    void ClearTheFrameBuffer() {
        // Sets the initial color of the framebuffer in the RGBA, R = Red, G = Green,
        // B = Blue, and A = alpha.
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // Tells OpenGL to clear the Color buffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    bool CreateShaderProgram(wvu::ShaderProgram* shader_program) {
        if (shader_program == nullptr) return false;
        shader_program->LoadVertexShaderFromString(vertex_shader_src);
        shader_program->LoadFragmentShaderFromString(fragment_shader_src);
        std::string error_info_log;
        if (!shader_program->Create(&error_info_log)) {
            std::cout << "ERROR: " << error_info_log << "\n";
        }
        if (!shader_program->shader_program_id()) {
            std::cerr << "ERROR: Could not create a shader program.\n";
            return false;
        }
        return true;
    }
    GLuint LoadTexture(const std::string& texture_filepath) {
        cimg_library::CImg<unsigned char> image;
        image.load(texture_filepath.c_str());
        const int width = image.width();
        const int height = image.height();
        // OpenGL expects to have the pixel values interleaved (e.g., RGBD, ...). CImg
        // flatens out the planes. To have them interleaved, CImg has to re-arrange
        // the values.
        // Also, OpenGL has the y-axis of the texture flipped.
        image.permute_axes("cxyz");
        GLuint texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        // We are configuring texture wrapper, each per dimension,s:x, t:y.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Define the interpolation behavior for this texture.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        /// Sending the texture information to the GPU.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                     0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
        // Generate a mipmap.
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture_id;
    }
// Renders the scene.
void RenderScene(const wvu::ShaderProgram& shader_program,
                     const Eigen::Matrix4f& projection,
                     const Eigen::Matrix4f& view,
                     std::vector<Model*>* models_to_draw,
                     const GLuint texture_id1,
                     const GLuint texture_id2,
                     const GLuint texture_id3,
                     const GLuint texture_id4,
                     GLFWwindow* window) {
  // Clear the buffer.
  ClearTheFrameBuffer();
  // Let OpenGL know that we want to use our shader program.
  shader_program.Use();
  // Render the models in a wireframe mode.
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  // Draw the models.
 
  // TODO: For every model in models_to_draw, call its Draw() method, passing
    

    Eigen::Vector3f rotate = (*models_to_draw)[2]->orientation();

    for(std::vector<Model*>::iterator it = models_to_draw->begin(); it != models_to_draw->end(); ++it) 
    {
    if((*it) == (*models_to_draw)[2]){
    (*it)->set_orientation(Eigen::Vector3f(rotate[0],rotate[1],rotate[2]+0.001));
    (*it)->Draw(shader_program,projection,view,texture_id2);
    }
     if((*it) == (*models_to_draw)[1]){
    (*it)->Draw(shader_program,projection,view,texture_id3);
    }
     if((*it) == (*models_to_draw)[3] 
      or (*it) == (*models_to_draw)[4] 
      or (*it) == (*models_to_draw)[5]
      or (*it) == (*models_to_draw)[6]
      or (*it) == (*models_to_draw)[7]
      or (*it) == (*models_to_draw)[8]){
    Eigen::Vector3f pos = (*it)->position();
    (*it)->set_position(Eigen::Vector3f(pos[0]-.0002,pos[1],pos[2]));
    (*it)->Draw(shader_program,projection,view,texture_id4);
    }
     if((*it) == (*models_to_draw)[0]){
    Eigen::Vector3f rot = (*it)->orientation();
    (*it)->set_orientation(Eigen::Vector3f(rot[0],rot[1]+.0002,rot[2]));
    (*it)->Draw(shader_program,projection,view,texture_id1);
  }

  }
  // the view and projection matrices.


  // Let OpenGL know that we are done with our vertex array object.
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);

}

void ConstructModels(std::vector<Model*>* models_to_draw) {
  // TODO: Prepare your models here.
  // 1. Construct models by setting their vertices and poses.
  // 2. Create your models in the heap and add the pointers to models_to_draw.
  // 3. For every added model in models to draw, set the GPU buffers by
  // calling the method model method SetVerticesIntoGPU().

  std::vector<GLuint> indices = {
    0, 1, 2,  // First triangle.
    0, 2, 3,  // bottom triangle.
    3, 4, 2,  // third triangle.
    0, 3, 4,  // forth triangle.
    1, 2, 4,  // bottom triangle.
    0, 1, 4   // sixth triangle.
  };
  Eigen::MatrixXf vertices(3, 5);
  vertices.col(0) = Eigen::Vector3f(0.5f, 1.0f, -0.5f);
  vertices.col(1) = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
  vertices.col(2) = Eigen::Vector3f(1.0f, 0.0f, 0.0f);
  vertices.col(3) = Eigen::Vector3f(1.0f, 0.0f, -1.0f);
  vertices.col(4) = Eigen::Vector3f(0.0f, 0.0f, -1.0f);

  Model* pyrimid = new Model(Eigen::Vector3f(-0.3, -0.3, 0.0),  // Orientation of object.
                      Eigen::Vector3f(-0.7, -0.5, -1.4),  // Position of object.
                      vertices, indices);
  pyrimid->SetVerticesIntoGpu();

  models_to_draw->push_back(pyrimid);



  Eigen::MatrixXf vertices_ground(3, 4);
  vertices_ground.col(0) = Eigen::Vector3f(-5.0f, 0.0f, 10.0f);
  vertices_ground.col(1) = Eigen::Vector3f(5.0f, 0.0f, 10.0f);
  vertices_ground.col(2) = Eigen::Vector3f(-5.0f, 0.0f, 0.0f);
  vertices_ground.col(3) = Eigen::Vector3f(5.0f, 0.0f, 0.0f);

  std::vector<GLuint> indices_ground = {
    3, 1, 0,  // bottom triangle.
    2, 0, 3,  // bottom triangle.
  };

  Model* ground = new Model(Eigen::Vector3f(-0.3, -0.3, 0.0),  // Orientation of object.
                      Eigen::Vector3f(0, -2.6, -8),  // Position of object.
                      vertices_ground, indices_ground);
     ground->SetVerticesIntoGpu();

  // models_to_draw->push_back(ground);
  Eigen::MatrixXf vertices_sky(3, 4);
  vertices_sky.col(0) = Eigen::Vector3f(0.0f, 10.0f, 0.0f);
  vertices_sky.col(1) = Eigen::Vector3f(10.0f, 0.0f, 0.0f);
  vertices_sky.col(2) = Eigen::Vector3f(10.0f, 10.0f, 0.0f);
  vertices_sky.col(3) = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
  
  models_to_draw->push_back(ground);


  std::vector<GLuint> indices_sky = {
    0, 2, 1,  // bottom triangle.
    0, 3, 1,  // bottom triangle.
  };

  Model* sky = new Model(Eigen::Vector3f(0.1, 0.1,0.1),  // Orientation of object.
                      Eigen::Vector3f(-0.7, -4.0, -8),  // Position of object.
                      vertices_sky, indices_sky);
  
  sky->SetVerticesIntoGpu();

  models_to_draw->push_back(sky);


   

  Eigen::MatrixXf vertices_rectangle(3, 10);
  vertices_rectangle.col(0) = Eigen::Vector3f(0.0f, 1.0f, 0.0f);
  vertices_rectangle.col(1) = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
  vertices_rectangle.col(2) = Eigen::Vector3f(0.10f, 1.0f, 0.0f);
  vertices_rectangle.col(3) = Eigen::Vector3f(0.10f, 0.0f, 0.0f);
  vertices_rectangle.col(4) = Eigen::Vector3f(0.10f, 1.0f, -0.10f);
  vertices_rectangle.col(5) = Eigen::Vector3f(0.10f, 0.0f, -0.10f);
  vertices_rectangle.col(6) = Eigen::Vector3f(0.0f, 1.0f, -0.10f);
  vertices_rectangle.col(7) = Eigen::Vector3f(0.0f, 0.0f, -0.10f);
  vertices_rectangle.col(8) = Eigen::Vector3f(0.0f, 1.0f, 0.0f);
  vertices_rectangle.col(9) = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
  
  std::vector<GLuint> indices_rectangle = {
    0, 1, 3,  // First triangle.
    0, 3, 2,  // Second triangle.
    2, 3, 5,  // Third triangle.
    2, 5, 4,  // Fourth triangle.
    4, 5, 7,  // Fifth triangle.
    4, 7, 6,  // Sixth triangle.
    0, 1, 7,  // Seventh triangle.
    0, 7, 6   // Eigth triangle.
};


float rightPos = 0.09;
  

  Model* cactus_1 = new Model(Eigen::Vector3f(-0.3, -0.3, 0.0),  // Orientation of object.
                      Eigen::Vector3f(-0.01 + rightPos, -0.5, -1.0),  // Position of object.
                      vertices_rectangle, indices_rectangle);
  Model* cactus_2 = new Model(Eigen::Vector3f(-0.3, -0.3, wvu::ConvertDegreesToRadians(90)),  // Orientation of object.
                      Eigen::Vector3f(0.32+ rightPos, 0.06, -1.1),  // Position of object.
                      vertices_rectangle/1.5, indices_rectangle);
  Model* cactus_3 = new Model(Eigen::Vector3f(-0.3, -0.3, 0.0),  // Orientation of object.
                      Eigen::Vector3f(0.32+ rightPos, 0.06, -1.1),  // Position of object.
                      vertices_rectangle/3, indices_rectangle);
  Model* cactus_4 = new Model(Eigen::Vector3f(-0.3, -0.3, 0.0),  // Orientation of object.
                      Eigen::Vector3f(0.15+0.08+ rightPos, 0.06, -1.1),  // Position of object.
                      vertices_rectangle/3, indices_rectangle);
  Model* cactus_5 = new Model(Eigen::Vector3f(-0.3, -0.3, 0.0),  // Orientation of object.
                      Eigen::Vector3f(0.32-.5+ rightPos, 0.06, -1.3),  // Position of object.
                      vertices_rectangle/3, indices_rectangle);
  Model* cactus_6 = new Model(Eigen::Vector3f(-0.3, -0.3, 0.0),  // Orientation of object.
                      Eigen::Vector3f(0.15-.42+ rightPos, 0.06, -1.3),  // Position of object.
                      vertices_rectangle/3, indices_rectangle);


  cactus_1->SetVerticesIntoGpu();

  models_to_draw->push_back(cactus_1);

  cactus_2->SetVerticesIntoGpu();

  models_to_draw->push_back(cactus_2);

  cactus_3->SetVerticesIntoGpu();

  models_to_draw->push_back(cactus_3);

  cactus_4->SetVerticesIntoGpu();

  models_to_draw->push_back(cactus_4);

  cactus_5->SetVerticesIntoGpu();

  models_to_draw->push_back(cactus_5);

  cactus_6->SetVerticesIntoGpu();

  models_to_draw->push_back(cactus_6);




  // for(std::vector<Model*>::iterator it = models_to_draw->begin(); it != models_to_draw->end(); ++it) 
  // {
  //   (*it)->SetVerticesIntoGpu();
  // }
}

void DeleteModels(std::vector<Model*>* models_to_draw) {
  // TODO: Implement me!
  models_to_draw->clear();
  // Call delete on each models to draw.
}

}  // namespace

int main(int argc, char** argv) {
  // Initialize the GLFW library.
  GLUTILS_GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  if (!glfwInit()) {
    return -1;
  }



  // Setting Window hints.
  SetWindowHints();

  // Create a window and its OpenGL context.
  const std::string window_name = "Assignment 4";
  GLFWwindow* window = glfwCreateWindow(kWindowWidth,
                                        kWindowHeight,
                                        window_name.c_str(),
                                        nullptr,
                                        nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  // Make the window's context current.
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glfwSetKeyCallback(window, KeyCallback);

  // Initialize GLEW.
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    std::cerr << "Glew did not initialize properly!" << std::endl;
    glfwTerminate();
    return -1;
  }

  // Configure View Port.
  ConfigureViewPort(window);

  // Compile shaders and create shader program.
    const std::string vertex_shader_filepath =
            FLAGS_vertex_shader_filepath;
    const std::string fragment_shader_filepath =
            FLAGS_fragment_shader_filepath;
    wvu::ShaderProgram shader_program;
    shader_program.LoadVertexShaderFromFile(vertex_shader_filepath);
    shader_program.LoadFragmentShaderFromFile(fragment_shader_filepath);
  if (!CreateShaderProgram(&shader_program)) {
    return -1;
  }

  // Construct the models to draw in the scene.
  std::vector<Model*> models_to_draw;
  ConstructModels(&models_to_draw);

  //textures
  const GLuint texture_id1 = LoadTexture(FLAGS_texture1_filepath);
  const GLuint texture_id2 = LoadTexture(FLAGS_texture2_filepath);
  const GLuint texture_id3 = LoadTexture(FLAGS_texture3_filepath);
  const GLuint texture_id4 = LoadTexture(FLAGS_texture4_filepath);



  // Construct the camera projection matrix.
  const float field_of_view = wvu::ConvertDegreesToRadians(45.0f);
  const float aspect_ratio = static_cast<float>(kWindowWidth / kWindowHeight);
  const float near_plane = 0.1f;
  const float far_plane = 10.0f;
  const Eigen::Matrix4f& projection =
  wvu::ComputePerspectiveProjectionMatrix(field_of_view, aspect_ratio,
                                              near_plane, far_plane);
  const Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

  // Loop until the user closes the window.
  while (!glfwWindowShouldClose(window)) {
    // Render the scene!
    RenderScene(shader_program, projection, view, &models_to_draw,texture_id1,texture_id2,texture_id3,texture_id4, window);

    // Swap front and back buffers.
    glfwSwapBuffers(window);

    // Poll for and process events.
    glfwPollEvents();
  }

  // Cleaning up tasks.
  DeleteModels(&models_to_draw);
  // Destroy window.
  glfwDestroyWindow(window);
  // Tear down GLFW library.
  glfwTerminate();

  return 0;
}
