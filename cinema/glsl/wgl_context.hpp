//
// GLSL support library
// Hanno Hildenbrandt 2008
//

//! \file wgl_context.hpp WGL context creation

#ifndef _WIN32 
  #error wgl not supported
#endif

#ifndef GLSL_WGL_CONTEXT_HPP_INCLUDED
#define GLSL_WGL_CONTEXT_HPP_INCLUDED


#define WIN32_MEAN_AND_LEAN
#include <glsl/glsl.hpp>
#include <glad/glad_wgl.h>
#include <thread>


namespace glsl {

  class Context
  {
  public:
    Context(Context&&);
    Context(const Context&) = delete;
    Context& operator=(Context&&);
    Context& operator=(const Context&) = delete;

    Context();
    explicit Context(::HWND hWnd);
    Context(::HWND hWnd, int colorSamples, int coverageSamples, int* attributes = nullptr);
    ~Context();

    BOOL MakeCurrent(HDC) const;
    BOOL MakeCurrent() const;
    BOOL ReleaseCurrent() const;
    void SwapBuffers(HDC) const;
    void SwapBuffers() const;
    void SwapInterval(int) const;
    int PixelFormat() const;

  private:
    ::HWND hWnd_;     // Creators HWND
    ::HDC hDC_;       // Creators HDC
    ::HGLRC hGLRC_;
  };
  

  class ContextGuard
  {
  public:
    ContextGuard(const Context& ctx, HDC hDC) : ctx_(ctx)
    {
      while (FALSE == ctx_.MakeCurrent(hDC)) {
        if (ERROR_BUSY == ::GetLastError()) {
          std::this_thread::yield();
        }
        else break;   // can't handle this
      }
    }

    ~ContextGuard()
    {
      //ctx_.ReleaseCurrent();
    }

  private:
    const Context& ctx_;
  };

}


#endif
