#ifndef CINEMA_GLSIMSTATE_H_INCLUDED
#define CINEMA_GLSIMSTATE_H_INCLUDED

#include <memory>
#include <array>
#include <string>
#include <unordered_map>
#include <glsl/wgl_context.hpp>
#include <glsl/bmfont.hpp>


namespace cine2 {

  class Simulation;

}


namespace cinema {


  class GLSimState
  {
  public:
    enum VBO
    {
      VBO_PREY_ANN,
      VBO_PRED_ANN,
      VBO_LAYER,
      VBO_DUMMY,
      VBO_MAX,
    };

    enum VAO
    {
      VAO_DUMMY,
      VAO_MAX
    };

    enum COLORMAP
    {
    };

    struct ann_meta
    {
      const int N;
      const int weights;
      const int stride;
      const int type_size;
    };


  public:
    GLSimState(HWND hWnd, const class cine2::Simulation* sim);
    ~GLSimState();

    int dim() const { return dim_; }
    const ann_meta& prey_ann() const { return prey_ann_; }
    const ann_meta& pred_ann() const { return pred_ann_; }

    void* ptr(VBO vbo) const { return ptr_[vbo]; }
    GLuint vbo(VBO vbo) const { return vbo_[vbo]; }
    GLuint vao(VAO vbo) const { return vao_[vbo]; }
    GLuint colortex(int i) const { return ColorMap_[i]; }

    void flush_async(const class cine2::Simulation&, long long msg);
    const glsl::Context& ctx() const { return glctx_; }

    // save to access in flush_async
    const cine2::Simulation* sim() const { return sim_; }

  private:
    const int dim_;
    const ann_meta prey_ann_;
    const ann_meta pred_ann_;
    std::array<void*, VBO_MAX> ptr_;
    std::array<GLuint, VBO_MAX> vbo_;
    std::array<GLuint, VAO_MAX> vao_;

    // Font rendering
    std::unordered_map<std::string, std::shared_ptr<glsl::bmfont::Font> > Faces_;
    std::unique_ptr<glsl::bmfont::Text2D>  text2D_;

    std::array<GLuint, 1 + 5> ColorMap_;
    std::string exePath_;
    GLuint textProg_;
    glsl::Context glctx_;
    const cine2::Simulation* sim_;
  };


}

#endif

