#ifndef CINE2_SIMULATION_H_INCLUDED
#define CINE2_SIMULATION_H_INCLUDED

#include <memory>
#include <vector>
#include "landscape.h"
#include "image.h"
#include "parameter.h"
#include "observer.h"
#include "any_ann.hpp"
#include "analysis.h"
#include "archive.hpp"


namespace cine2 {


  struct Population
  {
    std::vector<Individual> pop;        // curent pooulation
    std::vector<Individual> tmp_pop;    // new generation during reproduction, ancestors otherwise
    std::unique_ptr<any_ann> ann;       // Ann's of current population
    std::unique_ptr<any_ann> tmp_ann;   // new Anns during reproduction, ancestors Anns otherwise
    std::vector<float> foraged;         // fitness after last timestep
    std::vector<float> handled;         // fitness after last timestep
	int conflicts;
    std::vector<float> fitness;         // fitness after last timestep
    rndutils::mutable_discrete_distribution<int, rndutils::all_zero_policy_uni> rdist;  // reproduction distr.
  };


  class Simulation
  {
  public:
    // Messages send to the observer
    enum msg_type : int {
      INITIALIZED,          // send if simulation was initialized
      NEW_GENERATION,       // send if new generation was created
      PRE_TIMESTEP,         // send before every timestep
      POST_TIMESTEP,        // send after every timestep
      GENERATION,           // send after final timestep in one generation
      FINISHED,             // send if simulation is done and busted
      WATCHDOG,             // send to confirm app is running
    };

  public: 
    explicit Simulation(const Param& param);
    ~Simulation() {}

    const Population& agents() const { return agents_; }
    const Landscape& landscape() const { return landscape_; }
    const Param& param() const { return param_; }
    const Analysis& analysis() const { return analysis_; }

    int generation() const { return g_; }   // current generation
    int timestep() const { return t_; }     // current timestep
    bool fixed() const { return (g_ >= 0) && (g_ > param_.Gfix); }
    int dim() const { return landscape_.dim(); }

    // returns completion
    bool run(Observer* observer = nullptr); 

  private:
    void simulate_timestep(int t);
    void update_landscaperecord();
    void assess_fitness();
    void assess_inds();
    void create_new_generations();
    void resolve_grazing_and_attacks();
    void init_layer(image_layer imla);
    void init_anns_from_archive(Population& Pop, archive::iarch& ia);

    int g_, t_;
    const Param param_;
    Population agents_;
    //Population pred_;
    std::vector<int> attacking_inds_;
    std::vector<Individual*> attacked_potentially_;
    std::vector<Individual*> attacked_inds;
    std::vector<int> shuffle_vec;
    Landscape landscape_;
    Analysis analysis_;
  };


  class SimulationHost
  {
  public:
    SimulationHost() {}
    virtual ~SimulationHost() {}

    const Simulation* simulation() const { return sim_.get(); }

    virtual bool run(Observer* observer, const Param& param) 
    {
      sim_.reset(new Simulation(param) );
      return sim_->run(observer);
    }

  protected:
    std::unique_ptr<Simulation> sim_;
  };


  std::unique_ptr<Observer> CreateSimpleObserver();

}

#endif

