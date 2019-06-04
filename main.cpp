#include <cassert>
#include <iostream>
#include <stdexcept>
#include "cine/simulation.h"
#include "cine/cmd_line.h"
#include "cine/cnObserver.h"
#include "cinema/AppWin.h"
#include <cine/archive.hpp>


using namespace cine2;


static_assert(std::is_trivially_copyable<Individual>::value, 
              "Folks, you've messed up the Individual class");


int main(int argc, const char** argv)
{
  cmd::cmd_line_parser clp(argc, argv);
  try {
    bool gui = clp.flag("--gui");
    bool quiet = clp.flag("--quiet");
    auto config = clp.optional_val("config", std::string{});
    if (!config.empty()) clp.append(config_file_parser(config));
    Param param = parse_parameter(clp);
    auto unknown = clp.unrecognized();
    if (!unknown.empty()) {
      for (const auto& arg : unknown) {
        std::cerr << "unknown argument '" << arg << "'\n";
      }
      return 1;
    }
    // create simulation host
    std::unique_ptr<SimulationHost> host;
    host.reset(gui ? new cinema::AppWin() 
                   : new SimulationHost()
    );

    // create observer chain
    auto headObserver = std::unique_ptr<Observer>(new Observer());    // dummy observer for chaining
    std::unique_ptr<Observer> cmdline_observer = quiet ? nullptr : CreateSimpleObserver();
    std::unique_ptr<Observer> cn_observer = param.outdir.empty() ? nullptr : CreateCnObserver(param.outdir);
    headObserver->chain_back(cmdline_observer.get());
    headObserver->chain_back(cn_observer.get());
    if (!host->run(headObserver.get(), param)) {
      std::cerr << "\nSimulation terminated.\nBailing out.\n";
    }
    else {
      std::cout << "\nRegards.\n";
    }
  }
  catch (cmd::parse_error& err) {
    std::cerr << "\nParameter trouble: " << err.what() << '\n';
  }
  catch (std::exception& err) {
    std::cerr << "\nExeption caught: " << err.what() << '\n';
  }
  catch (...) {
    std::cerr << "\nUnknown exeption caught\n";
  }
  return 0;
}
