#include <iostream>
#include <fstream>
#include "simulation.h"
#include "archive.hpp"
#include "analysis.h"
#include <filesystem>


namespace fs = std::filesystem;


namespace cine2 {


const char* sourceMe = R"R(

# auxiliary function
import.raw <- function(filename, type, size) {
  finfo <- file.info(filename)
  if (is.na(finfo$size)) stop(paste(fname, "doesn't exist"))
  con <- file(filename, "rb", raw = TRUE)
  n <- finfo$size / size
  buf <- readBin(con, type, n, size=size)
  close(con)
  buf
}

# auxiliary function
import.generation <- function(G, what, stderr) {
  if (!(what=="pred" || what=="agents")) stop("argument what shall be 'agents' or 'pred'")
  extractor <- paste0(config$dir, '/depends/extract.exe')
  Args <- paste0('G="', toString(G), '" ' , "dir=", config$dir, " what=", what)
  system2(extractor, args=Args, stderr=stderr)
  ann <- matrix(import.raw(paste0(config$dir, "/tmp/", what, "_ann.tmp"), numeric(), 8),
                ncol=config[[paste0(what, ".ann.weights")]], byrow=T)
  fit <- import.raw(paste0(config$dir, "/tmp/", what, "_fit.tmp"), numeric(), 8)
  anc <- import.raw(paste0(config$dir, "/tmp/", what, "_anc.tmp"), integer(), 4)
  foa <- import.raw(paste0(config$dir, "/tmp/", what, "_foa.tmp"), numeric(), 8)
  han <- import.raw(paste0(config$dir, "/tmp/", what, "_han.tmp"), numeric(), 8)
  system2(extractor, paste0("dir=", config$dir, " --cleanup"))
  list(ann=ann, fit=fit, anc=anc, foa=foa, han=han)
}

# extract generation
# params:
#   G    : generation
generation <- function(G, stderr=F) {
  agents = import.generation(G, "agents", stderr)
  list(agents=agents)
}

# load input estimates
input <- function() {
  agents = matrix(import.raw(paste0(config$dir, '/agents_input.bin'), numeric(), 8), ncol=15, byrow=T)
#  pred = matrix(import.raw(paste0(config$dir, '/pred_input.bin'), numeric(), 8), ncol=15, byrow=T)
  cn <- c('min0', 'max0', 'mean0', 'std0', 'mad0', 
          'min1', 'max1', 'mean1', 'std1', 'mad1', 
          'min2', 'max2', 'mean2', 'std2', 'mad2') 
  colnames(agents) <- cn
#  colnames(pred) <- cn
  list(agents=agents) #CN: pred excluded , pred=pred
}
  
# load summary
summary <- function() {
  agents = matrix(import.raw(paste0(config$dir, '/agents_summary.bin'), numeric(), 8), ncol=8, byrow=T)
  cn <- c('pop fitness', 'repro fitness', 'repro ind', 'repro clades', 'complexity', 'foraging', 'handling', 'conflicts')
  colnames(agents) <- cn
#  colnames(pred) <- cn
  list(agents=agents) #CN: pred excluded, pred=pred
}
  
config$dir = getSrcDirectory(generation)[1]
)R";


  class CnObserver : public Observer
  {
  public:
    explicit CnObserver(const fs::path& path) 
    : Observer(), 
	    folder(path)
    {
    }
    
    ~CnObserver() override 
    {
    }

    // required observer interface
    bool notify(void* userdata, long long msg) override
    {
      auto sim = reinterpret_cast<const Simulation*>(userdata);
      using msg_type = Simulation::msg_type;
      
      switch (msg) {
        case msg_type::INITIALIZED:
          oa_agents_ann_.open(folder / "agents_ann.arc", sim->param().agents.ann);
          oa_agents_fit_.open(folder / "agents_fit.arc", "fitness");
          oa_agents_anc_.open(folder / "agents_anc.arc", "ancestors");
          oa_agents_foa_.open(folder / "agents_foa.arc", "forage");
          oa_agents_han_.open(folder / "agents_han.arc", "handle");

          break;
        case msg_type::GENERATION:
          stream_generation(sim->agents(), oa_agents_ann_, oa_agents_fit_, oa_agents_anc_, oa_agents_foa_, oa_agents_han_);

          break;
        case msg_type::FINISHED:
          stream_meta(sim);
          stream_analysis(sim);
          copy_dependencies();
          break;
      }
      return notify_next(userdata, msg);
    };

  private:
    void stream_generation(const Population& Pop, 
                           archive::oarch& oa_ann, 
                           archive::oarch& oa_fit, 
                           archive::oarch& oa_anc,
                           archive::oarch& oa_foa,
                           archive::oarch& oa_han)
    {
      oa_ann.insert(archive::compress(Pop.ann->data(),
                                      Pop.ann->N(),
                                      Pop.ann->state_size() * sizeof(float),
                                      Pop.ann->stride() * sizeof(float)));
      oa_fit.insert(archive::compress(Pop.fitness.data(),
                                      Pop.fitness.size(),
                                      sizeof(float)));
      oa_anc.insert(archive::compress((char*)Pop.pop.data() + offsetof(Individual, ancestor),
                                      Pop.pop.size(),
                                      sizeof(int),
                                      sizeof(Individual)));
      oa_foa.insert(archive::compress(Pop.foraged.data(),
                                      Pop.foraged.size(),
                                      sizeof(float)));
      oa_han.insert(archive::compress(Pop.handled.data(),
                                      Pop.handled.size(),
                                      sizeof(float)));
    }


    void stream_summary(std::ostream& os, const int N, const std::vector<Analysis::Summary>& summary)
    {
      const size_t g = summary.size();
      for (size_t i = 0; i < g; ++i) {
        double val = summary[i].ave_fitness; os.write((const char*)&val, sizeof(double));
        val = (summary[i].ave_fitness * N)/ summary[i].repro_ind; os.write((const char*)&val, sizeof(double));
        val = summary[i].repro_ind; os.write((const char*)&val, sizeof(double));
        val = summary[i].repro_ann; os.write((const char*)&val, sizeof(double));
        val = summary[i].complexity; os.write((const char*)& val, sizeof(double));
        val = summary[i].foragers; os.write((const char*)& val, sizeof(double));
		    val = summary[i].handlers; os.write((const char*)& val, sizeof(double));
		    val = summary[i].conflicts; os.write((const char*)& val, sizeof(double));
	  }
    }


    template <typename INPUT>
    void stream_input(std::ostream& os, const INPUT& input)
    {
      const size_t g = input[0].size();
      for (size_t i = 0; i < g; ++i) {
        for (int j = 0; j < 3; ++j) {
          double val = input[j][i].mini; os.write((const char*)&val, sizeof(double));
          val = input[j][i].maxi; os.write((const char*)&val, sizeof(double));
          val = input[j][i].mean; os.write((const char*)&val, sizeof(double));
          val = input[j][i].std; os.write((const char*)&val, sizeof(double));
          val = input[j][i].mad; os.write((const char*)&val, sizeof(double));
        }
      }
    }


    void stream_analysis(const Simulation* sim)
    {
      {
        std::ofstream os;
        os.open(folder / "agents_summary.bin", std::ios::out | std::ios::binary); 
        if (!os.is_open()) throw std::runtime_error("can't create agents_summary.bin");
        stream_summary(os, sim->param().agents.N, sim->analysis().agents_summary());
      }
      //{
      //  std::ofstream os;
      //  os.open(folder / "pred_summary.bin", std::ios::out | std::ios::binary); 
      //  if (!os.is_open()) throw std::runtime_error("can't create pred_summary.bin");
      //  stream_summary(os, sim->param().pred.N, sim->analysis().pred_summary());
      //}
      {
        std::ofstream os;
        os.open(folder / "agents_input.bin", std::ios::out | std::ios::binary); 
        if (!os.is_open()) throw std::runtime_error("can't create agents_input.bin");
        stream_input(os, sim->analysis().agents_input());
      }

    }


    void stream_meta(const Simulation* sim)
    {
      { // stream config
        std::ofstream os(folder / "config.ini");
        stream_parameter(os, sim->param(), "", "\n", "{", "}");
      }
      { // stream sourceMe
        std::ofstream os(folder / "sourceMe.R");
        os << "# Cinema generated script, do not edit.\n\n";
        os << "config = list(\n";
        stream_parameter(os, sim->param(), "  ", ",\n", "c(", ")");
        os << "\n# Metadata\n";
        os << "  agents.ann.weights = " << sim->agents().ann->state_size() << "\n";
        os << ")\n";
        os << sourceMe;
      }
    }

    void copy_dependencies()
    {
      auto cur = fs::current_path();
      if (!fs::exists(folder / "depends")) {
        fs::create_directory(folder / "depends");
      }
      fs::copy(cur / "extract.exe", folder / "depends", fs::copy_options::overwrite_existing);
    }

	  fs::path folder;
    archive::oarch oa_agents_ann_;
    archive::oarch oa_agents_fit_;
    archive::oarch oa_agents_anc_;
    archive::oarch oa_agents_foa_;
    archive::oarch oa_agents_han_;

  };


  std::unique_ptr<Observer> CreateCnObserver(const std::string& folder)
  {
    fs::path path(folder);
    fs::create_directory(path);
    return std::unique_ptr<Observer>(new CnObserver(path));
  }


}

