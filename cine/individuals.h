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

    void pick_item(int h_time) {
      handle_time = -h_time;			//handling time is setted	[WE SHOULD MAKE THIS A PARAMETER IN "CONFIG.INI"]
      handling = true;			//agend handling status is set to true
    }

	//HANDLING FUNCTION (per agent)
    void do_handle() {
      if (handle_time < 0 && handling) {		//if agent handling time is smaller than zero AND agent is handling 
        ++handle_time;								//handling time is udpated
      }
      if (handle_time == 0 && handling) {		//if handling time has reached zero AND agent is handling
        food += 1.0f;								//food is consumed
        handling = false;							//the handling status is resetted (FALSE)
      }
												//ELSE (agent is not handling), do nothig.
    }

    void flee(const Landscape& landscape, int flee_radius) {

      handling = false;				//handling status reset to false
      handle_time = 0;				//handling time reset to 0 

      std::uniform_int_distribution<int> dxy(-flee_radius, flee_radius);	//uniform distribution of the fleeing distance

      pos = landscape.wrap(pos + Coordinate{ short(dxy(rnd::reng)), short(dxy(rnd::reng)) });		//new position with difference in coordinates sampled form previous distribution


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
