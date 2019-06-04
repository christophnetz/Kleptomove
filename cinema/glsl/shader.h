#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include "glm/glm.hpp"
#include "glsl/glsl.hpp"
#include <cine/landscape.h>


namespace shader {


  struct IndividualProxy
  {
    cine2::Coordinate pos;
    float food;
    int attacks;
    float fitness;
  };


  extern const char* landscapeVert;
  extern const char* landscapeFrag;
  extern const char* rectVert;
  extern const char* rectFrag;
  extern const char* lineVert;
  extern const char* lineFrag;
  extern const char* annVert;
  extern const char* annTanhFrag;
  extern const char* annLogP1Frag;

  extern const char* wrapDupGeo;

  extern const char* TextVert;
  extern const char* TextGeo;
  extern const char* TextFrag;


  GLuint ProgFromLiterals(const char* vertexShader, const char* fragmentShader, const char* geometyShader = nullptr);

}

#endif
