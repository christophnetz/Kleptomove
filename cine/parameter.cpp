#include <omp.h>
#include <algorithm>
#include <map>
#include <ostream>
#include <fstream>
#include <streambuf>
#include <experimental/filesystem>
#include "parameter.h"


namespace filesystem = std::experimental::filesystem;


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

    clp_required(prey.N);
    clp_optional_val(prey.L, 3);
    clp_required(prey.ann);

    clp_optional_val(prey.sprout_radius, 10000);
    clp_optional_val(prey.mutation_prob, 0.001f);
    clp_optional_val(prey.mutation_step, 0.001f);
    clp_optional_val(prey.mutation_knockout, 0.001f);
    clp_optional_val(prey.noise_sigma, 0.1f);
    clp_optional_val(prey.cmplx_penalty, 0.01f);

    param.prey.input_layers = { { Layers::risk, Layers::grass, Layers::pred } };
    clp_optional_vec(prey.input_layers, param.prey.input_layers);
    param.prey.input_mask = { { 1, 1, 1 } };
    clp_optional_vec(prey.input_mask, param.prey.input_mask);

    clp_required(pred.N);
    clp_optional_val(pred.L, 3);
    clp_required(pred.ann);

    clp_optional_val(pred.sprout_radius, 10000);
    clp_optional_val(pred.mutation_prob, 0.001f);
    clp_optional_val(pred.mutation_step, 0.001f);
    clp_optional_val(pred.mutation_knockout, 0.001f);
    clp_optional_val(pred.noise_sigma, 0.1f);
    clp_optional_val(pred.cmplx_penalty, 0.01f);

    param.pred.input_layers = {{ Layers::risk, Layers::grass, Layers::pred } };
    clp_optional_vec(pred.input_layers, param.pred.input_layers);
    param.pred.input_mask = { { 1, 1, 1 } };
    clp_optional_vec(pred.input_mask, param.pred.input_mask);

    clp_optional_val(landscape.max_grass_cover, 1.0f);
	clp_optional_val(landscape.grass_growth, 0.01f);
	clp_optional_val(landscape.grass_deplete, 1.0f); //*&*
	clp_required(landscape.risk.image);
    param.landscape.risk.channel = ImageChannel(clp.required<int>("landscape.risk.channel"));
    param.landscape.risk.layer = Landscape::Layers::risk;

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

    stream(prey.N);
    stream(prey.L);
    stream_str(prey.ann);
    stream(prey.sprout_radius);
    stream(prey.mutation_prob);
    stream(prey.mutation_step);
    stream(prey.mutation_knockout);
    stream(prey.noise_sigma);
    stream(prey.cmplx_penalty);
    stream_array(prey.input_layers);
    stream_array(prey.input_mask);
    os << '\n';

    stream(pred.N);
    stream(pred.L);
    stream_str(pred.ann);
    stream(pred.sprout_radius);
    stream(pred.mutation_prob);
    stream(pred.mutation_step);
    stream(pred.mutation_knockout);
    stream(pred.noise_sigma);
    stream(pred.cmplx_penalty);
    stream_array(pred.input_layers);
    stream_array(pred.input_mask);
    os << '\n';

    stream(landscape.max_grass_cover);
	stream(landscape.grass_growth);
	stream(landscape.grass_deplete); //*&*
	stream_str(landscape.risk.image);
    stream(landscape.risk.channel);

    return os;
  }

#undef stream
#undef stream_str
#undef stream_array

}

