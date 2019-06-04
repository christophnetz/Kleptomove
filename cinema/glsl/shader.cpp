#include "shader.h"
#include <string>


namespace shader {


  const char* version = "#version 440 core\n";

  const char* landscapeVert = R"glsl(
    
    flat out vec2 vTex;            // [0,1]
    flat out vec4 vVertex;         // [-1,1]

    const vec2[6] vertices = vec2[6]( 
      vec2(1, 1), vec2(1, 0), vec2(0, 1),
      vec2(0, 1), vec2(1, 0), vec2(0, 0)
    );

    void main(void)
    {
      vTex = vertices[gl_VertexID];
      vVertex = vec4(2.0*vTex.x-1.0, 2.0*vTex.y-1.0, 0.0, 1.0);
    }

  )glsl";


  const char* wrapDupGeo = R"glsl(

    layout (triangles) in;
    layout (triangle_strip, max_vertices=27) out;
    layout (location = 0) uniform mat4 MV;

    flat in vec2 vTex[3];
    flat in vec4 vVertex[3];

    smooth out vec2 gTex;


    void EmitTriangle(vec4 ofs)
    {
      gTex = vTex[0];
      gl_Position = MV * (vVertex[0] + ofs);
      EmitVertex();            
      gTex = vTex[1];
      gl_Position = MV * (vVertex[1] + ofs);
      EmitVertex();            
      gTex = vTex[2];
      gl_Position = MV * (vVertex[2] + ofs);
      EmitVertex();     
      EndPrimitive();
    }


    void main()
    {
      float s = 2.0;
      EmitTriangle(vec4(0, 0, 0, 0));
      EmitTriangle(vec4(s, 0, 0, 0));
      EmitTriangle(vec4(-s, 0, 0, 0));
      EmitTriangle(vec4(0, s, 0, 0));
      EmitTriangle(vec4(0, -s, 0, 0));
      EmitTriangle(vec4(-s, -s, 0, 0));
      EmitTriangle(vec4(s, -s, 0, 0));
      EmitTriangle(vec4(-s, s, 0, 0));
      EmitTriangle(vec4(s, s, 0, 0));
    }
    
  )glsl";

  
  const char* landscapeFrag = R"glsl(

    layout (binding = 1) uniform sampler2DArray Texture;
    layout (location = 2) uniform vec4 layer_mask;

    smooth in vec2 gTex;
    out vec4 FragColor;

    void main(void)
    {
      vec3(gTex.x, gTex.y, 0);
      float prey = layer_mask.x * texture(Texture, vec3(gTex.x, gTex.y, 0)).r;
      float pred = layer_mask.y * texture(Texture, vec3(gTex.x, gTex.y, 1)).r;
      float grass = layer_mask.z * texture(Texture, vec3(gTex.x, gTex.y, 2)).r;
      float risk = layer_mask.w * texture(Texture, vec3(gTex.x, gTex.y, 3)).r;
      
      // draw grass below pred & prey
      //grass *= (0.0 < (prey + pred)) ? 0.0 : 1.0;
      FragColor.rgb = vec3(pred, grass, prey) + risk;
    }

  )glsl";


  const char* rectVert = R"glsl(
    
    layout (location = 0) uniform mat4 MV;
    
    const vec2[6] vertices = vec2[6]( 
      vec2(1, 1), vec2(1, 0), vec2(0, 1),
      vec2(0, 1), vec2(1, 0), vec2(0, 0)
    );

    void main(void)
    {
      vec2 vTex = vertices[gl_VertexID];
      gl_Position = MV * vec4(2.0*vTex.x-1.0, 2.0*vTex.y-1.0, 0.0, 1.0);
    }

  )glsl";


  const char* rectFrag = R"glsl(
    
    layout (location = 1) uniform vec4 color = vec4(0.1,0.1,0.1,1.0);
    
    out vec4 FragColor;

    void main(void)
    {
      FragColor = color;
    }

  )glsl";


  const char* lineVert = R"glsl(
    
    layout (location = 0) uniform mat4 MV;
    layout (location = 1) uniform vec2 scale;
    layout (location = 0) in float y;

    void main(void)
    {
      vec4 pos = vec4(scale.x * gl_VertexID, scale.y * y, 0, 1);
      gl_Position = MV * pos;
    }

  )glsl";


  const char* lineFrag = R"glsl(
    
    layout (location = 2) uniform vec4 color;
    out vec4 FragColor;

    void main(void)
    {
      FragColor = color;
    }

  )glsl";


  const char* annVert = R"glsl(
    
    smooth out vec2 vTex;            // [0,1]
    layout (location = 0) uniform mat4 MV;
    
    const vec2[6] vertices = vec2[6]( 
      vec2(1, 1), vec2(1, 0), vec2(0, 1),
      vec2(0, 1), vec2(1, 0), vec2(0, 0)
    );

    void main(void)
    {
      vTex = vertices[gl_VertexID];
      gl_Position = MV * vec4(2.0*vTex.x-1.0, 2.0*vTex.y-1.0, 0.0, 1.0);
    }

  )glsl";


  const char* annTanhFrag = R"glsl(

    layout (binding = 0) uniform sampler1D colorMap;
    layout (binding = 1) uniform sampler2D Texture;
    layout (location = 2) uniform float scale;


    smooth in vec2 vTex;
    out vec4 FragColor;

    void main(void)
    {
      float x = texture(Texture, vTex).r;
      if (x == 0.0) discard;
      float val = 0.5 + scale * tanh(x);
      FragColor = texture(colorMap, val);
    }

  )glsl";


  const char* annLogP1Frag = R"glsl(

    layout (binding = 0) uniform sampler1D colorMap;
    layout (binding = 1) uniform sampler2D Texture;
    layout (location = 2) uniform float scale;

    smooth in vec2 vTex;
    out vec4 FragColor;

    void main(void)
    {
      float x = texture(Texture, vTex).r;
      if (x == 0.0) discard;
      float val = scale * log(x + 1.0);
      FragColor = texture(colorMap, val);
    }

  )glsl";


  const char* TextVert = R"glsl(

    layout (location = 0) in vec2 TexCoord;
    layout (location = 1) in vec2 Vertex;
    layout (location = 2) in vec4 Color;

    out vec2 vTexCoord;
    out vec2 vVertex;
    out vec4 vColor;

    void main()
    {
      vColor = Color;
      vTexCoord = TexCoord;
      vVertex = Vertex;
    }

  )glsl";


  const char* TextGeo = R"glsl(

    layout(lines) in;
    layout(triangle_strip, max_vertices=4) out;

    layout (location = 0) uniform mat4 Ortho;

    in vec2 vTexCoord[2];
    in vec2 vVertex[2];
    in vec4 vColor[2];

    smooth out vec2 gTexCoord;
    flat out vec4 gColor;

    void main()
    {
      vec2 t0 = vTexCoord[0];
      vec2 t1 = vTexCoord[1];
      vec2 v0 = vVertex[0];
      vec2 v1 = vVertex[1];

      gColor = vColor[1];
      gTexCoord = vec2(t0.x, t1.y);
      gl_Position = Ortho * vec4(v0.x, v1.y, 0, 1);
      EmitVertex();

      gTexCoord = vec2(t0.x, t0.y);
      gl_Position = Ortho * vec4(v0.x, v0.y, 0, 1);
      EmitVertex();

      gTexCoord = vec2(t1.x, t1.y);
      gl_Position = Ortho * vec4(v1.x, v1.y, 0, 1);
      EmitVertex();

      gTexCoord = vec2(t1.x, t0.y);
      gl_Position = Ortho * vec4(v1.x, v0.y, 0, 1);
      EmitVertex();

      EndPrimitive();
    }
  
  )glsl";


  const char* TextFrag = R"glsl(

    #extension GL_ARB_texture_rectangle : enable
  
    uniform sampler2DRect FontTexture;

    smooth in vec2 gTexCoord;
    flat in vec4 gColor;

    layout (location = 1) out vec4 FragColor;

    void main()
    {
      vec4 color = gColor;
      color.a = texture2DRect(FontTexture, gTexCoord).r;
      FragColor = color;
    }

  )glsl";


  namespace {

    GLuint ShaderFromLiteral(const char* shaderSource, GLenum shaderType)
    {
      auto sh = glCreateShader(shaderType);
      glShaderSource(sh, 1, &shaderSource, 0);
      glCompileShader(sh);
      return sh;
    }

  }


  GLuint ProgFromLiterals(const char* vertexShader, const char* fragmentShader, const char* geometryShader)
  {
    std::string mb = shader::version;
    auto vSh = ShaderFromLiteral((mb + vertexShader).c_str(), GL_VERTEX_SHADER);
    auto fSh = ShaderFromLiteral((mb + fragmentShader).c_str(), GL_FRAGMENT_SHADER);
    GLuint gSh = (geometryShader) ? ShaderFromLiteral((mb + geometryShader).c_str(), GL_GEOMETRY_SHADER) : 0;
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vSh);
    glAttachShader(prog, fSh);
    if (gSh) glAttachShader(prog, gSh);
    glLinkProgram(prog);
    glDetachShader(prog, vSh);
    glDetachShader(prog, fSh);
    glDeleteShader(vSh);
    glDeleteShader(fSh);
    if (gSh) 
    {
      glDetachShader(prog, gSh);
      glDeleteShader(gSh);
    }
#ifdef GLSL_DEBUG
    GLchar buf[1024];
    GLsizei length;
    glGetProgramInfoLog(prog, 1024, &length, buf);
    if (length) {
      glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 1, GL_DEBUG_SEVERITY_HIGH, length, buf);
    }
#endif
    return prog;
  }


}
