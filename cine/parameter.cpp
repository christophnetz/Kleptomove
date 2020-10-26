#include <omp.h>
#include <algorithm>
#include <map>
#include <ostream>
#include <fstream>
#include <streambuf>
#include <filesystem>
#include "parameter.h"


namespace filesystem = std::filesystem;


namespace cine2 {

  
#define clp_required(x) (param.x = clp.required<decltype(param.x )>( #x ))
#define clp_optional_val(x, val) (param.x = clp.optional_val( #x, val))
#define clp_optional_vec(x, val) (clp.optional_vec( #x, val))


  Param parse_parameter(cmd::cmd_line_parser& clp)
  {
    using Layers = Landscape::Layers;

    Param param;
    clp_optional_val(Gburnin, 0);
    clp_required(G);
    clp_required(T);

    clp_optional_val(Gfix, param.G);
    clp_optional_val(Tfix, param.T);
    clp_optional_val(outdir, std::string{});
    clp_optional_val(omp_threads, omp_get_max_threads());
    omp_set_num_threads(param.omp_threads);

    clp_required(agents.N);
    clp_optional_val(agents.L, 3);
    clp_required(agents.ann);

    clp_optional_val(agents.obligate, false);
    clp_optional_val(agents.forage, false);
    clp_optional_val(agents.sprout_radius, 10000);
    clp_optional_val(agents.flee_radius, 10);
    clp_optional_val(agents.handling_time, 5);
    clp_optional_val(agents.mutation_prob, 0.001f);
    clp_optional_val(agents.mutation_step, 0.001f);
    clp_optional_val(agents.mutation_knockout, 0.001f);
    clp_optional_val(agents.noise_sigma, 0.1f);
    clp_optional_val(agents.cmplx_penalty, 0.01f);

    param.agents.input_layers = { { Layers::nonhandlers, Layers::handlers, Layers::items } };
    clp_optional_vec(agents.input_layers, param.agents.input_layers);
    param.agents.input_mask = { { 1, 1, 1} };
    clp_optional_vec(agents.input_mask, param.agents.input_mask);



    clp_optional_val(landscape.max_item_cap, /*1.0f*/10.0f);
	clp_optional_val(landscape.item_growth,/*0.01f*/0.01f);
	clp_optional_val(landscape.detection_rate, 0.1f);
	clp_required(landscape.capacity.image);
    param.landscape.capacity.channel = ImageChannel(clp.required<int>("landscape.capacity.channel"));
    param.landscape.capacity.layer = Landscape::Layers::capacity;

    clp_optional_val(gui.wait_for_close, true);
    param.gui.selected = { { true, true, true, false } };
    clp_optional_vec(gui.selected, param.gui.selected);

    return param;
  }

#undef clp_required
#undef clp_optional_val
#undef clp_optional_vec


  cmd::cmd_line_parser config_file_parser(const std::string& config)
  {
    std::ifstream is(config);
    if (!is) throw cmd::parse_error("can't open config file");
    std::string str((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
    return cmd::cmd_line_parser(str);
  }


#define stream(x) (os << prefix << #x "=" << param.x << postfix)
#define stream_str(x) (os << prefix << #x "=\"" << param.x << '"' << postfix)
#define stream_array(x) (do_stream_array(os << prefix, #x, param.x, lb, rb) << postfix)


  namespace {

    template <typename C>
    std::ostream& do_stream_array(std::ostream& os, const char* name, const C& cont, const char* lb, const char* rb)
    {
      os << name << '=' << lb;
      for (size_t i = 0; i < cont.size() - 1; ++i) {
        os << cont[i] << ',';
      }
      os << cont.back() << rb;
      return os;
    }

  }


  std::ostream& stream_parameter(std::ostream& os, 
                                 const Param& param, 
                                 const char* prefix,
                                 const char* postfix, 
                                 const char* lb = "{", 
                                 const char* rb = "}")
  {
    stream(Gburnin);
    stream(G);
    stream(T);
    stream(Gfix);
    stream(Tfix);
    stream_str(outdir);
    stream(omp_threads);
    os << '\n';

    stream(agents.N);
    stream(agents.L);
    stream_str(agents.ann);
    stream(agents.obligate);
    stream(agents.forage);
    stream(agents.sprout_radius);
    stream(agents.flee_radius);
    stream(agents.handling_time);
    stream(agents.mutation_prob);
    stream(agents.mutation_step);
    stream(agents.mutation_knockout);
    stream(agents.noise_sigma);
    stream(agents.cmplx_penalty);
    stream_array(agents.input_layers);
    stream_array(agents.input_mask);
    os << '\n';



    stream(landscape.max_item_cap);
	stream(landscape.item_growth);
	stream(landscape.detection_rate); //*&*
	stream_str(landscape.capacity.image);
    stream(landscape.capacity.channel);

    return os;
  }

#undef stream
#undef stream_str
#undef stream_array

}

