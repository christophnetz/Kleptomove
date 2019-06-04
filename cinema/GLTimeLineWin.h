#ifndef CINE2_GLTIMELINEWIN_H_INCLUDED
#define CINE2_GLTIMELINEWIN_H_INCLUDED

#include <array>
#include <mutex>
#include "GLWin.h"
#include <glsl/camera.h>
#include "glsl/shader.h"
#include "cine/simulation.h"


namespace cinema {


  class GLTimeLineWin :
    public GLWin
  {
    static const int NT = 10;

  public:
    GLTimeLineWin(GLSimState*);
    ~GLTimeLineWin() override;

    // GLWin interface
    void flush_async(long long msg) override;
    void render() override;
    void on_size() override;
    void on_mouse_track(int dx, int dy) override;
    void on_mouse_wheel(int zDelta) override;

    // Win32 stuff
    BEGIN_MSG_MAP(GLTimeLineWin)
      CHAIN_MSG_MAP(GLWin);
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
    END_MSG_MAP()


    void on_create() override;
    void on_close() override;
    LRESULT OnContextMenu(UINT, WPARAM, LPARAM, BOOL&);

    std::string title() const { return title_[selected_]; }

  private:
    void render_timeline(int idx);
    glm::dvec2 plot_coor(int x, int y);
    void snap_camera();
    
    static const std::array<const char*, 5+1> title_;
    static const std::array<glm::vec4, NT> color_;

    int G_, g_;
    float max_[NT];
    int selected_;
    GLuint buf_;
    float* pbuf_;
    std::array<GLuint, NT> vao_;
    std::unique_ptr<glsl::Camera> camera_;
    glsl::CameraManip cameraManip_;
    glm::dvec2 zoom_;
    glm::dmat4 M_;
    GLuint rectProg_;
    GLuint lineProg_;
  };


}


#endif

