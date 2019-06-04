//
// GLSL support library
// Hanno Hildenbrandt 2008
//

//! \file framebuffer.hpp Thin Wrapper for OpenGL Framebuffer Objects.

#ifndef GLSL_FRAMEBUFFER_HPP_INCLUDED
#define GLSL_FRAMEBUFFER_HPP_INCLUDED

#include "glsl.hpp"
#include "proxy.hpp"


namespace glsl  {

  //! Thin Wrapper for OpenGL Framebuffer Objects (fbo).
  //!
  //! Supports:
  //! \li \c ARB_framebuffer_object
  //! \li \c ARB_texture_rectangle
  //! \li \c ARB_geometry_shader4
  class framebuffer : proxy<framebuffer>
  {
  public:
    //! Alias to BlitFramebuffer
    static void blit(int srcX0, int srcY0, int srcX1, int srcY1,
                     int dstX0, int dstY0, int dstX1, int dstY1,
                     GLbitfield mask, GLenum filter);

  public:
    //! Constructs 0-framebuffer.
    framebuffer();

    //! Takes ownership over existing OpenGL \c framebuffer object.
    explicit framebuffer(GLuint fbo);

    //! Deletes fbo
    ~framebuffer();

    bool isValid() const { return isValid_(); }
    operator GLuint () const { return get_(); }    //!< Cast to underlying OpenGL object.
    GLuint get() const { return get_(); }      //!< Returns the underlying OpenGL object.

    //! Returns the target the fbo is currently bounded to.
    GLenum target() const { return target_; }

    GLenum status() const;      //!< Returns the fbo status.

    //! Binds the fbo
    void bind(GLenum target = GL_FRAMEBUFFER);

    //! Unbinds the fbo
    void unbind();

    //! Attaches a \ref renderbuffer Object
    void attachRenderbuffer(GLenum attachment, GLuint rbo) const;

    //! Attaches a 1D Texture
    void attachTexture1D(GLenum attachment, GLuint texture, GLint level = 0) const;

    //! Attaches a 2D Texture
    void attachTexture2D(GLenum attachment, GLuint texture, GLint level = 0) const;

    //! Attaches a 3D Texture
    void attachTexture3D(GLenum attachment, GLuint texture, GLint level, GLint zoffset) const;

    //! Attaches a Texture layer
    void attachTextureLayer(GLenum attachment, GLuint texture, GLint level, GLint layer) const;

    //! Attaches a Texture Rectangle, see \c ARB_texture_rectangle
    void attachTextureRectangle(GLenum attachment, GLuint texture, GLint level = 0) const;

    //! Swap
    void swap(framebuffer& other);

  private:
    GLenum  target_;

  private:
    friend class proxy<framebuffer>;
    static void release(GLuint x) { glDeleteFramebuffers(1, &x); }
  };


}  // namespace glsl


#endif



