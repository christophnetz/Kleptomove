#ifndef CINE2_ANALYSIS_H_INCLUDED
#define CINE2_ANALYSIS_H_INCLUDED

#include <array>
#include <vector>


namespace cine2 {


  // used for the summary statistic
  class Analysis
  {
  public:
    struct Input
    {
      float mini;
      float maxi;
      float mean;
      float std;      // standard deviation
      float mad;      // mean absolute deviation
    };

    struct Summary
    {
      float ave_fitness;
      int repro_ind;
      int repro_ann;
      float complexity;
      float foragers;
      float handlers;
	    int conflicts;
    };

  public:
    Analysis() {}

    void generation(const class Simulation* sim) const;
    
    const std::vector<Summary>& agents_summary() const { return summary_[0]; }
    //const std::vector<Summary>& pred_summary() const { return summary_[1]; }
    const std::array<std::vector<Input>, 3>& agents_input() const { return input_[0]; }
    //const std::array<std::vector<Input>, 3>& pred_input() const { return input_[1]; }

  private:
    static Input reduce(const class LayerView& view, class LayerView& tmp);
    void assess_input(const class Simulation* sim) const;
    Summary assess_summary(const struct Population& Pop) const;

    mutable std::array<std::vector<Summary>, 1> summary_; //with pred 2
    mutable std::array<std::array<std::vector<Input>, 3>, 1> input_;
  };
  
}


#endif
