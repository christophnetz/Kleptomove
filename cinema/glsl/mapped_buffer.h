#ifndef GLSL_MAPPED_BUFFER_H_INCLUDED
#define GLSL_MAPPED_BUFFER_H_INCLUDED


#include <memory>
#include "glsl.hpp"


namespace glsl {


  class mapped_buffer
  {
  public:
    mapped_buffer(const mapped_buffer&) = delete;
    mapped_buffer& operator=(const mapped_buffer&) = delete;

    mapped_buffer(mapped_buffer&& rhs) 
    {
      *this = std::move(rhs);
    }

    mapped_buffer& operator=(mapped_buffer&& rhs)
    {
      size_ = rhs.size_; rhs.size_ = 0;
      pbuf_ = rhs.pbuf_; rhs.pbuf_ = nullptr;
      vbo_ = rhs.vbo_; rhs.vbo_ = GL_NONE;
      return *this;
    }

    mapped_buffer() : size_(0), pbuf_(nullptr), vbo_(GL_NONE)
    {
    }

    explicit mapped_buffer(std::size_t size) : mapped_buffer()
    {
      glGenBuffers(1, &vbo_);
      const auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
      glNamedBufferStorage(vbo_, GLsizei(size), nullptr, flags);
      pbuf_ = glMapNamedBufferRange(vbo_, 0, GLsizei(size), flags);
      size_ = size;
    }

    ~mapped_buffer()
    {
      glDeleteBuffers(1, &vbo_);
    }

    void* data() { return pbuf_; }
    const void* data() const { return pbuf_; }

  private:
    std::size_t size_;
    void* pbuf_;
    GLuint vbo_;
  };


}



#endif

