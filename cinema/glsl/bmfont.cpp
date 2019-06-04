#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include "bmfont.hpp"
#include <glmutils/bbox.hpp>
#include "stb_image.h"


namespace glsl { namespace bmfont {


#pragma pack(push, 1)
  struct FileKerningPair
  {
    uint32_t first;
    uint32_t second;
    int16_t  amount;
  };

  struct FileChar
  {
    uint32_t id;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    int16_t  xoffset;
    int16_t  yoffset;
    int16_t  xadvance;
    uint8_t  page;
    uint8_t  chnl;
  };
#pragma pack(pop)


  struct CharDescr
  {
    CharDescr() {}
    explicit CharDescr(wchar_t Id): id(Id), xadvance(0) {}
    CharDescr(const struct FileChar& chr, int YOfs)
    :  id((wchar_t)chr.id),
      xadvance(chr.xadvance),
      TexCoord0(chr.x, chr.y),
      TexCoord1(chr.x + chr.width, chr.y + chr.height),
      Vert0(chr.xoffset, chr.yoffset - YOfs),
      Vert1(chr.xoffset + chr.width, chr.yoffset + (chr.height -YOfs))
    {}

    wchar_t id;
    uint16_t xadvance;
    glm::detail::tvec2<uint16_t> TexCoord0, TexCoord1;      // [left-top, right-bottom] [tu, tv]
    glm::detail::tvec2<int16_t> Vert0, Vert1;               // [left-top, right-bottom] [x, y]
  };


  bool operator < (const CharDescr& a, const CharDescr b) 
  { 
    return a.id < b.id; 
  }


  struct KerningPair
  {
    KerningPair() {}
    KerningPair(const struct FileKerningPair& kp)
      : first((wchar_t)kp.first), second((wchar_t)kp.second), amount(kp.amount)
    {
    }

    KerningPair(int First, int Second)
      : first(First), second(Second), amount(0)
    {
    }

    wchar_t first, second;
    int16_t amount;
  };


  bool operator < (const KerningPair& a, const KerningPair& b)
  {
    return (a.first < b.first) ? (a.second < b.second) : false;
  }



  // BMFontLoader
  // Loader of AngleCode's BMFont binary font files.
  // s. www.AngleCode.com
  //
  struct FontLoader
  {
#pragma pack(push, 1)
    struct Info
    {
      char     header[4];
      int8_t   id;
      int32_t  size;
      int16_t  fontSize;
      int8_t   bitField;
      uint8_t  charSet;
      uint16_t stretchH;
      uint8_t  aa;
      uint8_t  paddingUp;
      uint8_t  paddingRight;
      uint8_t  paddingDown;
      uint8_t  paddingLeft;
      uint8_t  spacingHoriz;
      uint8_t  spacingVerts;
      uint8_t  outline;
    };

    struct Common
    {
      int8_t   id;
      int32_t  size;
      uint16_t lineHeight;
      uint16_t base;
      uint16_t scaleW;
      uint16_t scaleH;
      uint16_t pages;
      uint8_t  bitField;
      uint8_t  alphaChnl;
      uint8_t  redChnl;
      uint8_t  greenChnl;
      uint8_t  blueChnl;
    };
#pragma pack(pop)


    FontLoader(const char* FontFileName) 
    : InfoBlk(0), FontName(0), CommonBlk(0), 
      PageName(0), PageNameLen(0), 
      CharBlk(0), KerningBlk(0)
    {
      std::ifstream fs(FontFileName, std::ios_base::binary | std::ios_base::in);
      if (fs.fail())
      {
        throw FontException((std::string("Unable to open font file: ") + FontFileName).c_str());
      }

      // Read file into buffer
      std::streambuf* pbuf = fs.rdbuf();
      std::streamsize size = pbuf->pubseekoff(0, std::ios::end, std::ios::in);
      pbuf->pubseekpos(0, std::ios::in);
      Buffer.reset( new char[size_t(size + 16)] );
      memset((void*)Buffer.get(), '\0', size_t(size + 16));
      pbuf->sgetn(Buffer.get(), size);

      // Sanity check
      if ((size < 5) || (0 != strncmp(Buffer.get(), "BMF" "\3", 4)))
      {
        throw glsl::bmfont::FontException("BMFFontLoader: Invalid font file");
      }

      // Calculate reinterpreted pointer
      InfoBlk = (Info*)(Buffer.get());
      FontName = (char*)(InfoBlk) + sizeof(Info);
      CommonBlk = (Common*)(FontName + strlen(FontName) + 1);
      if (1 == (CommonBlk->bitField & (1 << 7)))
      {
        throw FontException("BMFFontLoader: packed format unsupported");
      }
      if (1 != (CommonBlk->pages))
      {
        throw FontException("BMFFontLoader: multiple pages unsupported");
      }
      PageName = (char*)(CommonBlk) + sizeof(Common) + 5;
      PageNameLen = strlen(PageName);
      char* Block = PageName + CommonBlk->pages * (PageNameLen + 1);
      CharCount = *(int*)(Block + 1) / sizeof(FileChar);
      CharBlk = (FileChar*)(Block + 5);
      Block = (char*)CharBlk + CharCount * sizeof(FileChar);
      KerningCount = *(int*)(Block + 1) / sizeof(FileKerningPair);
      KerningBlk = (FileKerningPair*)(Block + 5);
    }

    Info*     InfoBlk;
    char*     FontName;
    Common*   CommonBlk;
    char*     PageName;
    size_t    PageNameLen;
    int       CharCount;
    FileChar* CharBlk;
    int       KerningCount;
    FileKerningPair* KerningBlk;
    std::unique_ptr<char[]> Buffer;
  };


  class TextBuffer : public TextBufferBase
  {
  public:
    TextBuffer(size_t) {};
    virtual ~TextBuffer() {};

    virtual unsigned int VertexCount() const { return (unsigned int)attribs_.size(); }
    virtual unsigned int PrimCount() const { return (unsigned int)attribs_.size() / 2; }

    virtual void push(const CharDescr& chr, const glm::ivec2& cursor, const glm::detail::tvec4<unsigned char>& color)
    {
      glm::detail::tvec2<int16_t> i16c(cursor);
      attribs_.push_back(TUS2_VS2_CUB4());
      TUS2_VS2_CUB4& a = attribs_.back();
      a.tex = chr.TexCoord0;
      a.vertex = i16c + chr.Vert0;
      a.color = color;
      attribs_.push_back(TUS2_VS2_CUB4());
      TUS2_VS2_CUB4& b = attribs_.back();
      b.tex = glm::vec2(chr.TexCoord1);
      b.vertex = i16c + chr.Vert1;
      b.color = color;
    }

    virtual const void* AttribPointer() const
    {
      return (attribs_.empty()) ? 0 : (void*)&attribs_[0];
    }
    
    virtual void flush()
    {
      attribs_.clear();
    }

    virtual std::shared_ptr<TextBufferBase> clone()
    {
      return std::shared_ptr<TextBufferBase>( new TextBuffer(0) );
    }

  private:
    std::vector<TUS2_VS2_CUB4> attribs_;
  };


  struct FontImpl
  {
    FontImpl::FontImpl(const char* fontFile)
    {
      FontLoader FL(fontFile);
      std::string FontFile(fontFile);
      FontName = FL.FontName;
      FontSize = FL.InfoBlk->fontSize;
      BitField = FL.InfoBlk->bitField;
      LineHeight = FL.CommonBlk->lineHeight;
      Base = (int)FL.CommonBlk->base;
      TexSize = glm::ivec2(FL.CommonBlk->scaleW, FL.CommonBlk->scaleH);  
      int CharCount = FL.CharCount;
      CharMap.reserve(CharCount);
      for (int i=0; i<CharCount; ++i)
      {
        CharMap.push_back( CharDescr(FL.CharBlk[i], Base) );
      }
      
      int KerningCount = FL.KerningCount;
      KerningMap.reserve(KerningCount);
      for (int i=0; i<FL.KerningCount; ++i)
      {
        KerningMap.push_back( KerningPair(FL.KerningBlk[i]) );
      }

      GLuint tex = Texture = GL_NONE;
      glCreateTextures(GL_TEXTURE_2D, 1, &tex);
      glBindTextureUnit(0, tex);
      glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

#ifdef _WIN32
      size_t separator = FontFile.find_last_of("/\\");
#else
      size_t separator = FontFile.find_last_of('/');
#endif
      size_t len = (separator == std::string::npos) ? 0 : (separator + 1);
      std::string FontPath(FontFile.begin(), FontFile.begin() + len);
      std::vector< std::string > Page;
      for (int i=0; i < (int)FL.CommonBlk->pages; ++i)
      {
        std::string TexFile = FontPath + std::string(FL.PageName + (i * (FL.PageNameLen + 1)), FL.PageName + ((i+1) * (FL.PageNameLen + 1)));
        int w,h,c;
        stbi_uc* texData = stbi_load(TexFile.c_str(), &w, &h, &c, STBI_grey);
        if (texData)
        {
          glTextureStorage2D(tex, 1, GL_R8, w, h);
          glTextureSubImage2D(tex, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, (void*)texData);
          stbi_image_free(texData);
        }
        else
        {
          throw FontException((std::string("Unable to decode texture file: ") + TexFile + stbi_failure_reason()).c_str());
        }
      }
      Texture = tex;
      // Set default buffer
      Buffer.reset( new TextBuffer(0) );
    }

    ~FontImpl()
    {
      glDeleteTextures(1, &Texture);
    }

    int xadvance(wchar_t current, wchar_t next) const
    {
      int advance = 0;
      {
        std::vector<CharDescr>::const_iterator it = std::lower_bound(CharMap.begin(), CharMap.end(), CharDescr(current));
        if ((it != CharMap.end()) && (it->id == current)) advance = it->xadvance;
      }
      {
        std::vector<KerningPair>::const_iterator it = std::lower_bound(KerningMap.begin(), KerningMap.end(), KerningPair(current, next));
        if ((it != KerningMap.end()) && (it->first == current) && (it->second == next)) advance += it->amount;
      }
      return advance;
    }

    bool push(wchar_t chr, const glm::ivec2& cursor, const glm::detail::tvec4<unsigned char>& color)
    {
      std::vector<CharDescr>::const_iterator it = std::lower_bound(CharMap.begin(), CharMap.end(), CharDescr(chr));
      if ((it != CharMap.end()) && (it->id == chr)) 
      {
        Buffer->push(*it, cursor, color);
        return true;
      }
      return false;
    }

    GLuint Texture;
    std::string FontName;
    int FontSize;
    int BitField;
    int LineHeight;
    int Base;
    glm::ivec2 TexSize;
    std::vector<CharDescr> CharMap;
    std::vector<KerningPair> KerningMap;
    std::shared_ptr<TextBufferBase> Buffer;
  };



  Font::Font(const char* FontFile)
  {
    try
    {
      pimpl_.reset( new FontImpl(FontFile) );
    }
    catch (FontException&)
    {
      throw;
    }
    catch (...)
    {
      throw FontException("BMFont file corrupted");
    }
  }

  std::shared_ptr<Font> Font::Create(const char* FontFile)
  {
    return std::shared_ptr<Font>( new Font(FontFile) );
  }

  const char* Font::FontName() const { return pimpl_->FontName.c_str(); }
  int Font::FontSize() const { return pimpl_->FontSize; }
  int Font::LineHeight() const { return pimpl_->LineHeight; }
  int Font::Base() const { return pimpl_->Base; }
  bool Font::is_unicode() const { return 1 == (pimpl_->BitField & (1 << 1)); }
  bool Font::is_italic() const { return 1 == (pimpl_->BitField & (1 << 2)); }
  bool Font::is_bold() const { return 1 == (pimpl_->BitField & (1 << 3)); }

  void Font::SetBuffer(std::shared_ptr<TextBufferBase> buf)
  {
    pimpl_->Buffer = buf;
  }

  std::shared_ptr<TextBufferBase> Font::GetBuffer()
  {
    return pimpl_->Buffer;
  }

  void Font::SwapBuffer(std::shared_ptr<TextBufferBase>& buf)
  {
    pimpl_->Buffer.swap(buf);
  }

  void Font::BindTexture(unsigned int Unit)
  {
    glBindTextureUnit(Unit, pimpl_->Texture);
  }

  glm::ivec2 Font::TextureSize() const 
  {
    return pimpl_->TexSize;
  }


  namespace {

    template <typename CHAR>
    glm::ivec2 Text2DExtent(const FontImpl& fimpl, const CHAR* first, const CHAR* last, 
                            int tabSize, int lineHeight,
                            glm::ivec2& cursor, const glm::ivec2& orig)
    {
      glmutils::tbbox< glm::ivec2 > box(cursor);
      for (; first != last; ++first)
      {
        if (*first == '\n') 
        { 
          cursor.y += lineHeight; 
          glmutils::include(box, cursor);
          cursor.x = orig.x;
          glmutils::include(box, cursor);
        }
        else if (*first == '\t')
        {
          int tabs = cursor.x / tabSize;
          cursor.x = (tabs + 1) * tabSize;
          glmutils::include(box, cursor);
        }
        else
        {
          glmutils::include(box, cursor);
          cursor.x += fimpl.xadvance((wchar_t)*first, (wchar_t)*(first + 1));
          glmutils::include(box, glm::ivec2(cursor.x, cursor.y + lineHeight));
        }
      }
      return glmutils::extent(box);
    }

    template <typename CHAR>
    void Text2DStream(FontImpl& fimpl, 
                      const CHAR* first,  const CHAR* last,
                      int tabSize, int lineHeight,
                      const glm::detail::tvec4<unsigned char>& color, 
                      glm::ivec2& cursor, const glm::ivec2& orig)
    {
      for (; first != last; ++first)
      {
        if (*first == '\n') 
        { 
          cursor.x = orig.x; cursor.y += lineHeight; 
        }
        else if (*first == '\t')
        {
          int tabs = cursor.x / tabSize;
          cursor.x = (tabs + 1) * tabSize;
        }
        else
        {
          if (fimpl.push((wchar_t)*first, cursor, color))
          {
            cursor.x += fimpl.xadvance((wchar_t)*first, (wchar_t)*(first + 1));
          }
        }
      }
    }

  }


  Text2D::Text2D(std::shared_ptr<Font> font, const glm::ivec2& orig)
  : cursor_(orig), orig_(orig), 
    tabSize_(100), lineHeight_(font->LineHeight()), 
    color_(255, 255, 255, 255), font_(font)
  {
  }

  void Text2D::SetFont(std::shared_ptr<Font> font)
  { 
    font_ = font; 
    lineHeight_ = font->LineHeight();
  }

  const Font& Text2D::GetFont() const
  {
    return *font_;
  }

  Font& Text2D::GetFont()
  {
    return *font_;
  }

  std::shared_ptr<TextBufferBase> Text2D::GetBuffer()
  {
    return font_->GetBuffer();
  }

  glm::ivec2 Text2D::Extent(const char* first, const char* last, glm::ivec2& vCursor) const
  {
    return Text2DExtent<char>(*(font_->pimpl_), first, last, tabSize_, lineHeight_, vCursor, orig_);
  }

  glm::ivec2 Text2D::Extent(const wchar_t* first, const wchar_t* last, glm::ivec2& vCursor) const 
  {
    return Text2DExtent<wchar_t>(*(font_->pimpl_), first, last, tabSize_, lineHeight_, vCursor, orig_);
  }

  glm::ivec2 Text2D::Extent(const char* str, glm::ivec2& vCursor) const
  {
    return Extent(str, str + strlen(str), vCursor);
  }

  glm::ivec2 Text2D::Extent(const wchar_t* str, glm::ivec2& vCursor) const
  {
    return Extent(str, str + wcslen(str), vCursor);
  }

  void Text2D::Stream(const char* first, const char* last)
  {
    return Text2DStream<char>(*(font_->pimpl_), first, last, tabSize_, lineHeight_, color_, cursor_, orig_);
  }

  void Text2D::Stream(const wchar_t* first, const wchar_t* last)
  {
    return Text2DStream<wchar_t>(*(font_->pimpl_), first, last, tabSize_, lineHeight_, color_, cursor_, orig_);
  }

  Text2D& Text2D::operator << (const char* str)
  {
    Stream(str, str + strlen(str));
    return *this;
  }

  Text2D& Text2D::operator << (const wchar_t* str)
  {
    Stream(str, str + wcslen(str));
    return *this;
  }

  Text2D& Text2D::operator << (const cursor_manip& x) 
  { 
    cursor_ = orig_ + x.val;
    if (!x.baseline) cursor_.y -= (lineHeight_ - font_->Base());
    return *this; 
  }

  Text2D& Text2D::operator << (const orig_manip& x) 
  { 
    orig_ = cursor_ = x.val;
    return *this; 
  }

} }

