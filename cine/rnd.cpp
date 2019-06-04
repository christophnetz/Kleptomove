#include "rnd.hpp"


namespace rnd {

  rndutils::default_engine thread_local reng = rndutils::make_random_engine();
}

