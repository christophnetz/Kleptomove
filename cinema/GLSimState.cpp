#include "GLSimState.h"
#include <experimental/filesystem>
#include <cine/simulation.h>
#include <glsl/shader.h>
#include <glsl/debug.h>


namespace bmf = glsl::bmfont;
namespace filesystem = std::experimental::filesystem;


namespace cinema {

  extern void LoadTextureData(GLuint tex, GLenum texUnit, GLenum target, filesystem::path& path);
  extern GLuint LoadTexture(GLenum texUnit, GLenum target, filesystem::path& path);


  GLSimState::GLSimState(HWND hWnd, const cine2::Simulation* sim)
  : dim_(sim->dim()),
    prey_ann_{ static_cast<int>(sim->prey().pop.size()),
           sim->prey().ann->state_size(),
           sim->prey().ann->stride(),
           sim->prey().ann->type_size() },
    //pred_ann_{ static_cast<int>(sim->pred().pop.size()),
    //       sim->pred().ann->state_size(),
    //       sim->pred().ann->stride(),
    //       sim->pred().ann->type_size() },
    glctx_(hWnd),
    sim_(sim)
  {
    ColorMap_.assign(GL_NONE);
    glctx_.MakeCurrent();
#ifdef GLSL_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glsl::SetDebugCallback(static_cast<glsl::GLSL_DEBUG_MSG_LEVEL>(GLSL_DEBUG));
#endif
    glCreateBuffers(VBO_MAX, vbo_.data());
    const auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    GLsizei size = static_cast<GLsizei>(prey_ann_.type_size) * prey_ann_.N;
    glNamedBufferStorage(vbo_[VBO_PREY_ANN], size, nullptr, flags);
    ptr_[VBO::VBO_PREY_ANN] = glMapNamedBufferRange(vbo_[VBO_PREY_ANN], 0, size, flags);
    
    //size = static_cast<GLsizei>(pred_ann_.type_size) * pred_ann_.N;
    //glNamedBufferStorage(vbo_[VBO_PRED_ANN], size, nullptr, flags);
    //ptr_[VBO::VBO_PRED_ANN] = glMapNamedBufferRange(vbo_[VBO_PRED_ANN], 0, size, flags);
    
    size = static_cast<GLsizei>(4 * sizeof(float) * dim_ * dim_);
    glNamedBufferStorage(vbo_[VBO_LAYER], size, nullptr, flags);
    ptr_[VBO::VBO_LAYER] = glMapNamedBufferRange(vbo_[VBO_LAYER], 0, size, flags);

    // dummy vao
    glCreateVertexArrays(VAO_MAX, vao_.data());
    glNamedBufferStorage(vbo_[VBO_DUMMY], 128, nullptr, GL_DYNAMIC_STORAGE_BIT); 
    glEnableVertexArrayAttrib(vao_[VAO_DUMMY], 0);
    glVertexArrayVertexBuffer(vao_[VAO_DUMMY], 0, vbo_[VBO::VBO_DUMMY], 0, 0);
    glVertexArrayAttribFormat(vao_[VAO_DUMMY], 0, 2, GL_SHORT, GL_FALSE, 0);
    ptr_[VBO::VBO_DUMMY] = nullptr;  // unmapped

    // setup font stuff
    TCHAR buf[MAX_PATH];
    auto hModule = GetModuleHandle(NULL);
    GetModuleFileName(hModule, buf, MAX_PATH);
    auto fontPath = filesystem::path(buf).parent_path() / ".." / "media" / "Fonts";
    Faces_["small"] = bmf::Font::Create((fontPath / "Verdana12.fnt").string().c_str());
    text2D_.reset( new bmf::Text2D(Faces_.begin()->second) );
    textProg_ = shader::ProgFromLiterals(shader::TextVert, shader::TextFrag, shader::TextGeo);

    // color map textures
    glGenTextures(static_cast<GLsizei>(ColorMap_.size()), ColorMap_.data());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, ColorMap_[0]);
    std::array<GLubyte, 4 * 512> gradient;
    gradient.fill(0);
    for (int i=0; i < 256; ++i) {
      gradient[4*i+2] = GLubyte(i);
      gradient[4*i+3] = 255;
    }
    for (int i=256; i < 512; ++i) {
      gradient[4*i] = GLubyte(i-255);
      gradient[4*i+3] = 255;
    }
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, gradient.data());
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    LoadTextureData(ColorMap_[1], 0, GL_TEXTURE_1D, filesystem::path(exePath_) / "../media/bone.png");
    LoadTextureData(ColorMap_[2], 0, GL_TEXTURE_1D, filesystem::path(exePath_) / "../media/cool.png");
    LoadTextureData(ColorMap_[3], 0, GL_TEXTURE_1D, filesystem::path(exePath_) / "../media/hot.png");
    LoadTextureData(ColorMap_[4], 0, GL_TEXTURE_1D, filesystem::path(exePath_) / "../media/hsv.png");
    LoadTextureData(ColorMap_[5], 0, GL_TEXTURE_1D, filesystem::path(exePath_) / "../media/spectrum.png");

    // copy capacity layer
    auto dst = (float*)ptr_[VBO_LAYER] + 3 * dim_ * dim_;
    std::memcpy(dst, sim->landscape()[cine2::Landscape::Layers::capacity].data(), dim_ * dim_ * sizeof(float));
  }


  GLSimState::~GLSimState()
  {
    glDeleteTextures(static_cast<GLsizei>(ColorMap_.size()), ColorMap_.data());
    glDeleteBuffers(VBO_MAX, vbo_.data());
    glDeleteVertexArrays(VAO_MAX, vao_.data());
  }


  void GLSimState::flush_async(const cine2::Simulation& sim, long long msg)
  {
    using msg_type = cine2::Simulation::msg_type;
    using Layers = cine2::Landscape::Layers;

    switch (msg) {
    case msg_type::INITIALIZED:
    case msg_type::NEW_GENERATION:
      std::memcpy(ptr_[VBO::VBO_PREY_ANN], sim.prey().ann->data(), prey_ann_.N * prey_ann_.type_size);
      //std::memcpy(ptr_[VBO::VBO_PRED_ANN], sim.pred().ann->data(), pred_ann_.N * pred_ann_.type_size);
    case msg_type::POST_TIMESTEP: {
      std::memcpy(ptr_[VBO::VBO_LAYER], sim.landscape().data(), 3 * dim_* dim_ * sizeof(float));
      break;
    }
    }
  }


}


