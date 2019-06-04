#pragma warning( disable : 4996 )   // suppress superflous MSVC  warning(s)

#include <stdexcept>
#include <memory>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include "image.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "glsl/stb_image.h"
#include "glsl/stb_image_write.h" 



namespace cine2 {


  Image::Image(const std::string& fileName)
    : data_(nullptr, std::free)
  {
    int channels;
    width_ = height_ = channels = 0;
    data_.reset((unsigned*)stbi_load(fileName.c_str(), &width_, &height_, &channels, STBI_rgb_alpha));
    if (!data_)
    {
      throw std::runtime_error((std::string("Can't read Image from ") + fileName).c_str());
    }
  }


  void image_channel_to_layer(LayerView dst, const Image& src, ImageChannel channel)
  {
    if (!(src.width() == dst.dim() && src.height() == dst.dim())) {
      throw std::runtime_error("assign_image_channel_to_layer: dimensions don't match");
    }
    auto select_channel = [shift = 8 * channel](unsigned rgba) { 
      return (rgba & (0xff << shift)) >> shift; 
    };
    const int n = dst.size();
    float* pdst = dst.data();
    const unsigned* psrc = src.data();
    for (int i=0; i<n; ++i, ++pdst, ++psrc) {
      *pdst = static_cast<float>(select_channel(*psrc)) / 255.0f;
    }
  }


  void layer_to_image_channel(Image& dst, const LayerView& src, ImageChannel channel)
  {
    if (!(dst.width() == src.dim() && dst.height() == src.dim())) {
      throw std::runtime_error("assign_layer_to_image_channel: dimensions don't match");
    }
    auto set_channel = [=](unsigned char& c, float val) { 
      c = static_cast<unsigned char>(std::max(0.0f, std::min(val, 1.0f)) * 255.0f);
    };
    const int n = dst.width() * dst.height();
    unsigned char* pdst = (unsigned char*)(dst.data()) + channel;
    const float* psrc = src.data();
    for (int i=0; i<n; ++i, pdst += 4, ++psrc) {
      set_channel(*pdst, *psrc);
    }
  }


  void save_image(const Image& image, const std::string& filename)
  {
    int stride = image.width() * sizeof(unsigned);
    if (0 == stbi_write_png(filename.c_str(), image.width(), image.height(), 4, image.data(), stride)) { 
      throw std::runtime_error((std::string("Can't write Image to ") + filename).c_str());
    }
  }

}

