#ifndef GLSL_DEBUG_H_INCLUDED
#define GLSL_DEBUG_H_INCLUDED

#include "glsl.hpp"


namespace glsl
{

  enum GLSL_DEBUG_MSG_LEVEL
  {
    HIGH = 1,
    MEDIUM= 2,
    LOW = 3,
    NOTIFICATION = 4,
  };
      

  void __stdcall GLDebugLogStdErr(GLenum source, GLenum type, GLuint id, GLenum severity,
                                  GLsizei length, const GLchar* message, const GLvoid* userParam);


  void __stdcall GLDebugLogOnceStdErr(GLenum source, GLenum type, GLuint id, GLenum severity,
                                      GLsizei length, const GLchar* message, const GLvoid* userParam);


  void SetDebugCallback(GLSL_DEBUG_MSG_LEVEL level, 
                        GLDEBUGPROC callback = GLDebugLogOnceStdErr, 
                        void* userData = nullptr);

}

#endif
