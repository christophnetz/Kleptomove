#include <type_traits>
#include "bridge.h"
#include <glsl/shader.h>


namespace cinema {

  namespace bridge {

/*  
    Bridge::Bridge(const cine2::Simulation* sim)
    :  sim_(sim)
    {
      application_terminated = false;
      application_finished = false;
      application_paused = true;
      application_breakpoint = false;
      vbo_[0].assign(GL_NONE);
      vbo_[1].assign(GL_NONE);
      ptr_[0].assign(nullptr);
      ptr_[1].assign(nullptr);
      current_ = 0;
    }


    void Bridge::init_glsl()
    {
      using prey_ann = cine2::Param::prey_t::type::ann_type;
      using pred_ann = cine2::Param::pred_t::type::ann_type;
      static_assert(std::is_same<prey_ann::value_type, float>::value, "ANN value_type shall be float");
      static_assert(std::is_same<pred_ann::value_type, float>::value, "ANN value_type shall be float");

      if (ptr_[0][0]) return;
      const auto preyN = sim_->param().prey.N;
      const auto predN = sim_->param().pred.N;
      const auto N = preyN + predN;
      const auto dim = sim_->landscape().dim();
      const auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
      for (int s = 0; s < 2; ++s) {
        glGenBuffers(VBO_MAX, vbo_[s].data());
        glNamedBufferStorage(vbo_[s][VBO_PROXY], GLsizei(N * sizeof(shader::IndividualProxy)), nullptr, flags); 
        ptr_[s][VBO_PROXY] = glMapNamedBufferRange(vbo_[s][VBO_PROXY], 0, GLsizei(N * sizeof(shader::IndividualProxy)), flags);
        glNamedBufferStorage(vbo_[s][VBO_PREY_ANN], GLsizei(preyN * sizeof(float) * prey_ann::state_size), nullptr, flags); 
        ptr_[s][VBO_PREY_ANN] = glMapNamedBufferRange(vbo_[s][VBO_PREY_ANN], 0, GLsizei(preyN * sizeof(float) * sizeof(prey_ann)), flags);
        glNamedBufferStorage(vbo_[s][VBO_PRED_ANN], GLsizei(predN * sizeof(float) * pred_ann::state_size), nullptr, flags); 
        ptr_[s][VBO_PRED_ANN] = glMapNamedBufferRange(vbo_[s][VBO_PRED_ANN], 0, GLsizei(predN * sizeof(float) * sizeof(pred_ann)), flags);
        glNamedBufferStorage(vbo_[s][VBO_GRASS_RISK], 2 * GLsizei(dim * dim * sizeof(float)), nullptr, flags); 
        ptr_[s][VBO_GRASS_RISK] = glMapNamedBufferRange(vbo_[s][VBO_GRASS_RISK], 0, 2 * GLsizei(dim * dim * sizeof(float)), flags);
      }
      current_ = 0;
    }

    void Bridge::release_glsl()
    {
      glDeleteBuffers(VBO_MAX, vbo_[0].data());
      glDeleteBuffers(VBO_MAX, vbo_[1].data());
      vbo_[0].assign(GL_NONE);
      vbo_[1].assign(GL_NONE);
      ptr_[0].assign(nullptr);
      ptr_[1].assign(nullptr);
    }
  */

  }
}

