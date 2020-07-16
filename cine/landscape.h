#ifndef CINE2_LANDSCAPE_H_INCLUDED
#define CINE2_LANDSCAPE_H_INCLUDED

#include <cassert>
#include <cstring>      // memset
#include <stdexcept>
#include <xmmintrin.h>
#include "convolution.h"
#include "ann.hpp"      // ann_assume_aligned
#include "histogram.hpp"


namespace cine2 {


#pragma pack(push, 2)
  union Coordinate
  {
    Coordinate() {}
    Coordinate(short X, short Y) : x(X), y(Y) {}
    struct { short x; short y; };
    unsigned packed;
  };
#pragma pack(pop)


  inline bool operator==(Coordinate a, Coordinate b) { return a.packed == b.packed; }
  inline bool operator!=(Coordinate a, Coordinate b) { return a.packed != b.packed; }
  inline Coordinate operator+(Coordinate a, Coordinate b) { return Coordinate(a.x + b.x, a.y + b.y); }
  inline Coordinate operator+=(Coordinate& a, Coordinate b) { a.x += b.x; a.y += b.y; return a; }


  /// \brief  A View into an landscape layer.
  class LayerView
  {
  public:
    LayerView(float* data, int dim)
      : dim_(dim), data_(data)
    {
      assert((dim & (dim - 1)) == 0);
    }

    int dim() const { return dim_; }
    int size() const { return dim_ * dim_; }
    int mem_size() const { return dim_ * dim_ * sizeof(float); }

    void clear() { std::memset(data_, 0, mem_size()); }

    float operator()(Coordinate coor) const 
    { 
      ann_assume_aligned(data_, 32);
      const int mask = dim_ - 1;
      return data_[dim_ * (coor.y & mask) + (coor.x & mask)]; 
    }
  
    float& operator()(Coordinate coor)
    { 
      ann_assume_aligned(data_, 32);
      const int mask = dim_ - 1;
      return data_[dim_ * (coor.y & mask) + (coor.x & mask)]; 
    }


    /// \brief  Gathers the cells in a square around center.
    ///
    /// \tparam L   The side length of the square
    /// \param  center  The center.
    template <int L>
    std::array<float, L*L> gather(Coordinate center) const
    {
      std::array<float, L*L> res;
      for (int i = 0; i < L*L; ++i) {
        res[i] = this->operator()(center + Coordinate((i % L) - L/2, (i / L) - L/2));
      }
      return res;
    }

    /// \brief stamps the kernel around center
    ///
    /// \tparam L   The side length of the square
    /// \param  center  The center.
    template <int L>
    void stamp_kernel(Coordinate center, const std::array<float, L*L>& kernel)
    {
      for (int i = 0; i < L*L; ++i) {
        this->operator()(center + Coordinate((i % L) - L/2, (i / L) - L/2)) += kernel[i];
      }
    }

    const float* cbegin() const { return data_; }
    const float* cend() const { return data_ + size(); }
    float* begin() const { return data_; }
    float* end() const { return data_ + size(); }
    const float* data() const { return data_; }
    float* data() { return data_; }

    void copy(const LayerView& src) {
      assert(dim_ == src.dim_);
      std::memcpy(data_, src.data_, mem_size());
    }

  private:
    int dim_;
    float* data_;
  };


  /// \brief  Our landscape
  ///        
  /// A Landscape represents a quadradic (POT) area composed of
  /// several layers.
  class Landscape
  {
  public:
    /// \brief  Values that represent layers in a Landscape.
    enum Layers : int {
      foragers = 0,     //convoluted foragers_count
      klepts,			//convoluted klepts_count
      handlers,			//handlers
      items,			//food items
      capacity,			//maximum capacity of the landscape
      foragers_count,
      klepts_count,
      handlers_count,
      nonhandlers,
      items_rec, 
      foragers_rec,
      klepts_rec,
      foragers_intake,
      klepts_intake,
      temp,         // scratch for computation
      max_layer
    };

    Landscape() : dim_(0), data_(nullptr)
    {
    }

    Landscape(Landscape&& rhs) : Landscape()
    {
      *this = std::move(rhs);
    }

    Landscape& operator=(Landscape&& rhs) noexcept
    {
      dim_ = rhs.dim_; rhs.dim_ = 0;
      data_ = rhs.data_; rhs.data_ = nullptr;
      return *this;
    }

    /// \brief  Creates a landscape.
    ///
    /// \exception  std::runtime_error  Raised when a the dimension is not POT.
    /// \exception  std::bad_alloc      Thrown when a bad Allocate error condition occurs.
    ///
    /// \param  dim The dimension of the landscape.
    explicit Landscape(int dim) : Landscape()
    {
      if ((dim & (dim - 1)) != 0) {
        throw std::runtime_error("Landscape dimension shall be POT");
      }
      data_ = (float*)_mm_malloc(Layers::max_layer * dim * dim * sizeof(float), 64);
      if (data_ == nullptr) throw std::bad_alloc();
      dim_ = dim;
      std::memset(data_, 0, mem_size());
    }

    Landscape(const Landscape& rhs) : Landscape(rhs.dim_)
    {
      std::memcpy(data_, rhs.data_, mem_size());
    }
    
    Landscape& operator=(const Landscape& rhs)
    {
      Landscape tmp(rhs);
      *this = std::move(tmp);
      return *this;
    }
  
    /// \brief  Destructor.
    ~Landscape()
    {
      _mm_free(data_);
    }

    /// \return the dimension of the landscape.
    int dim() const { return dim_; }

    /// \return the total size of all layers in memory [bytes].
    int mem_size() const { return Layers::max_layer * dim_ * dim_ * sizeof(float); }

    /// \return the size of a layers in memory [bytes].
    int layer_mem_size() const { return dim_ * dim_ * sizeof(float); }

    Coordinate wrap(Coordinate coor) const
    {
      const unsigned mask = dim_ - 1;
      coor.packed &= (mask << 16) | mask;
      return coor;
    }

    /// \return LayerView of the indexed value.
    LayerView get_layer(Layers layer) { return LayerView(data_ + layer * dim_ * dim_, dim_); }
  
  
    /// \return LayerView of the indexed value.
    const LayerView get_layer(Layers layer) const { return LayerView(data_ + layer * dim_ * dim_, dim_); }


    /// \param  layer The layer.
    ///
    /// \return LayerView of the indexed value.
    LayerView operator[](Layers layer) { return get_layer(layer); }

    /// \param  layer The layer.
    ///
    /// \return LayerView of the indexed value.
    const LayerView operator[](Layers layer) const { return get_layer(layer); }

    template <typename IT, typename Kernel>
    void update_occupancy(Layers count, Layers conv, Layers count2, Layers conv2, Layers count3, Layers conv3, Layers combined, IT first, IT last, const Kernel& kernel)
    {
      LayerView vCount = get_layer(count);
      LayerView vConv = get_layer(conv);      
      LayerView vCount2 = get_layer(count2);
      LayerView vConv2 = get_layer(conv2);
      LayerView vCount3 = get_layer(count3);
      LayerView vConv3 = get_layer(conv3);
      LayerView vComb = get_layer(combined);
	  
	  //clearing the vectors before the visualization of the current timestep
      vCount.clear();
      vConv.clear();      
      vCount2.clear();
      vConv2.clear();
      vCount3.clear();
      vConv3.clear();
      vComb.clear();

      for (; first != last; ++first) {		//cycle trough the agents
        if (first->alive()) {				//if alive
          if (first->handle()) {				//and handling
            ++vCount3(first->pos);					//position stored in the vector3 (for handlers apparently)
            vConv3.stamp_kernel<Kernel::k>(first->pos, kernel.K);
          }
          else if (first->foraging) {			//if not handling, but foraging
            ++vCount(first->pos);					//position stored in vector1 (for foragers)
            vConv.stamp_kernel<Kernel::k>(first->pos, kernel.K);
            vComb.stamp_kernel<Kernel::k>(first->pos, kernel.K);
          }
          else {								//if not handling and not foragers (they are kleptoparasytes)
            ++vCount2(first->pos);					//position stored in vector2 (for klepts)
            vConv2.stamp_kernel<Kernel::k>(first->pos, kernel.K);
            vComb.stamp_kernel<Kernel::k>(first->pos, kernel.K);

          }

        }
      }
    }



    const float* data() const { return data_; }

  private:
    int dim_;
    float* data_;
  };

}

#endif

