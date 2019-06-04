#ifndef BMFONT_HPP_INCLUDED
#define BMFONT_HPP_INCLUDED


#include <exception>
#include <memory>
#include <cstdint>
#include <glm/glm.hpp>
#include "glsl.hpp"


namespace glsl { namespace bmfont {


  ///Exceptions thrown by the font classes.
  class FontException : public std::exception
  {
  public:
    explicit FontException(const char* msg) : std::exception(msg) {}
  };


#pragma pack(push, 4)
  struct TUS2_VS2_CUB4
  {
    TUS2_VS2_CUB4() {}
    glm::detail::tvec2<uint16_t>  tex;
    glm::detail::tvec2<int16_t>   vertex;
    glm::detail::tvec4<uint8_t>   color;
  };
#pragma pack(pop)


  class TextBufferBase
  {
  protected:
    TextBufferBase() {}

  public:
    explicit TextBufferBase(size_t bufferSize);
    virtual ~TextBufferBase() {};

    virtual unsigned int VertexCount() const = 0;
    virtual unsigned int PrimCount() const = 0;

    virtual void push(const struct CharDescr& chr, const glm::ivec2& cursor, const glm::detail::tvec4<unsigned char>& color) = 0;
    virtual const void* AttribPointer() const = 0;
    virtual void flush() = 0;
    virtual std::shared_ptr<TextBufferBase> clone() = 0;
  };


  class Font
  {
    Font(const char*);

  public:
    static std::shared_ptr<Font> 
    Create(const char* FontFile);

    const char* FontName() const;
    int FontSize() const;
    int LineHeight() const;
    int Base() const;
    bool is_unicode() const;
    bool is_italic() const;
    bool is_bold() const;

    void SetBuffer(std::shared_ptr<TextBufferBase> buf);
    std::shared_ptr<TextBufferBase> GetBuffer();
    void SwapBuffer(std::shared_ptr<TextBufferBase>& buf);

    void BindTexture(unsigned int Unit);
    glm::ivec2 TextureSize() const;

  private:
    friend class Text2D;
    std::shared_ptr< struct FontImpl > pimpl_;
  };


  // Manipulators
  struct color_manip
  {
    explicit color_manip(const glm::vec4 color) : val(255 * color) {}
    explicit color_manip(const glm::detail::tvec4<unsigned char>& color) : val(color) {}
    color_manip(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) : val(r,g,b,a) {}
    color_manip(float r, float g, float b, float a = 1.0f) : val(255*r, 255*g, 255*b, 255*a) {}
    glm::detail::tvec4<unsigned char> val;
  };

  struct tab_manip
  {
    explicit tab_manip(int tabSize) : val(tabSize) {}
    int val;
  };

  struct line_height_manip
  {
    explicit line_height_manip(int lineHeight) : val(lineHeight) {}
    int val;
  };

  struct cursor_manip
  {
    explicit cursor_manip(const glm::ivec2& cursor, bool baseLine = true) : val(cursor), baseline(baseLine) {}
    cursor_manip(int x, int y, bool baseLine = true) : val(x,y), baseline(baseLine) {}
    glm::ivec2 val;
    bool baseline;
  };

  struct orig_manip
  {
    explicit orig_manip(const glm::ivec2& orig) : val(orig) {}
    orig_manip(int x, int y) : val(x,y) {}
    glm::ivec2 val;
  };


  class Text2D
  {
  public:
    Text2D(std::shared_ptr<Font> font, const glm::ivec2& orig = glm::ivec2(0,0));

    const Font& GetFont() const;
    Font& GetFont();
    void SetFont(std::shared_ptr<Font> font);
    std::shared_ptr<TextBufferBase> GetBuffer();

    const char* FontName() const { return font_->FontName(); }
    int FontSize() const { return font_->FontSize(); }
    int LineHeight() const { return font_->LineHeight(); }
    int Base() const { return font_->Base(); }
    bool is_unicode() const { return font_->is_unicode(); }
    bool is_italic() const { return font_->is_italic(); }
    bool is_bold() const { return font_->is_bold(); }

    glm::ivec2 GetCursor() const { return cursor_; }
    glm::ivec2 GetOrig() const { return orig_; }

    glm::ivec2 Extent(const char* first, const char* last, glm::ivec2& vCursor) const;
    glm::ivec2 Extent(const wchar_t* first, const wchar_t* last, glm::ivec2& vCursor) const;
    glm::ivec2 Extent(const char* str, glm::ivec2& vCursor) const;
    glm::ivec2 Extent(const wchar_t* str, glm::ivec2& vCursor) const;

    void Stream(const char* first, const char* last);
    void Stream(const wchar_t* first, const wchar_t* last);
    Text2D& operator << (const char* str);
    Text2D& operator << (const wchar_t* str);

    // Manipulators
    Text2D& operator << (const color_manip& x) {  color_ = x.val; return *this; }
    Text2D& operator << (const tab_manip& x) { tabSize_ = x.val; return *this; }
    Text2D& operator << (const line_height_manip& x) { lineHeight_ = x.val; return *this; }
    Text2D& operator << (const cursor_manip& x);
    Text2D& operator << (const orig_manip& x);

  private:
    glm::ivec2 cursor_;
    glm::ivec2 orig_;
    int tabSize_;
    int lineHeight_;
    glm::detail::tvec4<unsigned char> color_;
    std::shared_ptr<Font> font_;
  };


} }

#endif
