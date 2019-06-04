// Simple image ' library'
// Thin layer above stbi_read & stbi_write
//
// Hanno 2016


#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED

#include <cassert>
#include <exception>
#include <string>
#include <memory>
#include <type_traits>
#include "landscape.h"


namespace cine2 {


  /// \brief  Values that represent channels.
  enum ImageChannel : int
  {
    red = 0,
    green,
    blue,
    alha
  };


  /// \brief  An RGBA image.
  class Image
  {
  public:
    explicit Image(const std::string& fileName);
    int width() const { return width_; }
    int height() const { return height_; }
    unsigned* data() { return data_.get(); }
    const unsigned* data() const { return data_.get(); }
  
  private:
    int width_;
    int height_;
    std::unique_ptr<unsigned, decltype(std::free)*> data_;
  };


  /// \brief  Assign image channel to LayerView.
  ///
  /// \param [in,out] dst     Destination LayerView.
  /// \param          src     Source Image.
  /// \param          channel Source channel.
  ///
  /// The destination values are in [0,1]
  void image_channel_to_layer(LayerView dst, const Image& src, ImageChannel channel);


  /// \brief  Assign layer to image channel.
  ///
  /// \param [in,out] dst     Destination Image.
  /// \param          src     Source LayerView.
  /// \param          channel Destination channel.
  ///
  /// The values in src are clamped to [0,1], multiplied by 255
  /// and coerced to unsigned char.
  void layer_to_image_channel(Image& dst, const LayerView& src, ImageChannel channel);


  /// \brief  Saves an image as png.
  ///
  /// \param  image     The image.
  /// \param  fileName  Filename of the file.
  void save_image(const Image& image, const std::string& fileName);
  
}


#endif
