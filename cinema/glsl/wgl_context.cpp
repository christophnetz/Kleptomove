#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <tchar.h>
#include "wgl_context.hpp"


namespace glsl { 
  
  
  class glslDummyWin
  {
    static const TCHAR* ClassName;
    static const TCHAR* WindowName;

  public:
    glslDummyWin() : hInstance(NULL), hWnd(NULL), cAtom(NULL), hRC(NULL)  
    {
      GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, nullptr, &hInstance);
      WNDCLASS wc;
      memset((void*)&wc, 0, sizeof(WNDCLASS));
      wc.style = CS_OWNDC;
      wc.lpfnWndProc = &WndProc;
      wc.hInstance = hInstance;
      wc.lpszClassName = ClassName;
      cAtom = RegisterClass(&wc);
      hWnd = CreateWindow( 
        ClassName,
        WindowName,
        WS_OVERLAPPED, 
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        (HWND) NULL,
        (HMENU) NULL,
        hInstance, 
        (LPVOID)(this));    
    }


    void Destroy() 
    {
      HDC hDC = GetDC(hWnd);
      wglMakeCurrent(hDC, NULL);
      wglDeleteContext(hRC); hRC = NULL;
      DestroyWindow(hWnd);
      UnregisterClass(ClassName, hInstance);
      FreeLibrary(hInstance);
      ReleaseDC(hWnd, hDC);
    }


    static LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
    {
      switch (uMsg)
      {
        case WM_CREATE:
        {
          glslDummyWin* self = (glslDummyWin*)((LPCREATESTRUCT)(lParam))->lpCreateParams;
          HDC hDC = GetDC(hWnd);
          PIXELFORMATDESCRIPTOR pfd;
          memset((void*)&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
          pfd.nSize  = sizeof(PIXELFORMATDESCRIPTOR);
          pfd.nVersion   = 1;
          pfd.dwFlags    = PFD_SUPPORT_OPENGL;
          pfd.iPixelType = PFD_TYPE_RGBA;
          int nPixelFormat = ChoosePixelFormat(hDC, &pfd);
          if (nPixelFormat && SetPixelFormat (hDC, nPixelFormat, &pfd))
          {
            self->hRC = wglCreateContext(hDC);
            wglMakeCurrent(hDC, self->hRC);
          }
          ReleaseDC(hWnd, hDC);
          return 0;
        }
      }
      return DefWindowProc(hWnd, uMsg, wParam, lParam); 
    }

  public:
    ATOM    cAtom;
    HWND    hWnd;
    HGLRC   hRC;
    HMODULE hInstance;
  };

  
  const TCHAR* glslDummyWin::ClassName = _T("{25324927-A9BE-4399-B6E9-F8F5966BB528}");
  const TCHAR* glslDummyWin::WindowName = _T("{B68FEF08-7169-4661-84C1-29B5F705099C}");


  HGLRC CreateContext(HDC hDC, int* attributes, int colorSamples, int coverageSamples, int dblBuf)
  {
    HGLRC hRC = NULL;
    glslDummyWin dWin;
    if (gladLoadWGL(hDC))
    {
      int wgl_sample_buffer_arb = 0;
      int wgl_samples_arb = 0;
      int wgl_coverage_samples = 0;
      if (colorSamples > 0)
      {
        wgl_sample_buffer_arb = WGL_SAMPLE_BUFFERS_ARB;
        if ((0 == coverageSamples) || !GLAD_WGL_NV_multisample_coverage)
        {
          wgl_samples_arb = WGL_SAMPLES_ARB;
        }
        else if (GLAD_WGL_NV_multisample_coverage)
        {
          wgl_samples_arb = WGL_COLOR_SAMPLES_NV;
          wgl_coverage_samples = WGL_COVERAGE_SAMPLES_NV;
        }
      }
      int iAttribs[2][24] = { 
      { 
        WGL_SUPPORT_OPENGL_ARB,   1,                          // Must support OGL rendering
        WGL_DRAW_TO_WINDOW_ARB,   1,                          // pf that can run a window
        WGL_ACCELERATION_ARB,     WGL_FULL_ACCELERATION_ARB,  // must be HW accelerated
        WGL_COLOR_BITS_ARB,       32,                         // 8 bits of each R, G and B
        WGL_DEPTH_BITS_ARB,       24,                         // 24 bits of depth precision for window
        WGL_PIXEL_TYPE_ARB,       WGL_TYPE_RGBA_ARB,          // pf should be RGBA type
        wgl_sample_buffer_arb,    (colorSamples ? 1 : 0),
        wgl_samples_arb,          colorSamples,
        wgl_coverage_samples,     coverageSamples,
        0, 0 
      },
      { 
        WGL_SUPPORT_OPENGL_ARB,   1,                          // Must support OGL rendering
        WGL_DRAW_TO_WINDOW_ARB,   1,                          // pf that can run a window
        WGL_ACCELERATION_ARB,     WGL_FULL_ACCELERATION_ARB,  // must be HW accelerated
        WGL_COLOR_BITS_ARB,       32,                         // 8 bits of each R, G and B
        WGL_DEPTH_BITS_ARB,       24,                         // 24 bits of depth precision for window
        WGL_PIXEL_TYPE_ARB,       WGL_TYPE_RGBA_ARB,          // pf should be RGBA type
        WGL_SWAP_METHOD_ARB,      WGL_SWAP_EXCHANGE_ARB,      // Exchange, don't copy
        WGL_DOUBLE_BUFFER_ARB,    1,                          // Double buffered context
        wgl_sample_buffer_arb,    (colorSamples ? 1 : 0),
        wgl_samples_arb,          colorSamples,
        wgl_coverage_samples,     coverageSamples,
        0, 0 
      }};
      FLOAT fAttribs[] = { 0, 0 };
      int pixelFormat;
      UINT numFormats;
      BOOL e = wglChoosePixelFormatARB(hDC, iAttribs[dblBuf], fAttribs, 1, &pixelFormat, &numFormats);
      if (numFormats)
      {
        if (SetPixelFormat(hDC, pixelFormat, NULL))
        {
          hRC = wglCreateContextAttribsARB(hDC, 0, attributes);
        }
      }
    }
    dWin.Destroy();
    wglMakeCurrent(hDC, hRC);
    if (!gladLoadGL()) {
      wglMakeCurrent(hDC, NULL);
      wglDeleteContext(hRC);
      return NULL;
    }
    return hRC;
  }
  
  
  HGLRC CreateContext(HDC hDC)
  {
    HGLRC hGLRC = NULL;
    int iAttribs[] =
    {
      WGL_CONTEXT_MAJOR_VERSION_ARB, GLSL_OPENGL_MAJOR_VERSION,
      WGL_CONTEXT_MINOR_VERSION_ARB, GLSL_OPENGL_MINOR_VERSION,
      WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef GLSL_DEBUG
      WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
      0, 0,
    };
    if ((hGLRC = CreateContext(hDC, iAttribs, 4, 8, 1))) 
    {
      glEnable(GL_MULTISAMPLE);
    }
    else 
    {
      return NULL;
    }
    return hGLRC;
  }


  Context::Context(Context&& ctx) : hWnd_(ctx.hWnd_), hDC_(ctx.hDC_), hGLRC_(ctx.hGLRC_)
  {
    ctx.hWnd_ = NULL;
    ctx.hDC_ = NULL;
    ctx.hGLRC_ = NULL;
  }

  Context::Context() : hWnd_(NULL), hDC_(NULL), hGLRC_(NULL)
  {
  }


  Context& Context::operator=(Context&& ctx)
  {
    hWnd_ = ctx.hWnd_; hDC_ = ctx.hDC_; hGLRC_ = ctx.hGLRC_;
    ctx.hWnd_ = NULL;
    ctx.hDC_ = NULL;
    ctx.hGLRC_ = NULL;
    return *this;
  }


  Context::Context(::HWND hWnd) : Context() 
  {
    hWnd_ = hWnd;
    hDC_ = GetDC(hWnd_);
    hGLRC_ = CreateContext(hDC_);
  }


  Context::Context(::HWND hWnd, int colorSamples, int coverageSamples, int* attributes) : Context()
  {
    hWnd_ = hWnd;
    hDC_ = GetDC(hWnd_);
    int iAttribs[] =
    {
      WGL_CONTEXT_MAJOR_VERSION_ARB, GLSL_OPENGL_MAJOR_VERSION,
      WGL_CONTEXT_MINOR_VERSION_ARB, GLSL_OPENGL_MINOR_VERSION,
      WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef GLSL_DEBUG
      WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
      0, 0,
    };
    hGLRC_ = CreateContext(hDC_, attributes ? attributes : iAttribs, colorSamples, coverageSamples, 1);
  }


  Context::~Context()
  {
    ReleaseCurrent();
    if (hGLRC_) wglDeleteContext(hGLRC_);
    if (hDC_) ReleaseDC(hWnd_, hDC_);
  }


  BOOL Context::MakeCurrent(HDC hDC) const
  {
    return wglMakeCurrent(hDC, hGLRC_);
  }


  BOOL Context::MakeCurrent() const
  {
    return wglMakeCurrent(hDC_, hGLRC_);
  }


  BOOL Context::ReleaseCurrent() const
  {
    return wglMakeCurrent(NULL, NULL);
  }


  void Context::SwapBuffers(HDC hDC) const
  {
    ::SwapBuffers(hDC);
  }


  void Context::SwapBuffers() const
  {
    ::SwapBuffers(hDC_);
  }


  void Context::SwapInterval(int I) const
  {
    wglSwapIntervalEXT(I);
  }


  int Context::PixelFormat() const 
  {
    return ::GetPixelFormat(hDC_);
  }

} // namespace glsl



#endif
