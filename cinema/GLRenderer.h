#ifndef CINEMA_GLRENDERER_H_INCLUDED
#define CINEMA_GLRENDERER_H_INCLUDED


namespace cinema {


  class GLRenderer
  {
  public:
    GLRenderer() {}
    virtual ~GLRenderer() {}

    virtual void on_create() = 0;
    virtual void on_destroy() = 0;
    virtual void on_render() = 0;
    virtual void on_size() {}
    virtual void on_mouse_track(int dx, int dy) {};
    virtual void on_mouse_btn_up(bool left) {};
    virtual void on_mouse_btn_down(bool left) {};
    virtual void on_mouse_wheel(int zDelta) {};
    virtual void on_mouse_hoower(int zDelta) {};
  };

}


#endif
