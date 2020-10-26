#include <atomic>
#include "GLLandscapeWin.h"
#include <glsl/shader.h>
#include <glm/gtc/matrix_transform.hpp>


namespace cinema {


  GLLandscapeWin::GLLandscapeWin(GLSimState* sim_state) 
  : GLWin(sim_state),
    zoom_(1.0),
    landscaleProg_(GL_NONE),
    cameraManip_(1000.0),
    M_(1.0),
    pUniBlk_(nullptr)
  {
    LayerTex_ = GL_NONE;
    selected_.fill(false);
  }

  
  GLLandscapeWin::~GLLandscapeWin() 
  {
  }


  void GLLandscapeWin::flush_async(long long msg)
  {
    using Msg = cine2::Simulation::msg_type;
    switch (msg) {
      case Msg::INITIALIZED:
      case Msg::NEW_GENERATION:
      case Msg::POST_TIMESTEP:
        ::InvalidateRect(m_hWnd, NULL, TRUE);
        break;
    }
  }


  void GLLandscapeWin::on_create()
  {
    layer_mask_ = { 1.f / sim_state_->sim()->param().landscape.foragers_kernel.maxK(),
                    1.f / sim_state_->sim()->param().landscape.klepts_kernel.maxK(),
                    2.f,
                    1.f / sim_state_->sim()->param().landscape.max_item_cap };
    org_layer_mask_ = layer_mask_;
    selected_ = sim_state_->sim()->param().gui.selected;
    camera_.reset( new glsl::Camera());
    cameraManip_.lookAt(glm::dvec3(0,0,1), glm::dvec3(0), glm::dvec3(0,1,0));
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // Layer texture array
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1 , &LayerTex_);
    glTextureStorage3D(LayerTex_, 1, GL_R32F, sim_state_->dim(), sim_state_->dim(), cine2::Landscape::Layers::max_layer);
    glTextureParameteri(LayerTex_, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(LayerTex_, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(LayerTex_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(LayerTex_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    landscaleProg_ = shader::ProgFromLiterals(shader::landscapeVert, shader::landscapeFrag, shader::wrapDupGeo);
  }


  void GLLandscapeWin::on_close()
  {
    glUseProgram(0);
    glDeleteProgram(landscaleProg_);
    glDeleteTextures(1, &LayerTex_);
  }


  LRESULT GLLandscapeWin::OnContextMenu(UINT, WPARAM, LPARAM lParam, BOOL&)
  {
    int screenX = GET_X_LPARAM(lParam);
    int screenY = GET_Y_LPARAM(lParam);
    ContextMenu menu(SELECTION_DISCR, MAX_SELECTION);
    int selection = menu.track(*this, screenX, screenY, selected_.data());
    if (selection >= 0 && selection < 6) {
      if (selected_[selection] = !selected_[selection]) {
        layer_mask_[selection] = org_layer_mask_[selection];
      } 
    }
    ::InvalidateRect(m_hWnd, NULL, TRUE);
    return 1;
  }


  void GLLandscapeWin::on_size()
  {
    float wh = static_cast<float>(sim_state_->dim());
    glm::ivec4 vp(0, 0, winW_ , winH_);
    auto world = glm::dvec4(0, wh, 0, wh);
    auto b = 10;  // border
    vp = (vp[2] > vp[3]) 
        ? glm::ivec4(b + ((vp[2] - vp[3]) >> 1), vp[1] + b, vp[3] - 2*b, vp[3] - 2*b)
        : glm::ivec4(vp.x + b, b + ((vp[3] - vp[2]) >> 1), vp[2] - 2*b, vp[2] - 2*b);
    camera_->setOrthoViewport(vp, world);

    // wrap
    auto eye = camera_->eye();
    if (abs(eye.x) > 2.0) cameraManip_.moveLeft(copysign(2.0, eye.x));
    if (abs(eye.y) > 2.0) cameraManip_.moveUp(copysign(2.0, -eye.y));
    cameraManip_.update(*camera_, 1.0);
    ::InvalidateRect(m_hWnd, NULL, TRUE);
  }


  // [-1,-1,1,1]
  glm::dvec2 GLLandscapeWin::landscape_coor(int x, int y)
  {
    auto vp_coor = viewport_coor(x, y, camera_->viewport());
    auto ls_coor = vp_coor + glm::dvec2(cameraManip_.eye());
    return ls_coor / zoom_;
  }


  void GLLandscapeWin::on_mouse_track(int dx, int dy)
  {
    if (hit_test(trackStartX_, trackStartY_, camera_->viewport())) {
      auto shift = landscape_coor(0,0) - landscape_coor(dx, dy);
      cameraManip_.move(shift.x, -shift.y, 0.);
      on_size();
    }
  }


  void GLLandscapeWin::on_mouse_wheel(int zDelta)
  {
    if (hit_test(mouseX_, mouseY_, camera_->viewport())) {
      auto sca = KeyState();
      if (sca == 0) {
        zoom_ = std::max(1.0, zoom_ + 0.25 * zDelta);
        M_ = glm::scale(glm::dmat4(1), glm::dvec3(zoom_, zoom_, 1.));
        on_size();
        return;
      }
      if (1 == (sca & 5)) {
        layer_mask_[0] *= (zDelta < 0) ? 0.9f : 1.1f;
      }
      if (4 == (sca & 5)) {
        layer_mask_[1] *= (zDelta < 0) ? 0.9f : 1.1f;
      }
      if (5 == (sca & 5)) {
        layer_mask_[2] *= (zDelta < 0) ? 0.9f : 1.1f;
      }
      ::InvalidateRect(m_hWnd, NULL, TRUE);
      return;
    }
  }


  void GLLandscapeWin::render()
  {
    glm::ivec4 vp;
    glm::mat4 MV;
    decltype(selected_) selected;
    decltype(layer_mask_) mask;
    vp = glm::ivec4(camera_->viewport());
    MV = glm::mat4(M_ * camera_->V());
    mask = layer_mask_;
    selected = selected_;

    // unpack
    const int dim = sim_state_->dim();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, sim_state_->vbo(GLSimState::VBO_LAYER));
    glTextureSubImage3D(LayerTex_, 0, 0, 0, 0, dim, dim, 4, GL_RED, GL_FLOAT, nullptr);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);

    glBindBuffer(GL_ARRAY_BUFFER, sim_state_->vbo(GLSimState::VBO_DUMMY));
    glBindVertexArray(sim_state_->vao(GLSimState::VAO_DUMMY));
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, LayerTex_);
    glUseProgram(landscaleProg_);
    glUniformMatrix4fv(0, 1, false, &MV[0][0]);
    for (size_t i=0; i<mask.size(); ++i) {
      mask[i] *= selected[i] ? 1.0f : 0.0f;
    }
    glUniform4fv(2, 1, mask.data());
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }


  const cine2::Landscape::Layers GLLandscapeWin::srcLayers[LAYERS::MAX_LAYER] = {
    cine2::Landscape::Layers::foragers,
    cine2::Landscape::Layers::klepts,
    cine2::Landscape::Layers::handlers,
    cine2::Landscape::Layers::items
  };


  const char* GLLandscapeWin::SELECTION_DISCR[GLLandscapeWin::SELECTION::MAX_SELECTION] = {
    "foragers",
    "klepts",
    "handlers",
    "items"
  };


}
