#include <iostream>
#include <filesystem>
#include "simulation.h"
#include "game_watches.hpp"
#include "cmd_line.h"
#include "cassert"
#include <fstream> 

namespace filesystem = std::filesystem;


namespace cine2 {


  Simulation::Simulation(const Param& param)
    : g_(-1), t_(-1),
    param_(param)
  {
    using Layers = Landscape::Layers;

    agents_.pop = std::vector<Individual>(param.agents.N);
    agents_.tmp_pop = std::vector<Individual>(param.agents.N);
    agents_.ann = make_any_ann(param.agents.L, param.agents.N, param.agents.ann.c_str());
    agents_.fitness = std::vector<float>(param.agents.N, 0.f);
    agents_.foraged = std::vector<float>(param.agents.N, 0);
    agents_.handled = std::vector<float>(param.agents.N, 0);
    agents_.conflicts = 0;
    agents_.tmp_ann = make_any_ann(param.agents.L, param.agents.N, param.agents.ann.c_str());

    shuffle_vec.resize(param.agents.N);
    std::iota(shuffle_vec.begin(), shuffle_vec.end(), 0);

    agents_.ann->initialize(param.agents);

    // initial landscape layers from image fies
    init_layer(param_.landscape.capacity); //capacity
    if (landscape_.dim() < 32) throw std::runtime_error("Landscape too small");

    // full grass cover
    //for (auto& g : landscape_[Layers::items]) g = param.landscape.max_grass_cover;
    const int DD = landscape_.dim() * landscape_.dim();
    float* __restrict items = landscape_[Layers::items].data();
    float* __restrict capacity = landscape_[Layers::capacity].data();
    for (int i = 0; i < DD; ++i) {

      items[i] = floor(capacity[i] * param.landscape.max_item_cap);

    }

    //empty grass cover
    //for (auto& g : landscape_[Layers::items]) g = 0.0f;

    // initial positions
    auto coorDist = std::uniform_int_distribution<short>(0, short(landscape_.dim() - 1));
    for (auto& p : agents_.pop) { p.pos.x = coorDist(rnd::reng); p.pos.y = coorDist(rnd::reng); }

    // initial occupancies and observable densities
    landscape_.update_occupancy(Layers::foragers_count, Layers::foragers, Layers::klepts_count,
      Layers::klepts, Layers::handlers_count, Layers::handlers, Layers::nonhandlers, agents_.pop.cbegin(), agents_.pop.cend(), param_.landscape.foragers_kernel);

    // optional: initialization from former runs
    if (!param_.init_agents_ann.empty()) {
      init_anns_from_archive(agents_, archive::iarch(param_.init_agents_ann));
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

      population.conflicts = 0;

      using std::swap;
      swap(population.pop, population.tmp_pop);
      swap(population.ann, population.tmp_ann);

      LayerView items_rec = landscape[Landscape::Layers::items_rec];
      LayerView foragers_rec = landscape[Landscape::Layers::foragers_rec];
      LayerView klepts_rec = landscape[Landscape::Layers::klepts_rec];
      LayerView foragers_intake = landscape[Landscape::Layers::foragers_intake];
      LayerView klepts_intake = landscape[Landscape::Layers::klepts_intake];
      items_rec.clear();
      foragers_rec.clear();
      klepts_rec.clear();
      foragers_intake.clear();
      klepts_intake.clear();
    }


    void assess_inds(Population& population) {

      const auto& pop = population.pop;
      auto& foraged = population.foraged;
      auto& handled = population.handled;

      const int N = static_cast<int>(pop.size());

      for (int i = 0; i < N; ++i) {
        foraged[i] = pop[i].forage_count;
        handled[i] = pop[i].handle_count;
      }
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
        simulate_timestep(tb);

        simulation_observer_notify(WATCHDOG);   // app alive?
      }

      //assess_fitness(); //CN: fix?
      // clear fitness
      agents_.fitness.assign(agents_.fitness.size(), 0.f);

      assess_fitness(); //CN: fix?
      create_new_generations();
    }
    const int G = param_.G;



    for (g_ = 0; g_ < G; ++g_) {
      simulation_observer_notify(NEW_GENERATION);
      const int T = fixed() ? param_.Tfix : param_.T;
      for (t_ = 0; t_ < T; ++t_) {
        simulate_timestep(t_);
        simulation_observer_notify(POST_TIMESTEP);
        
        
        
        // to print one screenshot

        //if (g_ % 5 == 0 && t_ == 50) {

        //    // pad generations helps with regex finding later
        //    const std::string strGen_tmp = std::to_string(g_);
        //    const std::string strGen = std::string(5 - strGen_tmp.length(), '0') + strGen_tmp;

        //    const std::string stritems = std::string(param_.outdir + "/" + strGen + "_" + "items.txt");
        //    const std::string strforagers = std::string(param_.outdir + "/" + strGen + "_" + "foragers.txt");
        //    const std::string strklepts = std::string(param_.outdir + "/" + strGen + "_" + "klepts.txt");
        //    const std::string strintakefor = std::string(param_.outdir + "/" + strGen + "_" + "foragers_intake.txt");
        //    const std::string strintakeklept = std::string(param_.outdir + "/" + strGen + "_" + "klepts_intake.txt");
        //    //const std::string strcapacity = std::string(std::to_string(g_) + param_.outdir + "capacity.txt");
        //    std::ofstream writeoutitems(stritems);
        //    std::ofstream writeoutforagers(strforagers);
        //    std::ofstream writeoutklepts(strklepts);
        //    std::ofstream writeoutintakefor(strintakefor);
        //    std::ofstream writeoutintakeklept(strintakeklept);
        //    //std::ofstream writeoutcapacity(strcapacity);

        //    layer_to_text(landscape_[Landscape::Layers::items_rec], writeoutitems);
        //    layer_to_text(landscape_[Landscape::Layers::foragers_rec], writeoutforagers);
        //    layer_to_text(landscape_[Landscape::Layers::klepts_rec], writeoutklepts);
        //    layer_to_text(landscape_[Landscape::Layers::foragers_intake], writeoutintakefor);
        //    layer_to_text(landscape_[Landscape::Layers::klepts_intake], writeoutintakeklept);
        //    //layer_to_text(landscape_[Landscape::Layers::capacity], writeoutcapacity);



        //    writeoutitems.close();
        //    writeoutforagers.close();
        //    writeoutklepts.close();
        //    writeoutintakefor.close();
        //    writeoutintakeklept.close();
        ////	
        ////    const std::string strGen_tmp = std::to_string(g_);
        ////    const std::string strGen = std::string(5 - strGen_tmp.length(), '0') + strGen_tmp;
        ////    Image screenshot3(std::string("../settings/empty.png"));
        ////    layer_to_image_channel(screenshot3, landscape_[Landscape::Layers::foragers_count], blue);
        ////    layer_to_image_channel(screenshot3, landscape_[Landscape::Layers::klepts_count], red);
        ////    layer_to_image_channel(screenshot3, landscape_[Landscape::Layers::handlers_count], green);
        ////    layer_to_image_channel_2(screenshot3, (landscape_[Landscape::Layers::items]), alha, static_cast<float>(param_.landscape.max_item_cap));
        ////    //layer_to_image_channel(screenshot2, landscape_[Landscape::Layers::items], alha);
        //    //save_image(screenshot3, std::string(param_.outdir + "/" + strGen + ".png"));

        //}



        //to print one screenshot end
      }

      if (!param_.outdir.empty() && g_ > G - 10 ) {

        const std::string stritems = std::string(param_.outdir + "/" + std::to_string(g_) + "items.txt");
        const std::string strforagers = std::string(param_.outdir + "/" + std::to_string(g_) + "foragers.txt");
        const std::string strklepts = std::string(param_.outdir + "/" + std::to_string(g_) + "klepts.txt");
        const std::string strintakefor = std::string(param_.outdir + "/" + std::to_string(g_) + "foragers_intake.txt");
        const std::string strintakeklept = std::string(param_.outdir + "/" + std::to_string(g_) + "klepts_intake.txt");
        //const std::string strcapacity = std::string(std::to_string(g_) + param_.outdir + "capacity.txt");
        std::ofstream writeoutitems(stritems);
        std::ofstream writeoutforagers(strforagers);
        std::ofstream writeoutklepts(strklepts);
        std::ofstream writeoutintakefor(strintakefor);
        std::ofstream writeoutintakeklept(strintakeklept);
        //std::ofstream writeoutcapacity(strcapacity);

        layer_to_text(landscape_[Landscape::Layers::items_rec], writeoutitems);
        layer_to_text(landscape_[Landscape::Layers::foragers_rec], writeoutforagers);
        layer_to_text(landscape_[Landscape::Layers::klepts_rec], writeoutklepts);
        layer_to_text(landscape_[Landscape::Layers::foragers_intake], writeoutintakefor);
        layer_to_text(landscape_[Landscape::Layers::klepts_intake], writeoutintakeklept);
        //layer_to_text(landscape_[Landscape::Layers::capacity], writeoutcapacity);



        writeoutitems.close();
        writeoutforagers.close();
        writeoutklepts.close();
        writeoutintakefor.close();
        writeoutintakeklept.close();
        //writeoutcapacity.close();
      }

      assess_fitness();
      assess_inds();
      analysis_.generation(this);
      simulation_observer_notify(GENERATION);
      create_new_generations();
    }



    simulation_observer_notify(FINISHED);
    return true;
  }

#undef simulation_observer_notify


  void Simulation::simulate_timestep(const int t)
  {
    using Layers = Landscape::Layers;

    // grass growth
    const int DD = landscape_.dim() * landscape_.dim();
    float* __restrict items = landscape_[Layers::items].data();					//items now refers to the layer of food items (in landscape)
    float* __restrict capacity = landscape_[Layers::capacity].data();			//capacity refers to the maximum capacity layer (in landscape)
    ann_assume_aligned(items, 32);
    const float max_item_cap = param_.landscape.max_item_cap;
    const float item_growth = param_.landscape.item_growth;
    //#   pragma omp parallel for schedule(static)

    for (int i = 0; i < DD; ++i) {
      if (std::bernoulli_distribution(item_growth * capacity[i] / max_item_cap)(rnd::reng) ) {  // altered: probability that items drop, && capacity[i] > 0.2
        items[i] = std::min(floor(max_item_cap), floor(items[i] + 1.0f));
      }
    }



    //landscape_.update_occupancy(Layers::foragers_count, Layers::foragers, Layers::klepts_count, Layers::klepts, Layers::handlers_count, Layers::handlers, Layers::nonhandlers, agents_.pop.cbegin(), agents_.pop.cend(), param_.landscape.foragers_kernel);

    // move
    agents_.ann->move(landscape_, agents_.pop, param_.agents);

    // update occupancies and observable densities
    landscape_.update_occupancy(Layers::foragers_count, Layers::foragers, Layers::klepts_count, Layers::klepts, Layers::handlers_count, Layers::handlers, Layers::nonhandlers, agents_.pop.cbegin(), agents_.pop.cend(), param_.landscape.foragers_kernel);

    if (t == param_.T / 2) {
      LayerView foragers_intake = landscape_[Landscape::Layers::foragers_intake];
      LayerView klepts_intake = landscape_[Landscape::Layers::klepts_intake];
      foragers_intake.clear();
      klepts_intake.clear();
    }

    if ( t >= param_.T / 2) {
      update_landscaperecord();

    }


    //RESOLVE GRAZING AND ATTACK function!
    resolve_grazing_and_attacks();


    landscape_.update_occupancy(Layers::foragers_count, Layers::foragers, Layers::klepts_count, Layers::klepts, Layers::handlers_count, Layers::handlers, Layers::nonhandlers, agents_.pop.cbegin(), agents_.pop.cend(), param_.landscape.foragers_kernel);

  }

  void Simulation::update_landscaperecord()
  {
    using Layers = Landscape::Layers;

    // grass growth
    const int DD = landscape_.dim() * landscape_.dim();
    float* __restrict items = landscape_[Layers::items].data();					//items now refers to the layer of food items (in landscape)
    float* __restrict foragers_count = landscape_[Layers::foragers_count].data();			//capacity refers to the maximum capacity layer (in landscape)
    float* __restrict klepts_count = landscape_[Layers::klepts_count].data();			//capacity refers to the maximum capacity layer (in landscape)
    float* __restrict items_rec = landscape_[Layers::items_rec].data();			//capacity refers to the maximum capacity layer (in landscape)
    float* __restrict foragers_rec = landscape_[Layers::foragers_rec].data();			//capacity refers to the maximum capacity layer (in landscape)
    float* __restrict klepts_rec = landscape_[Layers::klepts_rec].data();			//capacity refers to the maximum capacity layer (in landscape)
    //#   pragma omp parallel for schedule(static)

    for (int i = 0; i < DD; ++i) {
      items_rec[i] += items[i];
      foragers_rec[i] += foragers_count[i];
      klepts_rec[i] += klepts_count[i];

    }
  }

  void Simulation::assess_fitness()
  {
    detail::assess_fitness(agents_, param_.agents, Param::agents_fitness);
  }

  void Simulation::assess_inds()
  {

    detail::assess_inds(agents_);
  }

  void Simulation::create_new_generations()
  {
    detail::create_new_generation(landscape_, agents_, param_.agents, fixed());
  }


  void Simulation::resolve_grazing_and_attacks()
  {
    using Layers = Landscape::Layers;
    const float detection_rate = param_.landscape.detection_rate;
    //LayerView foragers_count = landscape_[Layers::foragers_count];
    //LayerView klepts_count = landscape_[Layers::klepts_count];
    //LayerView capacity = landscape_[Layers::capacity];
    LayerView items = landscape_[Layers::items];
    LayerView foragers_intake = landscape_[Layers::foragers_intake];
    LayerView klepts_intake = landscape_[Layers::klepts_intake];
    LayerView handlers = landscape_[Layers::handlers_count];
    LayerView old_grass = landscape_[Layers::temp];
    old_grass.copy(handlers);


    attacking_inds_.clear();
    attacked_potentially_.clear();
    attacked_inds.clear();

    auto last_agents = agents_.pop.data() + agents_.pop.size();


    for (int i = 0; i < agents_.pop.size(); ++i) {
      if (!agents_.pop[i].handling && !agents_.pop[i].foraging) {

        const Coordinate pos = agents_.pop[i].pos;
        if (handlers(pos) >= 1.0f) {
          attacking_inds_.push_back(i);

        }
      }
    }

    for (auto i : attacking_inds_) {						//cycle through the agents in that same vector

      for (auto attacked_pot = agents_.pop.data(); attacked_pot != last_agents; ++attacked_pot) {
        const Coordinate pos = attacked_pot->pos;
        if (agents_.pop[i].pos == pos && &agents_.pop[i] != attacked_pot) {  // self excluded
          if (attacked_pot->handling) {
            attacked_potentially_.push_back(attacked_pot);

          }
        }
      }
      if (!attacked_potentially_.empty()) {								//if then that vector is NOT empty
        std::uniform_int_distribution<int> rind(0, static_cast<int>(attacked_potentially_.size() - 1));		//sample one (random)
        int focal_ind = rind(rnd::reng);																	//now called "focal_ind"
        attacked_inds.push_back(attacked_potentially_[focal_ind]);			//added to the vector of ACTUALLY ATTACKED.

        attacked_potentially_.clear();										//clearing the POTENTIALLY ATTACKED vector
      }

    }


    assert(attacked_inds.size() == attacking_inds_.size() && "vector lengths uneven");

    // Shuffling
    std::vector<std::pair<int, Individual*>> conflicts_v(attacking_inds_.size());
    for (int i = 0; i < attacking_inds_.size(); ++i) {
      conflicts_v[i] = { attacking_inds_[i], attacked_inds[i] };
    }
    std::shuffle(conflicts_v.begin(), conflicts_v.end(), rnd::reng);


    for (int i = 0; i < conflicts_v.size(); i++) {				//cycle through the agents who attack
      float prob_to_fight = 1.0f;									//they always fight

      //if (attacked_inds[i]->handle())
      //  prob_to_fight = 0.5f;
      //else
      //  prob_to_fight = 0.2f;


      std::bernoulli_distribution fight(prob_to_fight);								//sampling whether fight occurs
      std::bernoulli_distribution initiator_wins(1.0)/*initiator always wins*/;		//sampling whether the initiator wins or not
      if (conflicts_v[i].second->handling) {			///isn't this always true?
        if (fight(rnd::reng)) {
          if (initiator_wins(rnd::reng)) {

            agents_.pop[conflicts_v[i].first].handling = conflicts_v[i].second->handling;
            agents_.pop[conflicts_v[i].first].handle_time = conflicts_v[i].second->handle_time;
            //attacking_inds_[i]->food += 1.0f;
            conflicts_v[i].second->flee(landscape_, param_.agents.flee_radius);

          }
          else
            agents_.pop[conflicts_v[i].first].attacker_flee(landscape_, param_.agents.flee_radius);
          //Energetic costs

          //attacking_inds_[i]->food -= 0.0f;
          //attacked_inds[i]->food -= 0.0f;

        }

      }
    }

    agents_.conflicts += static_cast<int>(conflicts_v.size());

    conflicts_v.clear();

    std::shuffle(shuffle_vec.begin(), shuffle_vec.end(), rnd::reng);



    for (int i : shuffle_vec) {
      auto& agent = agents_.pop[i];
      //for (auto agents = agents_.pop.data(); agents != last_agents; ++agents) {
      if (agent.handle() == false) {
        const Coordinate pos = agent.pos;

        if (agent.foraging && !agent.just_lost) {
          if (items(pos) >= 1.0f) {
            if (std::bernoulli_distribution(1.0 - pow((1.0f - detection_rate), items(pos)))(rnd::reng)) { // Ind searching for items
              agent.pick_item(param_.agents.handling_time);
              items(pos) -= 1.0f;
            }
          }
        }
      }



      else {
        if (agent.do_handle()) {
          if (agent.foraging) {
            foragers_intake(agent.pos) += 1.0f;

          }
          else {
            klepts_intake(agent.pos) += 1.0f;
          }
        }

      }

      if (agent.just_lost) {
        agent.just_lost = false;
      }
    }



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
        std::cout << sim->analysis().agents_summary().back().ave_fitness << "   ";
        std::cout << sim->analysis().agents_summary().back().repro_ind << "   ";
        std::cout << sim->analysis().agents_summary().back().repro_ann << "  (";
        std::cout << sim->analysis().agents_summary().back().complexity << ");   \t\t";
        std::cout << sim->analysis().agents_summary().back().foragers << "   ";
        std::cout << sim->analysis().agents_summary().back().handlers << "   \t";

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
