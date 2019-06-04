#include <set>
#include <numeric>
#include <algorithm>
#include "GLTimeLineWin.h"
#include <cine/analysis.h>
#include <glsl/shader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>


namespace cinema {


  const std::array<const char*, 5+1> GLTimeLineWin::title_ = {
    "Average fitness (pop)",
    "Average fitness (repro)",
    "Reproductive individuals",
    "Reproductive clades",
    "Complexity",
    "reset"
  };


  const std::array<glm::vec4, GLTimeLineWin::NT> GLTimeLineWin::color_ = {
    glm::vec4(0,0,1,1),
    glm::vec4(0,0,1,1),
    glm::vec4(0,0,1,1),
    glm::vec4(0,0,1,1),
    glm::vec4(0,0,1,1),
    glm::vec4(1,0,0,1),
    glm::vec4(1,0,0,1),
    glm::vec4(1,0,0,1),
    glm::vec4(1,0,0,1),
    glm::vec4(1,0,0,1),
  };


  GLTimeLineWin::GLTimeLineWin(GLSimState* sim_state) 
  : GLWin(sim_state), 
    G_(0), g_(0),
    max_{0,0,0,0,1,0,0,0,0,1},
    selected_(0),
    buf_(GL_NONE),
    pbuf_(nullptr),
    vao_{GL_NONE},
    cameraManip_(1000.0),
    zoom_{1.0, 1.0},
    rectProg_(GL_NONE),
    lineProg_(GL_NONE)
  {
  }

  
  GLTimeLineWin::~GLTimeLineWin() 
  {
  }


  void GLTimeLineWin::flush_async(long long msg)
  {
    using Msg = cine2::Simulation::msg_type;

    switch (msg) {
      case Msg::GENERATION:
        const auto& analysis = sim_state_->sim()->analysis();
        float* p = (float*)pbuf_ + g_ * NT;
        float y = 0.f;
        auto s = analysis.prey_summary().back();
        p[0] = y = s.ave_fitness; max_[0] = std::max(max_[0], y);
        p[1] = y = (s.ave_fitness * sim_state_->sim()->param().pred.N) / s.repro_ind; max_[1] = std::max(max_[1], y);
        p[2] = y = static_cast<float>(s.repro_ind); max_[2] = std::max(max_[2], y);
        p[3] = y = static_cast<float>(s.repro_ann); max_[3] = std::max(max_[3], y);
        p[4] = y = s.complexity; 
        s = analysis.pred_summary().back();
        p[5] = y = s.ave_fitness; max_[5] = std::max(max_[5], y);
        p[6] = y = (s.ave_fitness * sim_state_->sim()->param().prey.N) / s.repro_ind; max_[6] = std::max(max_[6], y);
        p[7] = y = static_cast<float>(s.repro_ind); max_[7] = std::max(max_[7], y);
        p[8] = y = static_cast<float>(s.repro_ann); max_[8] = max_[8] = std::max(max_[8], y);
        p[9] = y = s.complexity;
        ++g_;
        ::InvalidateRect(m_hWnd, NULL, TRUE);
        break;
    }
  }


  void GLTimeLineWin::on_create()
  {
    camera_.reset( new glsl::Camera());
    cameraManip_.lookAt(glm::dvec3(0,0,1), glm::dvec3(0), glm::dvec3(0,1,0));
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    rectProg_ = shader::ProgFromLiterals(shader::rectVert, shader::rectFrag);
    lineProg_ = shader::ProgFromLiterals(shader::lineVert, shader::lineFrag);

    // buffer
    G_ = sim_state_->sim()->param().G;
    glCreateBuffers(1, &buf_);
    const auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
    glNamedBufferStorage(buf_, G_ * NT * sizeof(float), nullptr, flags);
    pbuf_ = (float*)glMapNamedBufferRange(buf_, 0, G_ * NT * sizeof(float), flags | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

    // vao
    glCreateVertexArrays(GLsizei(vao_.size()), vao_.data());
    for (size_t i = 0; i < vao_.size(); ++i) {
      glEnableVertexArrayAttrib(vao_[i], 0);
      glVertexArrayAttribBinding(vao_[i], 0, 0);
      glVertexArrayVertexBuffer(vao_[i], 0, buf_, 0, NT * sizeof(float));
      glVertexArrayAttribFormat(vao_[i], 0, 1, GL_FLOAT, GL_FALSE, GLsizei(i * sizeof(float)));
    }
  }


  void GLTimeLineWin::on_close()
  {
    glUseProgram(0);
    glDeleteProgram(rectProg_);
    glDeleteProgram(lineProg_);
    glDeleteBuffers(1, &buf_);
    glDeleteVertexArrays(GLsizei(vao_.size()), vao_.data());
  }


  LRESULT GLTimeLineWin::OnContextMenu(UINT, WPARAM, LPARAM lParam, BOOL&)
  {
    int screenX = GET_X_LPARAM(lParam);
    int screenY = GET_Y_LPARAM(lParam);
    std::array<bool, title_.size()> selected{false};

    selected[selected_] = true; 
    ContextMenu menu(title_.data(), static_cast<int>(title_.size()));
    int selection = menu.track(*this, screenX, screenY, selected.data());
    if (selection >= 0) {
      if (selection < 5) selected_ = selection;
      else if (selection == 5) {
        zoom_[0] = zoom_[1] = 1.0;
        M_ = glm::scale(glm::dmat4(1), glm::dvec3(zoom_[0], zoom_[1], 1.));
        cameraManip_.lookAt(glm::dvec3(0,0,1));
      }
      on_size();
    }
    return 1;
  }


  void GLTimeLineWin::on_size()
  {
    glm::ivec4 vp(5, 5, winW_ - 10, winH_ - 10);
    auto world = glm::dvec4(0.0, 1.0,  0.0, 1.0);
    camera_->setOrthoViewport(vp, world);
    ::InvalidateRect(m_hWnd, NULL, TRUE);
  }


  // [-1,-1,1,1]
  glm::dvec2 GLTimeLineWin::plot_coor(int x, int y)
  {
    auto vp_coor = viewport_coor(x, y, camera_->viewport());
    auto ls_coor = vp_coor - glm::dvec2(cameraManip_.eye());
    return ls_coor * glm::dvec2(zoom_[0], -zoom_[1]);
  }


  void snap()
  {
  }


  void GLTimeLineWin::on_mouse_track(int dx, int dy)
  {
    if (hit_test(trackStartX_, trackStartY_, camera_->viewport())) {
      auto shift = plot_coor(0,0) - plot_coor(dx, dy);
      bool alt = ((KeyState() & 2) == 2);
      cameraManip_.move(alt ? 0.0 : shift.x, alt ? shift.y : 0.0, 0.);
      on_size();
    }
  }


  void GLTimeLineWin::on_mouse_wheel(int zDelta)
  {
    if (hit_test(mouseX_, mouseY_, camera_->viewport())) {
      auto ks = KeyState();
      if (ks == 0) zoom_[0] = std::max(1.0, zoom_[0] + 0.1 * zDelta);
      if ((ks & 2) == 2) zoom_[1] = std::max(1.0, zoom_[1] + 0.1 * zDelta);
      M_ = glm::scale(glm::dmat4(1), glm::dvec3(zoom_[0], zoom_[1], 1.));
      ::InvalidateRect(m_hWnd, NULL, TRUE);
    }
  }


  void GLTimeLineWin::render_timeline(int idx)
  {
    glBindVertexArray(vao_[idx]);
    glUniform2f(1, 1.f / G_, 1.f / max_[idx]);
    glUniform4fv(2, 1, &color_[idx].x);
    glDrawArrays(GL_LINE_STRIP, 0, g_);
  }


  void GLTimeLineWin::render()
  {
    if (g_) glFlushMappedNamedBufferRange(buf_, (g_ - 1) * NT * sizeof(float), NT * sizeof(float));
    // snap camera
    cameraManip_.update(*camera_, 1.0);
    auto eye = glm::dvec2(camera_->eye());
    auto limit = 1.0 - 1.0 / zoom_;
    auto ceye = glm::clamp(eye, -limit, limit);
    cameraManip_.lookAt(glm::dvec3(ceye.x, ceye.y, 1.), glm::dvec3(ceye.x, ceye.y, 0.));
    cameraManip_.update(*camera_, 1.0);

    auto vp = glm::ivec4(camera_->viewport());


    glViewport(vp[0], vp[1], vp[2], vp[3]);
    auto MV = glm::mat4(M_ * camera_->V());
    glUseProgram(rectProg_);
    glUniformMatrix4fv(0, 1, GL_FALSE, &MV[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    auto MVP = MV * glm::mat4(camera_->P());
    glUseProgram(lineProg_);
    glUniformMatrix4fv(0, 1, GL_FALSE, &MVP[0][0]);
    render_timeline(selected_);
    render_timeline(selected_ + 5);
  }


}
