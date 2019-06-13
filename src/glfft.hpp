#ifndef GL_FFT_HPP__
#define GL_FFT_HPP__

class glfft
{
  using fftType = decltype(
    GLDSEL::make_program_from_paths(
        boost::hana::make_tuple("", ""),
        glDselUniform("len", int)
    )
  );

  fftType fft_;
  GLuint input_;
  GLuint locks_;
public:
  ~glfft();
  glfft();
  glfft(const glfft&) = delete;
  glfft(glfft) = delete;
  void operator()(int, GLuint);
};

#endif
