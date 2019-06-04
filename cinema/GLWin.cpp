#include <memory>
#include <numeric>
#include "GLWin.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glsl/debug.h>
#include <glsl/camera.h>
#include "glsl/shader.h"
#define STBI_ONLY_PNG
//#define STB_IMAGE_IMPLEMENTATION
#include <glsl/stb_image.h>
#include <cine/simulation.h>
#include "stdafx.h"
#include "resource.h"


namespace cinema {


  ContextMenu::ContextMenu(const char* const* items, int n) 
    : hmenu_(0), entries_(0)
  {
    HMENU hmenu = CreatePopupMenu();
    MENUITEMINFO mii;
    memset((void*)&mii, 0, sizeof(MENUITEMINFO));
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    for (int i = 0; i < n; ++items, ++i) {
      mii.wID = i+1;
      CA2T pszt(*items);
      mii.dwTypeData = pszt;
      BOOL ok = InsertMenuItem(hmenu, i+1, FALSE, &mii);
      if (!ok) {
        throw std::exception("ContextMenu::ContextMenu failed");
      }
    }
    hmenu_ = hmenu;
    entries_ = n;
  }

  
  ContextMenu::~ContextMenu(void)
  {
    DestroyMenu(hmenu_);
  }


  int ContextMenu::track(HWND hwnd, int x, int y, bool* checked)
    {
    for (int i = 0; i < entries_; ++i) {
      CheckMenuItem(hmenu_, i+1, checked[i] ? MF_BYCOMMAND | MF_CHECKED :  MF_BYCOMMAND | MF_UNCHECKED);
    }
    BOOL ret = TrackPopupMenuEx(hmenu_, TPM_LEFTALIGN | TPM_RETURNCMD, x, y, hwnd, NULL);
    return static_cast<int>(ret)-1;
  }


  void LoadTextureData(GLuint tex, GLenum texUnit, GLenum target, filesystem::path& path)
  {
    auto FileName = path.string();
    GLint width, height, channels;
    uint32_t* texData = 0;
    texData = (uint32_t*)stbi_load(FileName.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (0 == texData)
    {
      throw std::exception((std::string("Can't read texture map ") + FileName).c_str());
    }
    glActiveTexture(GL_TEXTURE0 + texUnit);
    glBindTexture(target, tex);
    if (target == GL_TEXTURE_1D) {
      glTexImage1D(target, 0, GL_RGBA8, width, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
    }
    else {
      glTexImage2D(target, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
    }
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    stbi_image_free(texData);
  }


  GLuint LoadTexture(GLenum texUnit, GLenum target, filesystem::path& path)
  {
    GLuint tex = 0; glGenTextures(1, &tex);
    LoadTextureData(tex, texUnit, target, path);
    return tex;
  }


  glm::dvec3 cameraScreenDirection(int winX, int winY, int winZ, const glm::dmat4& IVP, const glm::ivec4& viewport)
  {
    auto tmp = glm::dvec4(winX, winY, winZ, 1);
    tmp.x = (tmp.x - viewport[0]) / viewport[2];
    tmp.y = ((viewport[3] - tmp.y) - viewport[1]) / viewport[3];
    tmp = tmp * 2.0 - 1.0;    // point in screen space
    auto obj = IVP * tmp;     // hom. point in world space 
    obj /= obj.w;             // point in world space
    return glm::dvec3(obj);
  }


  void WaitForSync(GLsync& sync)
  {
    while (sync)
    {
      GLenum waitReturn = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
      if (waitReturn == GL_ALREADY_SIGNALED || waitReturn == GL_CONDITION_SATISFIED)
      {
        glDeleteSync(sync);
        sync = nullptr;
        break;
      }
    }
  }


  unsigned KeyState()
  {
    unsigned shift = (GetAsyncKeyState(VK_SHIFT) < 0) ? 1 : 0;
    unsigned alt   = (GetAsyncKeyState(VK_MENU) < 0) ? 2 : 0;
    unsigned ctrl  = (GetAsyncKeyState(VK_CONTROL) < 0) ? 4 : 0;
    return shift | alt | ctrl;
  }


  GLWin::GLWin(GLSimState* sim_state)
  : hDC_(NULL), winW_(0), winH_(0), mouseX_(0), mouseY_(0), tracking_(false), 
    sim_state_(sim_state)
  {
  }


  GLWin::~GLWin()
  {
  }


  LRESULT GLWin::OnCreate(UINT, WPARAM, LPARAM, BOOL& bHandled)
  {
    ::SetPixelFormat(GetDC(), sim_state_->ctx().PixelFormat(), NULL);
    hDC_ = GetDC();
    //glctx_->MakeCurrent(hDC_);
    sim_state_->ctx().SwapInterval(0);
#ifdef GLSL_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);    
    glsl::SetDebugCallback(static_cast<glsl::GLSL_DEBUG_MSG_LEVEL>(GLSL_DEBUG));
#endif
    // pass through child class
    this->on_create();
    bHandled = FALSE;
    return 0;
  }


  LRESULT GLWin::OnClose(UINT, WPARAM, LPARAM, BOOL& bHandled)
  {
    // pass through child class
    this->on_close();
    auto ERR = glGetError();
    bHandled = FALSE;
    return 0;
  }


  LRESULT GLWin::OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL& bHandled)
  {
    if (winW_ && winH_) {
      glsl::ContextGuard _(sim_state_->ctx(), hDC_);
#ifdef GLSL_DEBUG
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);    
      glsl::SetDebugCallback(static_cast<glsl::GLSL_DEBUG_MSG_LEVEL>(GLSL_DEBUG));
#endif
      glViewport(0, 0, winW_, winH_);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      this->render();
      sim_state_->ctx().SwapBuffers(hDC_);
    }
    bHandled = TRUE;
    return 1;
  }


  LRESULT GLWin::OnSize(UINT, WPARAM, LPARAM lParam, BOOL& bHandled)
  {
    winW_ = GET_X_LPARAM(lParam);
    winH_ = GET_Y_LPARAM(lParam);
    on_size();
    bHandled = FALSE;
    return 0;
  }


  LRESULT GLWin::OnLButtonDown(UINT, WPARAM, LPARAM lParam, BOOL&)
  {
    SetCapture();
    tracking_ = true;
    mouseX_ = trackStartX_ = GET_X_LPARAM(lParam);
    mouseY_ = trackStartY_ = GET_Y_LPARAM(lParam);
    on_mouse_btn_down(false);
    return 0;
  }


  LRESULT GLWin::OnLButtonUp(UINT, WPARAM, LPARAM lParam, BOOL&)
  {
    ReleaseCapture();
    tracking_ = false;
    mouseX_ = GET_X_LPARAM(lParam);
    mouseY_ = GET_Y_LPARAM(lParam);
    on_mouse_btn_up(true);
    return 0;
  }


  LRESULT GLWin::OnLButtonDblClick(UINT, WPARAM, LPARAM lParam, BOOL&)
  {
    return 0;
  }


  LRESULT GLWin::OnRButtonDown(UINT, WPARAM, LPARAM lParam, BOOL&)
  {
    mouseX_ = GET_X_LPARAM(lParam);
    mouseY_ = GET_Y_LPARAM(lParam);
    on_mouse_btn_down(false);
    return 0;
  }


  LRESULT GLWin::OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
  {
    auto mouseX = GET_X_LPARAM(lParam);
    auto mouseY = GET_Y_LPARAM(lParam);
    if (tracking_) {
      on_mouse_track(mouseX_ - mouseX, mouseY_ - mouseY);
    }
    mouseX_ = mouseX;
    mouseY_ = mouseY;
    return 0;
  }


  LRESULT GLWin::OnMouseWheel(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
  {
    int zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
    on_mouse_wheel(zDelta);
    return 0;
  }


}
