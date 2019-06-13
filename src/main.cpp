#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <tuple>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <array>
#include <vector>
#include <complex>
#include <math.h>
#include <utility>
#include <algorithm>
#include <boost/hana/tuple.hpp>
#include <boost/hana/zip.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <algorithm>
#include "program.hpp"
#include "glfft.hpp"

void GLAPIENTRY msgCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam )
{
  std::cout << message << '\n';
}

struct screenQuad
{
  GLuint vao, vbo;

  screenQuad()
  {
    static auto verts = std::array<glm::vec4, 6>{
            glm::vec4(1.0f,  1.0f, 1.0f, 1.0f),
            glm::vec4(1.0f, -1.0f, 1.0f, 0.0f),
            glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f),
            glm::vec4(-1.0f, -1.0f, 0.0f, 0.0f),
            glm::vec4(1.0f, -1.0f, 1.0f, 0.0f),
            glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f)
    };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4)*6, verts.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(0);
  }

  screenQuad(const screenQuad &) = delete;

  ~screenQuad()
  {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
  }
};

int main()
{
  try
  {
    glfwSetErrorCallback([](auto err, const auto* desc){ std::cout << "Error: " << desc << '\n'; });

    // glfw init
    if(!glfwInit())
    {
      throw std::runtime_error(
        "GLFW failed to initialize"
      );
    }

    // context init
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    auto window = glfwCreateWindow(640, 480, "GLFFT", NULL, NULL);
    if (!window)
    {
      throw std::runtime_error(
        "GLFW window creation failed"
      );
    }

    glfwMakeContextCurrent(window);

    // glew init
    auto err = glewInit();
    if(GLEW_OK != err)
    {
      //std::cout << "glew failed to init: " << glewGetErrorString(err) << '\n';
      throw std::runtime_error(
        std::string("GLEW initialization failed with error: ") + std::string((const char*)(glewGetErrorString(err)))
      );
    }

    // gl init
    glEnable(GL_DEBUG_OUTPUT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glPointSize(15.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDebugMessageCallback(msgCallback, 0);
    glEnable(GL_DEPTH_TEST);



    // program initialization


    int width, height;
    double time = glfwGetTime();
    glm::mat4 proj;
    double tdelta;
    double temp;

    //glfwSetWindowUserPointer(window, &fsim);

     glfwSetKeyCallback(window, [](auto *window, auto key, auto, auto action, auto mods){
       //auto fsimptr = static_cast<>(glfwGetWindowUserPointer(window));

       if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
         glfwSetWindowShouldClose(window, GLFW_TRUE);
       if(key == GLFW_KEY_F4)
       { /* later */ }
     });

     auto draw = GLDSEL::make_program_from_paths(
      boost::hana::make_tuple("../src/main/vertex.vs", "../src/main/fragment.fs"),
      glDselUniform("time", float),
      glDselUniform("model", glm::mat4),
      glDselUniform("view", glm::mat4),
      glDselUniform("proj", glm::mat4)
    );

     screenQuad quad{};
     glfft fftcontext;

     std::vector<glm::vec2> inputs{
       glm::vec2(0,0),
       glm::vec2(0,0),
       glm::vec2(1,0),
       glm::vec2(0,0),
       glm::vec2(0,0),
       glm::vec2(0,0),
       glm::vec2(0,0),
       glm::vec2(0,0)
     };

     GLuint tex;
     glGenTextures(1, &tex);
     glBindTexture(GL_TEXTURE_1D, tex);
     glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, initial.size(), 0, GL_RED, GL_FLOAT, inputs.data()); // initialize to -1

     fftcontext(tex);

     glBindTexture(GL_TEXTURE_1D, tex);

     glfwSwapInterval(1);
     while(!glfwWindowShouldClose(window))
     {
      auto oldT = time;
      time = glfwGetTime();

      glfwGetFramebufferSize(window, &width, &height);
      glViewport(0, 0, width, height);
      proj = glm::perspective(1.57f, float(width)/float(height), 0.1f, 7000.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      //draw(proj, view, glm::mat4(), glfwGetTime(), plane);
      draw.setUniforms( // set uniforms
        glDselArgument("model", glm::mat4()),
        glDselArgument("view", glm::mat4()),
        glDselArgument("proj", proj),
        glDselArgument("time", float(glfwGetTime()))
      );

      glBindVertexArray(quad.vao);

      glDrawArrays(GL_TRIANGLES, 0, 6); // draw quad

      glfwSwapBuffers(window);
      glfwPollEvents();
    }
  }
  catch(std::exception &e)
  {
    std::cout << "main failed with " << e.what() << '\n';
  }

  glfwTerminate();
  return 0;
}
