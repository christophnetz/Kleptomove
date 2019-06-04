#ifndef CINE2_ANY_ANN_HPP_INCLUDED
#define CINE2_ANY_ANN_HPP_INCLUDED

#include <memory>
#include <algorithm>
#include <functional>
#include <vector>
#include "parameter.h"


namespace cine2 {


  // type erased wrapper for Anns
  class any_ann
  {
  public:
    any_ann(any_ann&&) noexcept = delete;
    any_ann& operator=(any_ann&&) noexcept = delete;
    any_ann(const any_ann&) = delete;
    any_ann& operator=(const any_ann&) = delete;

    any_ann(int N, int state_size, int size);
    virtual ~any_ann();

    int N() const { return N_; }

    // number of state floats
    int state_size() const { return state_size_; }

    // number of floats per ann: stride() >= state_size() due to alignment
    int stride() const { return size_ / sizeof(float); }

    int type_size() const { return size_; }

    // returns pointer to first weight of ANN idx
    float* operator[](int idx) { return (float*)((char*)state_ + idx * size_); }

    // returns pointer to first const weight of ANN idx
    const float* operator[](int idx) const { return (const float*)((const char*)state_ + idx * size_); }

    // assign single ann
    void assign(const any_ann& src, int src_idx, int dst_idx)
    {
      std::memcpy(this->operator[](dst_idx), src[src_idx], size_);
    }

    float* data() { return state_; };
    const float* data() const { return state_; }

    // Returns complexity of ann idx: 1 - (zero / weights)
    virtual float complexity(int idx) const = 0;
    virtual void move(const Landscape& landscape, std::vector<Individual>& pop, const Param::ind_param& iparam) = 0;
    virtual void mutate(const Param::ind_param& iparam, bool fixed) = 0;

  protected:
    int N_;
    int state_size_;
    int size_;
    float* state_;
  };


  // creates an any_ann from runtime parameters.
  std::unique_ptr<any_ann> make_any_ann(int L, int N, const char* ann_descr);

}


#endif
