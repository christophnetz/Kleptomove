#ifndef CINE2_PARAMETER_H_INCLUDED
#define CINE2_PARAMETER_H_INCLUDED


#include <iosfwd>
#include <string>
#include <array>
#include <deque>
#include "landscape.h"
#include "rnd.hpp"
#include "individuals.h"
#include "convolution.h"
#include "image.h"
#include "ann.hpp"
#include "cmd_line.h"


namespace cine2 {


  using namespace ann;


  using DumbAnn = Network<float,
		Layer< Neuron<3, activation::zero>, 1>
  >;


  using default_activation = activation::identity;


	using SimpleAnn = Network<float,
		Layer< Neuron<3, default_activation>, 2>
	>;


	using SimpleAnnFB = Network<float,
		Layer< Neuron<3, default_activation, feedback::direct>, 2>
	>;


	using SmartAnn = Network<float,
		Layer< Neuron<3, activation::rtlu>, 3>,
		Layer< Neuron<3, default_activation>, 2>
	>;


  struct image_layer
  {
    std::string image;
    ImageChannel channel;
    Landscape::Layers layer;
  };


  struct Param
  {
    int Gburnin;          // burnin-generations
    int G;                // generations
    int T;                // time ticks per generation
    int Gfix;             // fixation generation
    int Tfix;             // time ticks per fixed generation
    std::string outdir;   // output folder
    int omp_threads;

    struct ind_param
    {
      int N;
      int L;
      std::string ann;

      bool obligate;
      bool forage;
      int sprout_radius;
      int flee_radius;
      int handling_time;
      float mutation_prob;
      float mutation_step;
      float mutation_knockout;
      float noise_sigma;
      float cmplx_penalty;

      std::array<int, 3> input_layers;
      std::array<float, 3> input_mask;
    };
    
    ind_param agents;
    //ind_param pred;

    static float agents_fitness(const Individual& ind, float cmplx, float penalty)
    {
      return ind.alive() ? std::max(0.f, ind.food - cmplx * penalty) : 0.0f;
    }

    //static float pred_fitness(const Individual& ind, float cmplx, float penalty)
    //{
    //  return std::max(0.f, ind.food - cmplx * penalty);
    //}

    struct
    {
      image_layer capacity;
      float max_item_cap;
	  float item_growth;
	  float detection_rate; //*&*
      GaussFilter<3> foragers_kernel;
      GaussFilter<3> klepts_kernel;
    } landscape;

    struct
    {
      std::deque<std::pair<int,int>> breakpoints{};
      bool wait_for_close;
      std::array<bool, 4> selected;
    } gui;

    //std::string init_pred_ann;
    std::string init_agents_ann;
    int initG;
  };


  Param parse_parameter(cmd::cmd_line_parser& clp);
  cmd::cmd_line_parser config_file_parser(const std::string& config);

  // write as textfile
  std::ostream& stream_parameter(std::ostream& os, 
                                 const Param& param, 
                                 const char* prefix, 
                                 const char* postfix, 
                                 const char* lb, 
                                 const char* rb);

}

#endif
