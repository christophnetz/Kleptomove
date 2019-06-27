#include <iostream>
#include <experimental/filesystem>
#include "simulation.h"
#include "game_watches.hpp"
#include "cmd_line.h"


namespace filesystem = std::experimental::filesystem;


namespace cine2 {


  Simulation::Simulation(const Param& param)
    : g_(-1), t_(-1),
    param_(param)
  {
    using Layers = Landscape::Layers;

    prey_.pop = std::vector<Individual>(param.prey.N);
    prey_.tmp_pop = std::vector<Individual>(param.prey.N);
    prey_.ann = make_any_ann(param.prey.L, param.prey.N, param.prey.ann.c_str());
    prey_.fitness = std::vector<float>(param.prey.N, 0.f);
    prey_.tmp_ann = make_any_ann(param.prey.L, param.prey.N, param.prey.ann.c_str());

    pred_.pop = std::vector<Individual>(param.pred.N);
    pred_.tmp_pop = std::vector<Individual>(param.pred.N);
    pred_.ann = make_any_ann(param.pred.L, param.pred.N, param.pred.ann.c_str());
    pred_.fitness = std::vector<float>(param.pred.N, 0.f);
    pred_.tmp_ann = make_any_ann(param.pred.L, param.pred.N, param.pred.ann.c_str());

    // initial landscape layers from image fies
    init_layer(param_.landscape.risk);
    if (landscape_.dim() < 32) throw std::runtime_error("Landscape too small");

    // full grass cover
    for (auto& g : landscape_[Layers::grass]) g = param.landscape.max_grass_cover;
    //empty grass cover
    for (auto& g : landscape_[Layers::grass]) g = 0.0f;

    // initial positions
    auto coorDist = std::uniform_int_distribution<short>(0, short(landscape_.dim() - 1));
    for (auto& p : prey_.pop) { p.pos.x = coorDist(rnd::reng); p.pos.y = coorDist(rnd::reng); }
    //for (auto& p : pred_.pop) { p.pos.x = coorDist(rnd::reng); p.pos.y = coorDist(rnd::reng); }

    // initial occupancies and observable densities
    landscape_.update_occupancy(Layers::prey_count, Layers::prey, Layers::pred_count, Layers::pred, Layers::grass, prey_.pop.cbegin(), prey_.pop.cend(), param_.landscape.prey_kernel);
    //landscape_.update_occupancy(Layers::pred_count, Layers::pred, pred_.pop.cbegin(), pred_.pop.cend(), param_.landscape.pred_kernel);

    // optional: initialization from former runs
    if (!param_.init_prey_ann.empty()) {
      init_anns_from_archive(prey_, archive::iarch(param_.init_prey_ann));
    }
    //if (!param_.init_pred_ann.empty()) {
    //  init_anns_from_archive(pred_, archive::iarch(param_.init_pred_ann));
    //}
  }


  void Simulation::init_anns_from_archive(Population& Pop, archive::iarch& ia)
  {
    auto cm = ia.extract(param_.initG >= 0 ? std::min(param_.initG, param_.G - 1) : param_.G - 1);
    if (cm.un != Pop.ann->N()) throw cmd::parse_error("Number of ANNs doesn't match");
    if (cm.usize != Pop.ann->type_size()) throw cmd::parse_error("ANN state size doesn't match");
    auto dst = Pop.ann->data();
    uncompress(dst, cm, Pop.ann->stride() * sizeof(float));
  }


  namespace detail {


    template <typename FITNESSFUN>
    void assess_fitness(Population& population,
      const Param::ind_param& iparam,
      FITNESSFUN fitness_fun)
    {
      const float cmplx_penalty = iparam.cmplx_penalty;
      const auto& pop = population.pop;
      const auto& ann = population.ann;
      auto& fitness = population.fitness;
      const int N = static_cast<int>(pop.size());
#     pragma omp parallel for schedule(static)
      for (int i = 0; i < N; ++i) {
        fitness[i] = fitness_fun(pop[i], ann->complexity(i), cmplx_penalty);
      }
      population.rdist.mutate(fitness.cbegin(), fitness.cend());
    }


    void create_new_generation(const Landscape& landscape,
      Population& population,
      const Param::ind_param& iparam,
      bool fixed)
    {
      const auto& pop = population.pop;
      const auto& ann = *population.ann;
      const int N = static_cast<int>(pop.size());
#     pragma omp parallel 
      {
        auto& tmp_pop = population.tmp_pop;
        auto& tmp_ann = *population.tmp_ann;
        const auto& rdist = population.rdist;
        const auto coorDist = rndutils::uniform_signed_distribution<short>(-iparam.sprout_radius, iparam.sprout_radius);
#       pragma omp for schedule(static)
        for (int i = 0; i < N; ++i) {
          const int ancestor = rdist(rnd::reng);
          auto newPos = pop[ancestor].pos + Coordinate{ coorDist(rnd::reng), coorDist(rnd::reng) };
          tmp_pop[i].sprout(landscape.wrap(newPos), ancestor);
          tmp_ann.assign(ann, ancestor, i);   // copy ann
        }
      }
      population.tmp_ann->mutate(iparam, fixed);

      using std::swap;
      swap(population.pop, population.tmp_pop);
      swap(population.ann, population.tmp_ann);
    }

  }


#define simulation_observer_notify(msg) \
  if ((observer ? !observer->notify(this, msg) : true)) return false


  bool Simulation::run(Observer* observer)
  {
    // burn-in
    simulation_observer_notify(INITIALIZED);
    const int Gb = param_.Gburnin;
    for (int gb = 0; gb < Gb; ++gb) {
      const int Tb = param_.T;
      for (int tb = 0; tb < Tb; ++tb) {
        simulate_timestep();
        simulation_observer_notify(WATCHDOG);   // app alive?
      }

      //assess_fitness(); //CN: fix?
      // clear fitness
      prey_.fitness.assign(prey_.fitness.size(), 0.f);
      //pred_.fitness.assign(pred_.fitness.size(), 0.f);
      assess_fitness(); //CN: fix?
      create_new_generations();
    }
    const int G = param_.G;
    for (g_ = 0; g_ < G; ++g_) {
      simulation_observer_notify(NEW_GENERATION);
      const int T = fixed() ? param_.Tfix : param_.T;
      for (t_ = 0; t_ < T; ++t_) {
        simulate_timestep();
        simulation_observer_notify(POST_TIMESTEP);
        // to print one screenshot
        /*
        if (g_ == 50 && t_ == 25) {
          Image screenshot2(std::string("../settings/screenshot.png"));

          layer_to_image_channel(screenshot2, landscape_[Landscape::Layers::prey_count], blue);
          layer_to_image_channel(screenshot2, landscape_[Landscape::Layers::pred_count], red);
          layer_to_image_channel(screenshot2, landscape_[Landscape::Layers::grass], green);
          save_image(screenshot2, std::string("../settings/screenshot.png"));
        }
        */
        //to print one screenshot end
      }



      assess_fitness();
      analysis_.generation(this);
      simulation_observer_notify(GENERATION);
      create_new_generations();
    }
    simulation_observer_notify(FINISHED);
    return true;
  }

#undef simulation_observer_notify


  void Simulation::simulate_timestep()
  {
    using Layers = Landscape::Layers;

    // grass growth
    const int DD = landscape_.dim() * landscape_.dim();
    float* __restrict cover = landscape_[Layers::grass].data();
    float* __restrict abundance = landscape_[Layers::risk].data();
    ann_assume_aligned(cover, 32);
    const float max_grass_cover = param_.landscape.max_grass_cover;
    const float grass_growth = param_.landscape.grass_growth;
    //#   pragma omp parallel for schedule(static)
    //    for (int i = 0; i < DD; ++i) {
    //      if (std::bernoulli_distribution(abundance[i] / 100.f)(rnd::reng)) {  // altered: probability that items drop
    //        cover[i] = std::min(max_grass_cover, cover[i] + grass_growth);
    //
    //      }
    //    }
    //    cover = abundance;
    //
    auto last_prey = prey_.pop.data() + prey_.pop.size();
    for (auto prey = prey_.pop.data(); prey != last_prey; ++prey) {
      prey->do_handle();
    }

    landscape_.update_occupancy(Layers::prey_count, Layers::prey, Layers::pred_count, Layers::pred, Layers::grass, prey_.pop.cbegin(), prey_.pop.cend(), param_.landscape.prey_kernel);

    // move
    prey_.ann->move(landscape_, prey_.pop, param_.prey);
    //pred_.ann->move(landscape_, pred_.pop, param_.pred);

    // update occupancies and observable densities



    resolve_grazing_and_attacks();


    landscape_.update_occupancy(Layers::prey_count, Layers::prey, Layers::pred_count, Layers::pred, Layers::grass, prey_.pop.cbegin(), prey_.pop.cend(), param_.landscape.prey_kernel);

  }


  void Simulation::assess_fitness()
  {
    detail::assess_fitness(prey_, param_.prey, Param::prey_fitness);
    //detail::assess_fitness(pred_, param_.pred, Param::pred_fitness);
  }


  void Simulation::create_new_generations()
  {
    detail::create_new_generation(landscape_, prey_, param_.prey, fixed());
    //detail::create_new_generation(landscape_, pred_, param_.pred, fixed());
  }


  void Simulation::resolve_grazing_and_attacks()
  {
    using Layers = Landscape::Layers;
    const float grass_deplete = param_.landscape.grass_deplete;  //*&*
    LayerView prey_count = landscape_[Layers::prey_count];
    LayerView pred_count = landscape_[Layers::pred_count];
    LayerView risk = landscape_[Layers::risk];
    LayerView grass = landscape_[Layers::grass];
    LayerView old_grass = landscape_[Layers::temp];
    old_grass.copy(grass);

    attacking_inds_.clear();
    attacked_potentially_.clear();
    attacked_inds.clear();
    auto last_prey = prey_.pop.data() + prey_.pop.size();
    for (auto prey = prey_.pop.data(); prey != last_prey; ++prey) {

      if (prey->handle() == false) {
        const Coordinate pos = prey->pos;
        //if (grass(pos) >= 1.0f) {
        if (prey->forage) {
          if (std::bernoulli_distribution(risk(pos))(rnd::reng)) {
            prey->pick_item();
            //grass(pos) += 1.0f;
          }
        }
      }
    }

    for (auto prey = prey_.pop.data(); prey != last_prey; ++prey) {
      if (!prey->handling && !prey->forage) {

        //if (grass(pos) >= 1.0f) {

        const Coordinate pos = prey->pos;
        if (grass(pos) >= 1.0f) {
          attacking_inds_.push_back(prey);

        }
      }
    }

    for (auto attacking : attacking_inds_) {
      //std::vector<Individual*> tmp_vict{};
      for (auto attacked_pot = prey_.pop.data(); attacked_pot != last_prey; ++attacked_pot) {
        const Coordinate pos = attacked_pot->pos;
        if (attacking->pos == pos && attacking != attacked_pot) {  // self excluded
          if (attacked_pot->handle()) {
            attacked_potentially_.push_back(attacked_pot);
          }
        }
      }
      if (!attacked_potentially_.empty()) {
        std::uniform_int_distribution<int> rind(0, static_cast<int>(attacked_potentially_.size() - 1));
        int focal_ind = rind(rnd::reng);
        attacked_inds.push_back(attacked_potentially_[focal_ind]);
        attacked_potentially_.clear();
      }
    }

    for (int i = 0; i < attacking_inds_.size(); ++i) {

      float prob_to_fight = 1.0f;

      //if (attacked_inds[i]->handle())
      //  prob_to_fight = 0.5f;
      //else
      //  prob_to_fight = 0.2f;


      std::bernoulli_distribution fight(prob_to_fight);							//sampling whether fight occurs
      std::bernoulli_distribution initiator_wins(1.0);							//sampling whether the initiator wins or not

      if (fight(rnd::reng)) {
        if (initiator_wins(rnd::reng)) {
          attacking_inds_[i]->handling = attacked_inds[i]->handling;
          attacking_inds_[i]->handle_time = attacked_inds[i]->handle_time;
          //attacking_inds_[i]->food += 1.0f;
          attacked_inds[i]->flee(landscape_);

        }
        else
          attacking_inds_[i]->flee(landscape_);
        //Energetic costs
        //attacking_inds_[i]->food -= 0.0f;
        //attacked_inds[i]->food -= 0.0f;


      }
    }

    /*
    // find *attacking* predators
    attacking_inds_.clear();
    auto last_pred = pred_.pop.data() + pred_.pop.size();
    for (auto pred = pred_.pop.data(); pred != last_pred; ++pred) {
      const Coordinate pos = pred->pos;
      if (prey_count(pos)) {
        if (std::bernoulli_distribution(risk(pos))(rnd::reng)) {
          attacking_inds_.push_back(pred);
        }
        else {
          // non-attacking predator, remove
          --pred_count(pos);
        }
      }
    }




    // find *attacked* prey and graze on the fly
    attacked_potentially_.clear();
    auto last_prey = prey_.pop.data() + prey_.pop.size();
    for (auto prey = prey_.pop.data(); prey != last_prey; ++prey) {
      if (prey->alive()) {
        const Coordinate pos = prey->pos;
        if (pred_count(pos)) {
          attacked_potentially_.push_back(prey);
        }
        //*&*        prey->food += old_grass(pos) / prey_count(pos);
        if (old_grass(pos) > grass_deplete * prey_count(pos)) { //*&* New if else statement to resolve grazing conflict
          prey->food += grass_deplete;
        }
        else {
          prey->food += old_grass(pos) / prey_count(pos);
        }
        //*&*        grass(pos) = 0.f;  //   depleted
        grass(pos) -= grass_deplete; //*&*
        if (grass(pos) < 0.f) grass(pos) = 0.f; //*&*, necessary or resolved elsewhere?
      }
    }

    // resolve 'interactions'. This is now
    //   O(#attacking pred * #attacked prey)
    // instead of
    //   O(#pred * #prey)
    for (auto pred : attacking_inds_) {
      for (auto prey : attacked_potentially_) {
        const Coordinate pos = prey->pos;
        if (pred->pos == pos) {
          prey->die();
          pred->food += 1.f / pred_count(pos);
        }
      }
    }*/
  }


  void Simulation::init_layer(image_layer imla)
  {
    Image image(std::string("../settings/") + imla.image);
    if (landscape_.dim() == 0) {
      landscape_ = Landscape(image.width());
    }
    if (!(image.width() == landscape_.dim() && image.height() == landscape_.dim())) {
      throw std::runtime_error("image dimension mismatch");
    }
    image_channel_to_layer(landscape_[imla.layer], image, imla.channel);
  }


  class SimpleObserver : public Observer
  {
  public:
    SimpleObserver() {}
    ~SimpleObserver() override {}

    // required observer interface
    bool notify(void* userdata, long long msg) override
    {
      auto sim = reinterpret_cast<Simulation*>(userdata);
      using msg_type = Simulation::msg_type;
      switch (msg) {
      case msg_type::INITIALIZED:
        std::cout << "Simulation initialized\n";
        break;
      case msg_type::NEW_GENERATION: {
        watch_.reset();
        watch_.start();
        std::cout << "Generation: " << sim->generation() << (sim->fixed() ? "*  " : "   ");
        break;
      }
      case msg_type::GENERATION: {
        std::cout << sim->analysis().prey_summary().back().ave_fitness << "   ";
        std::cout << sim->analysis().prey_summary().back().repro_ind << "   ";
        std::cout << sim->analysis().prey_summary().back().repro_ann << "  (";
        std::cout << sim->analysis().prey_summary().back().complexity << ");   ";

        //std::cout << sim->analysis().pred_summary().back().ave_fitness << "   ";
        //std::cout << sim->analysis().pred_summary().back().repro_ind << "   ";
        //std::cout << sim->analysis().pred_summary().back().repro_ann << "  (";
        //std::cout << sim->analysis().pred_summary().back().complexity << ");   ";

        std::cout << (int)(1000 * watch_.elapsed().count()) << "ms\n";
        break;
      }
      case msg_type::FINISHED:
        std::cout << "\rSimulation finished\n";
        break;
      }
      return notify_next(userdata, msg);
    };

  private:
    game_watches::Stopwatch watch_;
  };


  std::unique_ptr<Observer> CreateSimpleObserver()
  {
    return std::unique_ptr<Observer>(new SimpleObserver());
  }

}
