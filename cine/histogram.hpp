//! \file histogram.hpp Generic histogram logger.
//! \ingroup Analysis

#ifndef HISTOGRAM_HPP_INCLUDED
#define HISTOGRAM_HPP_INCLUDED

#include <vector>
#include <numeric>
#include <algorithm>
#include <glm/glm.hpp>


class histogram;
class histogram2D;


class histogram
{
  typedef std::vector<float>          counts_vect;
  typedef counts_vect::const_iterator const_iterator;
  typedef counts_vect::iterator       iterator;

public:
  typedef std::vector<glm::vec2> cdf_vect;
  typedef glm::vec3              quartiles_t;

  histogram();
  histogram(float x_min, float x_max, int bins);

  void reset();
  void reset(float x_min, float x_max, int bins);

  void reduce(float factor);
  void append(const histogram& hist);

  template<typename U>
  void operator()(U value);

  template<typename U>
  void operator()(U value, int times);

  float count() const { return samples_; }
  float max_count() const;
  unsigned num_bins() const { return static_cast<unsigned>(counts_.size()); }
  glm::vec2 operator[](size_t i) const;          //!< \return vec2(bin_center, count)
  float quantile(float Q) const;
  float quantile(float Q, const cdf_vect& cdf) const;
  quartiles_t quartiles() const;
  quartiles_t quartiles(const cdf_vect& cdf) const;
  void CDF(cdf_vect& cdf) const;
  const counts_vect bins() const { return counts_; }

private:
  float bin_lo(int i) const { return i / binsScale_ + x_min(); }
  float x_min() const { return -binsOffs_/binsScale_; }
  float x_max() const { return x_min() + binsScale_*(counts_.size()-1); }

  float binsScale_;
  float binsOffs_;
  float samples_;
  counts_vect counts_;
};


class histogram2D
{
  typedef std::vector<histogram>      hists_vect;
  typedef hists_vect::const_iterator  const_iterator;
  typedef hists_vect::iterator        iterator;

public:
  histogram2D();
  histogram2D(float x_min, float x_max, int x_bins,
              float y_min, float y_max, int y_bins);
  void reset();
  void reset(float x_min, float x_max, int x_bins,
             float y_min, float y_max, int y_bins);

  template<typename U>
  void operator()(U x_value, U y_value);

  template<typename U>
  void operator()(U x_value, U y_value, int times);

  size_t size() const { return hists_.size(); }
  size_t count() const { return count_; }
  const histogram& operator[](size_t i) const { assert(i < hists_.size()); return hists_[i]; }
  histogram& operator[](size_t i) { assert(i < hists_.size()); return hists_[i]; }

private:
  float binsScale_;
  float binsOffs_;
  size_t count_;
  hists_vect hists_;
};


////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////

namespace detail {

  inline float linp(float b0, float b1, float ql, float qh, float q)
  {
    float dq = qh - ql;
    float x = (std::abs(dq) > 10e-20f) ? (q - ql) / dq : 0.5f;
    float y = glm::mix(b0, b1, x);
    return y;
  }
}


inline histogram::histogram()
{
  reset(0, 1, 0);
}


inline histogram::histogram(float x_min, float x_max, int bins)
{
  reset(x_min, x_max, bins);
}


inline void histogram::reset()
{
  samples_ = 0;
  counts_.assign(num_bins(), 0);
}


inline void histogram::reset(float x_min, float x_max, int bins)
{
  binsScale_ = static_cast<float>(bins) / float(x_max - x_min);
  binsOffs_ = - binsScale_ * float(x_min);
  counts_.resize(bins);
  reset();
}


inline void histogram::reduce(float factor)
{
  for (auto& x : counts_) x *= factor;
  samples_ *= factor;
}


inline void histogram::append(const histogram& h)
{
  bool compatible = (num_bins() == h.num_bins()) && (binsScale_ == h.binsScale_) && (binsOffs_ == h.binsOffs_);
  if (!compatible)
  {
    throw std::exception("histogram::append called with incompatible argument.");
  }
  counts_vect::const_iterator arg = h.counts_.begin();
  for (auto& x : counts_) x += *arg;
  samples_ += h.samples_;
}


inline float histogram::max_count() const 
{ 
  const_iterator it = std::max_element(counts_.begin(), counts_.end()); 
  return (it != counts_.end()) ? *it : 0;
}


inline float histogram::quantile(float q) const
{
  cdf_vect cdf;
  CDF(cdf);
  return quantile(q, cdf);
}


inline float histogram::quantile(float Q, const cdf_vect& cdf) const
{
  const size_t bins = cdf.size()-1; 
  if (0 == bins) return std::numeric_limits<float>::quiet_NaN();
  size_t i = 0;
  while ((i<bins) && (cdf[i+1].y < Q)) { ++i; }
  return detail::linp(cdf[i].x, cdf[i+1].x, cdf[i].y, cdf[i+1].y, Q);
}


inline histogram::quartiles_t histogram::quartiles() const
{
  cdf_vect cdf;
  CDF(cdf);
  return quartiles(cdf);
}


inline histogram::quartiles_t histogram::quartiles(const cdf_vect& cdf) const
{
  return quartiles_t(quantile(0.25, cdf), quantile(0.50, cdf), quantile(0.75, cdf));
}


inline void histogram::CDF(cdf_vect& cdf) const
{
  cdf.clear();
  float scale = 1.0f / samples_;
  const unsigned n = num_bins();
  float cs = 0;
  unsigned i = 0;
  for (; (i<n) && (counts_[i] < 10e-10f); ++i);
  cdf.emplace_back( bin_lo(i-1), 0.0f );
  for (; i<n; ++i)
  {
    if (counts_[i] > 10e-100)
    {
      cdf.emplace_back( bin_lo(i+1), scale * (cs += counts_[i]) );
    }
  }
}


template<typename U>
inline void histogram::operator()(U value)
{
  float dval(value);
//  if (! glmutils::any_nan(dval))
  {
    ++samples_;
    int bin = static_cast<int>( binsScale_ * dval + binsOffs_ );
    // clamp bin to [0,maxBin]
    ++counts_[std::max(std::min(bin, static_cast<int>(num_bins())-1), 0)];
  }
}


template<typename U>
inline void histogram::operator()(U value, int times)
{
  float dval(value);
//  if (! glmutils::any_nan(dval))
  {
    samples_ += times;
    int bin = static_cast<int>( binsScale_ * dval + binsOffs_ );
    // clamp bin to [0,maxBin]
    counts_[std::max(std::min(bin, static_cast<int>(num_bins())-1), 0)] += times;
  }
}


inline glm::vec2 histogram::operator[](size_t i) const 
{ 
  assert(i < counts_.size()); 
  return glm::vec2(bin_lo(static_cast<int>(i)), counts_[i]); 
}


template<typename U>
inline void histogram2D::operator()(U x_value, U y_value)
{
  float dxval(x_value);
//  if (! glmutils::any_nan(dxval))
  {
    ++count_;
    int bin = static_cast<int>( binsScale_ * float(dxval) + binsOffs_ );
    // clamp bin to [0,maxBin]
    hists_[std::max(std::min(bin, static_cast<int>(size())-1), 0)](y_value);
  }
}

template<typename U>
inline void histogram2D::operator()(U x_value, U y_value, int times)
{
  float dxval(x_value);
//  if (! glmutils::any_nan(dxval))
  {
    count_ += times;
    int bin = static_cast<int>( binsScale_ * float(dxval) + binsOffs_ );
    // clamp bin to [0,maxBin]
    hists_[std::max(std::min(bin, static_cast<int>(size())-1), 0)](y_value, times);
  }
}


inline histogram2D::histogram2D() 
{
}


inline histogram2D::histogram2D(float x_min, float x_max, int x_bins,
                                float y_min, float y_max, int y_bins)
{
  reset(x_min, x_max, x_bins, y_min, y_max, y_bins);
}


inline void histogram2D::reset()
{
  count_ = 0;
  for (size_t i=0; i<hists_.size(); ++i)
  {
    hists_[i].reset();
  }
}


inline void histogram2D::reset(float x_min, float x_max, int x_bins,
                               float y_min, float y_max, int y_bins)
{
  binsScale_ = static_cast<float>(x_bins) / float(x_max - x_min);
  binsOffs_ = - binsScale_ * float(x_min);
  count_ = 0;
  hists_.clear();
  for (int i=0; i<x_bins; ++i)
  {
    hists_.emplace_back( y_min, y_max, y_bins );
  }
}


#endif
