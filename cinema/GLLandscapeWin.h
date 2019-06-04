#ifndef CINE2_GLLANDSCAPE_WIN_H_INCLUDED
#define CINE2_GLLANDSCAPE_WIN_H_INCLUDED

#include <array>
#include "GLWin.h"
#include <glsl/camera.h>
#include "cine/simulation.h"


namespace cinema {


  class GLLandscapeWin :
    public GLWin
  {
  public:  
    GLLandscapeWin(GLSimState*);
    ~GLLandscapeWin() override;

    // GLWin interface
    void flush_async(long long msg) override;
    void render() override;
    void on_size() override;
    void on_mouse_track(int dx, int dy) override;
    void on_mouse_wheel(int zDelta) override;

    // Win32 stuff
    BEGIN_MSG_MAP(GLLandscapeWin)
      CHAIN_MSG_MAP(GLWin);
      MESSAGE_HANDLER_SYNC(WM_CONTEXTMENU, OnContextMenu);
    END_MSG_MAP()


    void on_create() override;
    void on_close() override;
    LRESULT OnContextMenu(UINT, WPARAM, LPARAM, BOOL&);

  private:
    glm::dvec2 landscape_coor(int x, int y);
    
    enum SELECTION {
      SELECTION_PREY,
      SELECTION_PRED,
      SELECTION_GRASS,
      SELECTION_RISK,
      MAX_SELECTION
    };

    enum LAYERS {
      LAYER_PREY,
      LAYER_PRED,
      LAYER_GRASS,
      LAYER_RISK,
      MAX_LAYER
    };

    static const cine2::Landscape::Layers srcLayers[LAYERS::MAX_LAYER];
    static const char* SELECTION_DISCR[MAX_SELECTION];

    double zoom_;
    GLuint landscaleProg_;
    GLuint LayerTex_;
    std::unique_ptr<glsl::Camera> camera_;
    glsl::CameraManip cameraManip_;
    glm::dmat4 M_;    // model matrix;

    void* pUniBlk_;
    void* pPos_;
    void* pFitness_;
    std::array<bool, SELECTION::MAX_SELECTION> selected_;
    std::array<float, 4> org_layer_mask_;
    std::array<float, 4> layer_mask_;
  };


}


#endif

