#ifndef CINEMA_GLWIN_H_INCLUDED
#define CINEMA_GLWIN_H_INCLUDED


#include "stdafx.h"
#include <memory>
#include <mutex>
#include <array>
#include <string>
#include <glsl/wgl_context.hpp>
#include <glsl/bmfont.hpp>
#include <filesystem>
#include "GLSimState.h"


namespace filesystem = std::filesystem;


#define MESSAGE_HANDLER_SYNC(msg, func) \
	if(uMsg == msg) \
	{ \
    std::lock_guard<std::recursive_mutex> __(mutex_); \
		bHandled = TRUE; \
		lResult = func(uMsg, wParam, lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}


namespace cine2 {

  class Simulation;

};


namespace cinema {


  class GLWin : 
    public CWindowImpl<GLWin>
  {
  public:
    DECLARE_WND_CLASS_EX(0, CS_OWNDC | CS_DBLCLKS, NULL);

    explicit GLWin(GLSimState* sim_state);
    virtual ~GLWin();

    BEGIN_MSG_MAP(GLWin)
      MESSAGE_HANDLER_SYNC(WM_CREATE, OnCreate);
      MESSAGE_HANDLER_SYNC(WM_CLOSE, OnClose);
      MESSAGE_HANDLER_SYNC(WM_ERASEBKGND, OnEraseBkgnd);
      MESSAGE_HANDLER_SYNC(WM_SIZE, OnSize);
      MESSAGE_HANDLER_SYNC(WM_LBUTTONDOWN, OnLButtonDown);
      MESSAGE_HANDLER_SYNC(WM_LBUTTONUP, OnLButtonUp);
      MESSAGE_HANDLER_SYNC(WM_LBUTTONDBLCLK, OnLButtonDblClick);
      MESSAGE_HANDLER_SYNC(WM_RBUTTONDOWN, OnRButtonDown);
      MESSAGE_HANDLER_SYNC(WM_MOUSEMOVE, OnMouseMove);
      MESSAGE_HANDLER_SYNC(WM_MOUSEWHEEL, OnMouseWheel);
    END_MSG_MAP()

    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL& bHandled);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL& bHandled);

    LRESULT OnSize(UINT, WPARAM, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnLButtonUp(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnLButtonDblClick(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnRButtonDown(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnMouseMove(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnMouseWheel(UINT, WPARAM, LPARAM, BOOL&);

    // windows coordinate in viewport?
    template <typename VP>
    bool hit_test(int x, int y, const VP& vp) const
    {
      return (x >= int(vp[0]) && y >= int(vp[1]) && x <= int(vp[0] + vp[2]) && y <= int(vp[1] + vp[3]));
    }

    // windows (x,y) -> [-1,-1,1,1];
    template <typename VP>
    glm::dvec2 viewport_coor(int x, int y, const VP& vp) const
    {
      double x_ = double(x) - vp[0];
      double y_ = double(winH_ - y) - vp[1];
      return glm::dvec2(2. * x_ / vp[2] - 1., 2. * y_ / vp[3] - 1.);
    }

  protected:
    virtual void flush_async(long long msg) = 0;
    virtual void render() = 0;
    virtual void post_render() {};
    virtual void on_create() = 0;
    virtual void on_close() = 0;
    virtual void on_size() {}
    virtual void on_mouse_track(int dx, int dy) {};
    virtual void on_mouse_btn_up(bool left) {};
    virtual void on_mouse_btn_down(bool left) {};
    virtual void on_mouse_wheel(int zDelta) {};
    virtual void on_mouse_hoower(int zDelta) {};

    HDC hDC_;
    int winW_;
    int winH_;
    int mouseX_;
    int mouseY_;
    int trackStartX_;
    int trackStartY_;
    bool tracking_;

    std::string exePath_;
    std::recursive_mutex mutex_;
    GLSimState* sim_state_;
  };


  // returns shift(1) | alt(2) | ctrl(4)
  unsigned KeyState();
  void LoadTextureData(GLuint tex, GLenum texUnit, GLenum target, filesystem::path& path);
  GLuint LoadTexture(GLenum texUnit, GLenum target, filesystem::path& path);
  glm::dvec3 cameraScreenDirection(int winX, int winY, int winZ, const glm::dmat4& IVP, const glm::ivec4& viewport);
  void WaitForSync(GLsync& sync);


  class ContextMenu
  {
  public:
    ContextMenu(const char* const* items, int n);
    ~ContextMenu(void);
    int track(HWND hwnd, int x, int y, bool* checked);
 
  private:
    HMENU hmenu_; 
    int entries_;
  };


}

#endif

