#include <numeric>
#include "GLAnnWin.h"
#include <cine/histogram.hpp>
#include <glsl/shader.h>
#include <glm/gtc/matrix_transform.hpp>


namespace cinema {


  struct AnnRenderer
  {
    std::unique_ptr<glsl::Camera> camera;
    glsl::CameraManip cameraManip;
    double zoom;
    double scale;
    double yorg;
    glm::dmat4 M;    // model matrix;
    GLSimState::ann_meta ann;
    GLuint tex;
    GLuint tex_hist;
    GLuint vbo_hist;
    float* ptr_hist;


    AnnRenderer(const GLSimState::ann_meta& indi) 
    : cameraManip(1000.0), zoom(1.0), scale(1.0), yorg(0), M(1), ann(indi), 
      tex(GL_NONE), tex_hist(GL_NONE), vbo_hist(GL_NONE), ptr_hist(nullptr)
    {
    }

    void on_size(glm::ivec4 vp)
    {
      auto world = glm::dvec4(0, ann.weights, 0, ann.N);
      camera->setOrthoViewport(vp, world);
      cameraManip.update(*camera, 1.0);
    }

    void on_zoom(int zDelta)
    {
      if (KeyState() == 0) {
        zoom = std::max(1.0, zoom + 0.25 * zDelta);
        M = glm::scale(glm::dmat4(1), glm::dvec3(1., zoom, 1.));
      }
      else if (KeyState() & 1) {
        double s = (zDelta < 0) ? 0.9 : 1.1;
        scale = std::max(0.01, s * scale); 
      }
    }

    // [-1,-1,1,1]
    glm::dvec2 ann_coor(int x, int y, const GLWin* glWin)
    {
      auto vp_coor = glWin->viewport_coor(x, y, camera->viewport());
      auto ls_coor = vp_coor + glm::dvec2(cameraManip.eye());
      return ls_coor / zoom;
    }

    void on_mouse_track(int dx, int dy, const GLWin* glWin)
    {
      auto shift = ann_coor(0, 0, glWin) - ann_coor(dx, dy, glWin);
      cameraManip.move(0., -shift.y, 0.);
      on_size(glm::ivec4(camera->viewport()));
    }

    void init_glsl()
    {
      camera.reset(new glsl::Camera());
      cameraManip.lookAt(glm::dvec3(0,0,1), glm::dvec3(0), glm::dvec3(0,1,0));
        
      glCreateTextures(GL_TEXTURE_2D, 1, &tex);
      glTextureStorage2D(tex, 1, GL_R32F, ann.weights, ann.N);
      glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

      glCreateTextures(GL_TEXTURE_2D, 1, &tex_hist);
      glTextureStorage2D(tex_hist, 1, GL_R32F, ann.weights, 1024);
      glTextureParameteri(tex_hist, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(tex_hist, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTextureParameteri(tex_hist, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTextureParameteri(tex_hist, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

      glCreateBuffers(1, &vbo_hist);
      const auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
      GLsizei size = static_cast<GLsizei>(sizeof(float) * ann.weights * 1024);
      glNamedBufferStorage(vbo_hist, size, nullptr, flags);
      ptr_hist = (float*)glMapNamedBufferRange(vbo_hist, 0, size, flags);
    }

    void destroy_glsl()
    {
      glDeleteTextures(1, &tex); tex = GL_NONE;
      glDeleteTextures(1, &tex_hist); tex_hist = GL_NONE;
      glDeleteBuffers(1, &vbo_hist); vbo_hist = GL_NONE;
    }

    void render_hist(GLuint prog, GLuint rProg, const float* pw)
    {
      if (vbo_hist == GL_NONE) return;
      histogram2D hist2D(0.f, static_cast<float>(ann.weights), ann.weights, -1.f, 1.f, 1024);
      for (int i = 0; i < ann.N; ++i, pw += ann.stride) {
        for (int w = 0; w < ann.weights; ++w) {
          hist2D(float(w), std::tanh(pw[w]));
        }
      }
      float max_count = 0;
      for (int w = 0; w < ann.weights; ++w) {
        float* pc = ptr_hist;
        for (int i = 0; i < 1024; ++i, pc += ann.weights) {
          auto val = hist2D[w][i].y;
          pc[w] = val;
          max_count = std::max(max_count, pc[w]);
        }
      }

      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, vbo_hist);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, ann.weights);
      glTextureSubImage2D(tex_hist, 0, 0, 0, ann.weights, 1024, GL_RED, GL_FLOAT, nullptr);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
      
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, tex_hist);
      auto vp = glm::ivec4(camera->viewport());
      glViewport(vp[0], vp[1], vp[2], vp[3]);
      auto MV = glm::mat4(M * camera->V());
      glUseProgram(rProg);
      glUniformMatrix4fv(0, 1, GL_FALSE, &MV[0][0]);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glUseProgram(prog);
      glUniformMatrix4fv(0, 1, GL_FALSE, &MV[0][0]);
      glUniform1f(2, static_cast<float>(scale / std::log(max_count + 1)));
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }


    void render(GLuint prog, GLuint rProg, GLuint vbo, GLint pack, const float* ptr)
    {
      if (vbo == GL_NONE) return;
      auto mm = std::minmax_element(ptr, ptr + ann.stride * ann.N);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, vbo);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, pack);
      glTextureSubImage2D(tex, 0, 0, 0, ann.weights, ann.N, GL_RED, GL_FLOAT, nullptr);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
      
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, tex);
      auto vp = glm::ivec4(camera->viewport());
      glViewport(vp[0], vp[1], vp[2], vp[3]);
      auto MV = glm::mat4(M * camera->V());
      glUseProgram(rProg);
      glUniformMatrix4fv(0, 1, GL_FALSE, &MV[0][0]);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glUseProgram(prog);
      glUniformMatrix4fv(0, 1, GL_FALSE, &MV[0][0]);
      glUniform1f(2, static_cast<float>(scale));
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

  };


  GLAnnWin::GLAnnWin(GLSimState* sim_state) 
  : GLWin(sim_state), 
    rectProg_(GL_NONE),
    annTanhProg_(GL_NONE),
    annLogP1Prog_(GL_NONE),
    colorMap_(3),
    histogram_(true) 
  {
    display_[0].reset(new AnnRenderer(sim_state->prey_ann()));
    display_[1].reset(new AnnRenderer(sim_state->pred_ann()));
  }

  
  GLAnnWin::~GLAnnWin() 
  {
  }


  void GLAnnWin::flush_async(long long msg)
  {
    using Msg = cine2::Simulation::msg_type;

    switch (msg) {
      case Msg::INITIALIZED:
      case Msg::NEW_GENERATION:
        ::InvalidateRect(m_hWnd, NULL, TRUE);
        break;
    }
  }


  void GLAnnWin::on_create()
  {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    rectProg_ = shader::ProgFromLiterals(shader::rectVert, shader::rectFrag);
    annTanhProg_ = shader::ProgFromLiterals(shader::annVert, shader::annTanhFrag);
    annLogP1Prog_ = shader::ProgFromLiterals(shader::annVert, shader::annLogP1Frag);
    display_[0]->init_glsl();
    display_[1]->init_glsl();
  }


  void GLAnnWin::on_close()
  {
    glUseProgram(0);
    glDeleteProgram(rectProg_);
    glDeleteProgram(annTanhProg_);
    glDeleteProgram(annLogP1Prog_);
    display_[0]->destroy_glsl();
    display_[1]->destroy_glsl();
  }


  LRESULT GLAnnWin::OnContextMenu(UINT, WPARAM, LPARAM lParam, BOOL&)
  {
    int screenX = GET_X_LPARAM(lParam);
    int screenY = GET_Y_LPARAM(lParam);
    std::array<bool, 7> selected{false};
    const char* title[7] = {
      "gradient",
      "bone",
      "cool",
      "hot",
      "hsv",
      "spectrum",
      "histograms"
    };

    selected[colorMap_] = true; 
    selected[6] = histogram_;
    ContextMenu menu(title, 7);
    int selection = menu.track(*this, screenX, screenY, selected.data());
    if (selection >= 0) {
      if (selection < 6) colorMap_ = selection;
      if (selection == 6) histogram_ = !histogram_;
      display_[0]->scale = 1.0;
      display_[1]->scale = 1.0;
      display_[0]->zoom = 1.0;
      display_[1]->zoom = 1.0;
    }
    ::InvalidateRect(m_hWnd, NULL, TRUE);
    return 1;
  }


  void GLAnnWin::on_size()
  {
    display_[0]->on_size(glm::ivec4(10, 10, (winW_ / 2) - 15, winH_ - 20));
    display_[1]->on_size(glm::ivec4((winW_ / 2) + 5, 10, (winW_ / 2) - 15, winH_ - 20));
    ::InvalidateRect(m_hWnd, NULL, TRUE);
  }


  // [-1,-1,1,1]
  glm::dvec2 GLAnnWin::landscape_coor(int x, int y)
  {
    return glm::dvec2();
  }


  void GLAnnWin::on_mouse_track(int dx, int dy)
  {
    if (hit_test(trackStartX_, trackStartY_, display_[0]->camera->viewport())) {
      display_[0]->on_mouse_track(dx, dy, this);
      ::InvalidateRect(m_hWnd, NULL, TRUE);
    }
    else if (hit_test(trackStartX_, trackStartY_, display_[1]->camera->viewport())) {
      display_[1]->on_mouse_track(dx, dy, this);
      ::InvalidateRect(m_hWnd, NULL, TRUE);
    }
  }


  void GLAnnWin::on_mouse_wheel(int zDelta)
  {
    if (hit_test(mouseX_, mouseY_, display_[0]->camera->viewport())) {
      display_[0]->on_zoom(zDelta);
      ::InvalidateRect(m_hWnd, NULL, TRUE);
    }
    else if (hit_test(mouseX_, mouseY_, display_[1]->camera->viewport())) {
      display_[1]->on_zoom(zDelta);
      ::InvalidateRect(m_hWnd, NULL, TRUE);
    }
  }


  void GLAnnWin::render()
  {
    glBindBuffer(GL_ARRAY_BUFFER, sim_state_->vbo(GLSimState::VBO_DUMMY));
    glBindVertexArray(sim_state_->vao(GLSimState::VAO_DUMMY));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, sim_state_->colortex(colorMap_));
    if (histogram_) {
      display_[0]->render_hist(annLogP1Prog_, rectProg_, (const float*)sim_state_->ptr(GLSimState::VBO_PREY_ANN));
      display_[1]->render_hist(annLogP1Prog_, rectProg_, (const float*)sim_state_->ptr(GLSimState::VBO_PRED_ANN));
      return;
    }
    GLuint vbo0, vbo1;
    GLint preyPack, predPack;
    {
      //std::lock_guard<std::recursive_mutex> _(mutex_);
      vbo0 = sim_state_->vbo(GLSimState::VBO_PREY_ANN);
      vbo1 = sim_state_->vbo(GLSimState::VBO_PRED_ANN);
      preyPack = sim_state_->prey_ann().type_size / sizeof(float);
      predPack = sim_state_->pred_ann().type_size / sizeof(float);
    }
    display_[0]->render(annTanhProg_, rectProg_, vbo0, preyPack, (const float*)sim_state_->ptr(GLSimState::VBO_PREY_ANN));
    display_[1]->render(annTanhProg_, rectProg_, vbo1, predPack, (const float*)sim_state_->ptr(GLSimState::VBO_PRED_ANN));
  }


}
