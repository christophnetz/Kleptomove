#include "analysis.h"
#include "ann.hpp"
#include "simulation.h"


namespace cine2 {


  namespace {

    struct ann_cmp
    {
      ann_cmp(int weights) : size_(weights * sizeof(float)) {}

      bool operator()(const float* a, const float* b) const
      {
        return std::memcmp(a, b, size_) < 0;
      }

      int size_;
    };

  }


  void Analysis::generation(const Simulation * sim) const
  {
    assess_input(sim);
    summary_[0].push_back(assess_summary(sim->agents()));
  }


  // returns {min, max, mean, stddev, mad}
  Analysis::Input Analysis::reduce(const LayerView& view, LayerView& tmp)
  {
    const float* __restrict p = view.data();
    ann_assume_aligned(p, 32);
    float mini = +std::numeric_limits<float>::max();
    float maxi = -std::numeric_limits<float>::max();
    double sum = 0.0;
    const int N = view.dim() * view.dim();
    for (int i =0; i < N; ++i) {
      const float val = p[i];
      if (val < mini) mini = val;
      if (val > maxi) maxi = val;
      sum += val;
    }
    const double mean = sum / N;
    double variance = 0.0;
    double mad = 0.0;
    for (int i =0; i < N; ++i) {
      const float val = p[i];
      variance += (val - mean) * (val - mean);
      mad += std::abs(val - mean);
    }
    return {
      mini, 
      maxi, 
      static_cast<float>(mean), (variance > 0.0) ? static_cast<float>(std::sqrt(variance / N)) : 0.f, 
      static_cast<float>(mad / N)
    };
  }


  void Analysis::assess_input(const Simulation* sim) const
  {
    LayerView tmp = sim->landscape()[Landscape::Layers::temp];
    input_[0][0].push_back( reduce(sim->landscape()[static_cast<Landscape::Layers>(sim->param().agents.input_layers[0])], tmp ) );
    input_[0][1].push_back( reduce(sim->landscape()[static_cast<Landscape::Layers>(sim->param().agents.input_layers[1])], tmp) );
    input_[0][2].push_back( reduce(sim->landscape()[static_cast<Landscape::Layers>(sim->param().agents.input_layers[2])], tmp) );

  }


  Analysis::Summary Analysis::assess_summary(const Population & Pop) const
  {
    double sfit = 0.0;
    int cfit = 0;
    for (auto x : Pop.fitness) {
      sfit += x;
      if (x > 0.f) ++cfit;
    }
    std::set<int> unique_anc;
    for (const auto& ind : Pop.pop) unique_anc.insert(ind.ancestor);
    auto* tmp_ann = Pop.tmp_ann.get();
    double complexity = 0.0;
    std::set<float*, ann_cmp> unique_ann(ann_cmp(tmp_ann->state_size()));
    for (int idx : unique_anc) {
      if (unique_ann.insert(tmp_ann->operator[](idx)).second) {
        complexity += tmp_ann->complexity(idx);
      }
    }

    double sforage = 0.0;
    for (auto x : Pop.foraged) {
      sforage += x;
    }

    double shandle = 0.0;
    for (auto x : Pop.handled) {
      shandle += x;
    }



    return { 
      static_cast<float>(sfit / Pop.fitness.size()), 
      cfit, 
      static_cast<int>(unique_ann.size()), 
      static_cast<float>(complexity / unique_ann.size()),
      static_cast<float>(sforage),
      static_cast<float>(shandle),
	  Pop.conflicts
    };
  }

}
