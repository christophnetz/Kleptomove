#ifndef CINEMA_BRIDGE_H_INCLUDED
#define CINEMA_BRIDGE_H_INCLUDED

#include <atomic>
#include <memory>
#include <array>
#include <cine/simulation.h>
#include <glsl/context.h>


namespace cinema {


  namespace bridge {


    //class Bridge 
    //{
    //public:
    //  explicit Bridge(const cine2::Simulation*);
    //  void init_glsl();
    //  void release_glsl();

    //public:

    //  const cine2::Simulation* sim() const { return sim_; }

    //private:
    //  enum VBO {
    //    VBO_PROXY,
    //    VBO_PREY_ANN,
    //    VBO_PRED_ANN,
    //    VBO_GRASS_RISK,
    //    VBO_MAX
    //  };
    //  const cine2::Simulation* sim_;
    //  std::array<GLuint, VBO_MAX> vbo_[2];
    //  std::array<void*, VBO_MAX> ptr_[2];
    //  int current_;
    //};

  }
}

#endif

