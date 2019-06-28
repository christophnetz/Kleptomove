#ifndef CINE2_INDIVIDUALS_H_INCLUDED
#define CINE2_INDIVIDUALS_H_INCLUDED

#include <vector>
#include <type_traits>
#include <iterator>
#include "rndutils.hpp"
#include "ann.hpp"
#include "rnd.hpp"
#include "landscape.h"    // Coordinate


namespace cine2 {


  struct Individual
  {
    Individual() : pos(0, 0), food(0), foraging(false), handling(false), handle_time(0), ancestor(0)
    {
    }

    void sprout(Coordinate Pos, int ancestor_idx)
    {
      pos = Pos;
      food = 0.f;
      ancestor = ancestor_idx;
    }

    bool alive() const { return food >= 0.f; }
    bool handle() const { return handling; }
    bool forage() const { return foraging; }
    void pick_item() {
      handle_time = -2;
      handling = true;
    }
    void do_handle() {
      if (handle_time < 0 && handling) {
        ++handle_time;
      }
      if (handle_time == 0 && handling) {
        food += 1.0f;
        handling = false;
      }


    }

    void flee(const Landscape& landscape, int flee_radius) {

      handling = false;
      handle_time = 0;

      std::uniform_int_distribution<int> dxy(-flee_radius, flee_radius);

      pos = landscape.wrap(pos + Coordinate{ short(dxy(rnd::reng)), short(dxy(rnd::reng)) });


    };
    void die() { food = -1.f; }

    Coordinate pos;
    float food;
    bool foraging;
    bool handling;
    int handle_time;
    int ancestor;
  };






}



#endif
