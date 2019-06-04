#ifndef CINE2_CONVOLUTION_H_INCLUDED
#define CINE2_CONVOLUTION_H_INCLUDED

#include <array>
#include <cmath>
#include <numeric>


namespace cine2 {


  template <int KernelSize>
  struct ConvolutionKernel
  {
    static_assert(KernelSize & 1, "Kernel size shall be odd");
    static const int k = KernelSize;
    
    static void normalize(std::array<float, KernelSize * KernelSize>& K)
    {
      auto sum = std::accumulate(K.cbegin(), K.cend(), 0.0f);
      for (auto& x : K) x /= sum;
    }
  };


  template <int KernelSize>
  struct BoxFilter : ConvolutionKernel<KernelSize>
  {
    BoxFilter()
    {
      K.assign(1.0);
      ConvolutionKernel<KernelSize>::normalize(K);
    }

    float maxK() const { return K[KernelSize / 2]; }
    std::array<float, KernelSize * KernelSize> K;
  };


  template <int KernelSize>
  struct GaussFilter : ConvolutionKernel<KernelSize>
  {
    GaussFilter()
    {
      const int radius = KernelSize >> 1;
      for (int i=0; i <= radius; ++i) {
        K[i] = std::exp(-0.5f * (i * i));
      }
      int s = 0;
      for (int x=-radius; x <= radius; ++x) {
        for (int y=-radius; y <= radius; ++y) {
          K[s++] = std::exp(-0.5f * ((x * x) + (y * y)));
        }
      }
      ConvolutionKernel<KernelSize>::normalize(K);
    }

    float maxK() const { return K[KernelSize / 2]; }
    std::array<float, KernelSize * KernelSize> K;
  };


}

#endif
