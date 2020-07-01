#include <stdexcept>
#include "any_ann.hpp"
#include "simulation.h"


namespace cine2 {


  any_ann::any_ann(int N, int state_size, int size)
  : N_(N),
    state_size_(state_size),
    size_(size),
    state_(nullptr)
  {
    state_ = (float*)_mm_malloc(N_ * size_ * sizeof(float), 16);
    std::memset(state_, 0, N_ * size_ * sizeof(float));
  }


  any_ann::~any_ann()
  {
    _mm_free(state_);
  }


  namespace ann_visitors {

    struct mutate
    {
      mutate(const Param::ind_param& iparam, bool Fixed)
      : mdist(iparam.mutation_prob),
        sdist(0.0f, iparam.mutation_step),
        kdist(iparam.mutation_knockout),
        fixed(Fixed)
      {
      }

      template <typename Neuron, typename T>
      void operator()(T* state, size_t, size_t) const
      {
        if (!fixed) {
          for (int w = 0; w < Neuron::total_weights; ++w) { 
            if (mdist(rnd::reng)) { state[w] += sdist(rnd::reng); }
            if (kdist(rnd::reng)) { state[w] = 0.f; }
          }
        }
        // clear feedback scratch
        for (int s = Neuron::feedback_scratch_begin; s < Neuron::state_size; ++s) {
          state[s] = 0.f;
        }
      }

      const std::bernoulli_distribution mdist;
      const std::cauchy_distribution<float> sdist;
      const std::bernoulli_distribution kdist;
      bool fixed;
    };


    struct complexity
    {
      template <typename Neuron, typename T>
      void operator()(const T* state, size_t, size_t)
      {
        for (int w = 0; w < Neuron::total_weights; ++w) {
          if (state[w] == 0.f) zeros += 1.f;
        }
        weights += Neuron::total_weights;
      }

      float zeros = 0.f;
      int weights = 0;
    };

  }


  template <int L, typename ANN>
  class concrete_ann : public any_ann
  {
    static_assert(std::is_trivially_copyable<ANN>::value, "Who messed with the Ann class?");

  public:
    explicit concrete_ann(int N) : any_ann(N, ANN::state_size, sizeof(ANN))
    {
    }


    float complexity(int idx) const override
    {
      const ANN* __restrict pann = reinterpret_cast<const ANN*>(state_);
      ann_visitors::complexity visitor;
      ann::visit_neurons(pann[idx], visitor);
      return 1.f - (visitor.zeros / visitor.weights);
    }


    void move(const Landscape& landscape,
              std::vector<Individual>& pop,
              const Param::ind_param& iparam) override
    {
      using Layers = Landscape::Layers;
      using env_info_t = std::array<float, L*L>;

	  //structure for evaluation of cells
      struct zip_eval_cell {
        float eval;		//suitability score (overall preference)
        float eval2;	//suitability score (for strategy decision)
        int cell;		//cell number
      };

      ANN* __restrict pann = reinterpret_cast<ANN*>(state_);
      const int N = static_cast<int>(iparam.N);
      const auto noise = std::uniform_real_distribution<float>(1.0f - iparam.noise_sigma, 1.0f + iparam.noise_sigma);
  #   pragma omp parallel for schedule(static,128)
      for (int p = 0; p < N; ++p) {							//cycle thrugh the agents
        if (pop[p].alive() && !(pop[p].handle())) {			//conditions for movement (alive and not handling)

		  //gather information from landscape
          Coordinate pos = pop[p].pos;							//gather position agent
          std::array<env_info_t, ANN::input_size> env_input;	//[input number definition stuff]
          for (int i = 0; i < ANN::input_size; ++i) {			//for cycle through inputs [4][can be changed]
            env_input[i] = landscape[static_cast<Layers>(iparam.input_layers[i])].gather<L>(pos);	//inputs are gathered from the first n layer in "landscape" at the position "pos"

          }

          // reflect about the possible cells (we are still in the agents for-cycle)
          float best_eval = - std::numeric_limits<float>::max();
          typename ANN::input_t input;
          typename ANN::input_t input2; //To get bias of second node
          std::array<zip_eval_cell, L*L> zip;
          for (int i = 0; i < L*L; ++i) {
            for (int j = 0; j < ANN::input_size; ++j) {
              
                input[j] = iparam.input_mask[j] * noise(rnd::reng) * (env_input[j][i]);
                input2[j] = 0.f;

            }
            
            auto output = pann[p](input);   // ask ANN
            float eval = output[0];			//first output, named eval
            float eval2;

            if (iparam.obligate) {
            auto output2 = pann[p](input2);   // ask ANN
            eval2 = output2[1];		//second output, named eval2

            }
            else {
            eval2 = output[1];
            }

            best_eval = std::max(best_eval, eval);		//best_eval is updated,
            zip[i] = { eval, eval2, i };				//structure filled with evaluation
          }

          // resolve ambiguities. bring 'best' ones to the front
          auto it = std::partition(zip.begin(), zip.end(), [=](const auto& a){ return a.eval == best_eval; }) - 1;
          if (it != zip.begin()) {
            // yep, more than one 'best' alternatives, select one at random
            it = zip.begin() + rndutils::uniform_signed_distribution<int>(0, static_cast<int>(std::distance(zip.begin(), it)))(rnd::reng);
          }
          pop[p].pos = landscape.wrap(pos + Coordinate{short((it->cell % L) - L/2), short((it->cell / L) - L/2)});

          /*
		  double s_prob = 1.0 / (1.0 + exp(-static_cast<double> (it->eval2)));	//creating s_prob which is function of eval2
          std::bernoulli_distribution s_decision(s_prob);							//this become the probability of adopting foraging strategy
		  pop[p].forage = s_decision(rnd::reng);				//if condition apply, foraging of agent set to TRUE
		  */
          if (iparam.forage) {
            pop[p].forage(true);
          }
          else {
           pop[p].forage(it->eval2 >= 0);

          }
		  
        }
      }
    }


    void mutate(const Param::ind_param& iparam, bool fixed) override
    {
      ANN* __restrict pann = reinterpret_cast<ANN*>(state_);
      const int N = static_cast<int>(iparam.N);
      const ann_visitors::mutate mutate_visitor(iparam, fixed);
  #   pragma omp parallel for schedule(static, 128)
      for (int i = 0; i < N; ++i) {
        ann::visit_neurons(pann[i], mutate_visitor);
      }
    }

  };


  template <int L, typename ANN>
  std::unique_ptr<any_ann> make_any_ann_2(int N)
  {
    return std::unique_ptr<any_ann>( new concrete_ann<L, ANN>(N) );
  }


  template <int L>
  std::unique_ptr<any_ann> make_any_ann_1(int N, const char* ann_descr)
  {
    if (0 == std::strcmp(ann_descr, "DumbAnn")) return make_any_ann_2<L, DumbAnn>(N);
    if (0 == std::strcmp(ann_descr, "SimpleAnn")) return make_any_ann_2<L, SimpleAnn>(N);
    if (0 == std::strcmp(ann_descr, "SimpleAnnFB")) return make_any_ann_2<L, SimpleAnnFB>(N);
    if (0 == std::strcmp(ann_descr, "SmartAnn")) return make_any_ann_2<L, SmartAnn>(N);
    // ToDo: add your Anns here
    return nullptr;
  }


  std::unique_ptr<any_ann> make_any_ann(int L, int N, const char* ann_descr)
  {
    if (L == 3) return  make_any_ann_1<3>(N, ann_descr);
    if (L == 5) return make_any_ann_1<5>(N, ann_descr);
    if (L == 7) return make_any_ann_1<7>(N, ann_descr);
	if (L == 33) return make_any_ann_1<33>(N, ann_descr);

    // ToDo: add your Anns here
    std::runtime_error("Unknown Ann type");
    return nullptr;
  }


}
