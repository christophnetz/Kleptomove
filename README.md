# Source Code for _Kleptomove_, an individual-based model of animal movement and competition strategies

[![Project Status: Active – The project has reached a stable, usable state and is being actively developed.](https://www.repostatus.org/badges/latest/active.svg)](https://www.repostatus.org/#active)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.4905475.svg)](https://doi.org/10.5281/zenodo.4905475)

This repository holds the source code for the _Kleptomove_ simulation, an individual-based, evolutionary model of the co-evolution of animal movement and competition strategies, written by [Christoph Netz](https://www.rug.nl/staff/c.f.g.netz/), based on a previous model by Hanno Hildenbrandt, in the [Modelling Adaptive Response Mechanisms Group (Weissing Lab)](https://www.marmgroup.eu/) at the Groningen Institute for Evolutionary Life Science, at the University of Groningen.

The source code for analyses of this simulation's output can be found on [Github](https://github.com/pratikunterwegs/kleptomove-ms), or archived on Zenodo: https://doi.org/10.5281/zenodo.4904497.

## Contact and Attribution

Please contact [Christoph Netz](c.f.g.netz@rug.nl) and Franjo Weissing ([PI](f.j.weissing@rug.nl)) for questions on the model or the project.
Please contact [Pratik Gupte](p.r.gupte@rug.nl) or Christoph or Franjo for questions about the associated manuscript.

Please cite this simulation as

```bibtex
@software{netz_2021_kleptomove,
  author       = {Christoph FG Netz and
                  Pratik Rajan Gupte},
  title        = {{Kleptomove: Source code for an individual-based 
                   model of the co-evolution of animal movement and
                   competition strategies}},
  month        = jun,
  year         = 2021,
  publisher    = {Zenodo},
  version      = {v0.9.1},
  doi          = {10.5281/zenodo.4905476},
  url          = {https://doi.org/10.5281/zenodo.4905476}
}
```

## Simulation Source Code: Key Files

The simulation source code is in `cine/`, while code for a GUI is in `cinema/`. This simulation is Windows only.

`simulation.cpp` runs the main simulation. Individual agents are defined in `individuals.h`, the landscape in `landscape.h`, and neural networks in `{any_ann.hpp, any_ann.cpp}`. Parameters are defined, read from command line and 
written out through `{parameter.h, parameter.cpp}`, relying on `cmd_line.h`. Preliminary data analysis is performed in `{analysis.cpp, analysis.hpp}`, and output is generated via an observer chain in `{observer.h, cnObserver.h}` and `cnObserver.cpp`, relying on `{archive.cpp, archive.hpp}`.
Files in the subproject `extract/` provides a custom executable to extract data from the archives. 

## Simulation Source Code: File Descriptions

This refers to code in the `cine/` directory.

- `main.cpp` Reads in the parameters, creates the `SimulationHost` and `Simulation` class, chains back the observers and runs the simulation.

- `simulation.h` 
    
    - Defines the `Simulation` class, consisting of a population structure (defined in the same file, with neural networks defined in `any_ann.hpp`, `any_ann.cpp` and `ann.hpp`, and the individual class defined in `individuals.h`), landscape (in `landscape.h`), parameters (in `parameter.h`) and analysis (in `analysis.h`). 
    
    - This simulation is encapsuled in a `SimulationHost` class that also contains the first observer.

    - The simulation is started using the function `bool run(Observer* observer = nullptr);`, that along with its component functions is defined in `simulation.cpp`.

- `simulation.cpp` 

    - Defines the constructor of the simulation class (initializing population (agents, neural networks, fitness vector, other record keeping), and landscape. 
    
    - Defines the main function `bool run(Observer* observer = nullptr);` which loops over generations and within generations over timesteps, with main functions being `simulate_timestep(t_);`, and at the end of a generation `assess_fitness();`, `assess_inds` and `create_new_generations();`, as well as `analysis_.generation()` (defined in `analysis.cpp`) and notification messages to the observer chain.

    - Within a timestep, resource items regenerate, agents move (defined in `any_ann.cpp`), occupancy is updated (defined in `landscape.h`), and grazing and stealing events are resolved. At the end of the generation, fitness values are calculated in `assess_fitness();`, individual time budget is assessed in `assess_inds`, and a new generation is created from the calculated fitness values, with some probability of mutation (defined in `any_ann.cpp`).

- `any_ann.hpp` 
    
    - Defines the class `any_ann`, which contains an array of all network weights, total number of ANNs within the array and the length of one individual.

    - Important functions are `move()` (for individual movements within timesteps), `mutate()` (mutation of weights during reproduction) and `initialize()` (at the beginning of the simulation). These functions are defined in `any_ann.cpp`.

- `any_ann.cpp` Constructor of `any_ann`, and the functions `move()`, `mutate()`, and `initialize()`, explained above.

- `ann.hpp` Artificial neural network library for feedforward and recursive networks.

- `individuals.h` Defines the `Individual` structure with all state variables such as position or food, and functions implementing individual actions like `handle` or `flee`. ANNs are stored separately in the `population` class.

- `landscape.h` Defines the `landscape` class with all the contained layers. The `update_occupancy` function updates the different layers with the momentary positions of individuals. 

- `convolution.h` Provides the function to transform discrete individual counts into gaussian density kernels.

- `image.h` and `image.cpp` Allow the transformation of landscape layers into images that can be written out to file, and vice versa the transformation of images to layers (when setting the `kernels32.png` as the `landscape.capacity` parameter).

- `rnd.hpp`, `rnd.cpp` and `rndutils.hpp` Random number generation and custom distributions (`mutable_discrete_distribution`, `uniform_signed_distribution`).

- `parameter.h` and `parameter.cpp` 

    - Contains the parameter structure, which is split into general parameters and substructures for agents, the landscape and the gui. 
    
    - Additionally contains a parser for parameters for input from command line (in `cmd_line.h`), declaration of different network types (in `ann.hpp`) whose usage is defined by a parameter, fitness function for individuals, default values for parameters and streaming of parameters to file.

- `analysis.cpp` and `analysis.hpp` At the end of each generation, `Analysis::generation` is called on the entire simulation class, which then provides two different sets of data: 
    
    - An assessment of inputs, which documents the minimum, maximum, mean and median of available cues for all three sensory input layers (resource items, handlers and non-handlers), and 
    
    - Summary statistics on the population consisting of average fitness, individuals with fitness > 0, number of unique anns, and total number of handling and foraging events.

    This analysis is then streamed to file by cnObserver.cpp at the end of the simulation (upon `msg_type::FINISHED`).

- `cnObserver.cpp` This observer of class `Observer` (from `observer.h`) is chained to the head observer of the simulation class, and through it receives messages during the simulation. 

    - Upon `msg_type::INITIALIZED`, five different archive files are opened (see `archive.cpp` and `archive.hpp`).

    - Upon `msg_type::GENERATION`, the observer writes to file all ANNs, fitness values, ancestry data and foraging and handling counts for all individuals in the present generation in compressed format.

    - Upon `msg_type::FINISHED`, the observer writes out the analysis of all generations (input statistics and summary population statistics, from `analysis.cpp` and `analysis.hpp`), as well as the parameters used and the `sourceMe.R` script to extract the data.

    - This `R` script relies on an `extract.exe` file that is custom-built in the sub-project `extract/`.

- `game_watches.hpp` Time measurements during the simulation run.

## The `cinema/` Directory

The scripts in this folder generate the simulation GUI, containing a landscape view, a histogram of ANN weights and timelines of the metrics calculated in the summary structure of `cine/analysis.cpp`.
This code has no influence on the data produced during the simulation, and we will not elaborate further on it here. For feedback and questions please drop us a mail. 

## `data/`

This stores the simulation data locally.

## Simulation Data

These simulation data can be found at the University of Groningen Dataverse repository **HERE: ADD REPO HERE**.
