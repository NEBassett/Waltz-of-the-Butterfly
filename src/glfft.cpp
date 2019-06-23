#include "glfft.hpp"

glfft::~glfft()
{
  glDeleteTextures(1, &input_);
  glDeleteTextures(1, &locks_);
}

glfft::glfft() :
  fft_(
    GLDSEL::make_program_from_paths(
      boost::hana::make_tuple(boost::none,boost::none,boost::none,boost::none,boost::none, "../src/fft/main.cs"),
      glDselUniform("len", int)
    )
  ),
  input_(0),
  locks_(0)
{
  glGenTextures(1, &input_);
  glBindTexture(GL_TEXTURE_1D, input_);
  //glTexImage1D(GL_TEXTURE_1D, 0, GL_RG32F, 1, 0, GL_RG, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glGenTextures(1, &locks_);
  glBindTexture(GL_TEXTURE_1D, locks_);
  //glTexImage1D(GL_TEXTURE_1D, 0, GL_R32I, 1, 0, GL_RED_INTEGER, GL_INT, nullptr);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void glfft::operator()(int dim, GLuint input)
{
  std::vector<int> initial(dim, -1);

  glBindTexture(GL_TEXTURE_1D, locks_);
  glTexImage1D(GL_TEXTURE_1D, 0, GL_R32I, initial.size(), 0, GL_RED_INTEGER, GL_INT, initial.data()); // initialize to -1

  glBindImageTexture(0, input, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RG32F);
  glBindImageTexture(1, locks_, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32I);

  fft_.setUniforms(
    glDselArgument("len", int(data.size()))
  );

  glDispatchCompute(data.size(), 1, 1);

  glBindTexture(GL_TEXTURE_1D, input_); // bind output
}
