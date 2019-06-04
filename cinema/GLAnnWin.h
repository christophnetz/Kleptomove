#ifndef CINE2_GLANNWIN_H_INCLUDED
#define CINE2_GLLANWIN_H_INCLUDED

#include <array>
#include <mutex>
#include "GLWin.h"
#include <glsl/camera.h>
#include "glsl/shader.h"
#include "cine/simulation.h"


namespace cinema {


  class GLAnnWin :
    public GLWin
  {
  public:
    GLAnnWin(GLSimState*);
    ~GLAnnWin() override;

    // GLWin interface
    void flush_async(long long msg) override;
    void render() override;
    void on_size() override;
    void on_mouse_track(int dx, int dy) override;
    void on_mouse_wheel(int zDelta) override;

    // Win32 stuff
    BEGIN_MSG_MAP(GLAnnWin)
      CHAIN_MSG_MAP(GLWin);
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
    END_MSG_MAP()


    void on_create() override;
    void on_close() override;
    LRESULT OnContextMenu(UINT, WPARAM, LPARAM, BOOL&);

  private:
    glm::dvec2 landscape_coor(int x, int y);
    
    GLuint rectProg_;
    GLuint annTanhProg_;
    GLuint annLogP1Prog_;
    GLuint colorMap_;
    bool histogram_;
    std::array<std::unique_ptr<struct AnnRenderer>, 2> display_;
  };


}


#endif

