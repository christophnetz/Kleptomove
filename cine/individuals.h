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
    Individual() : pos(0, 0), food(0), foraging(false), handling(false), just_lost(false), handle_time(0), forage_count(0.f), handle_count(0), ancestor(0)
    {
    }

    void sprout(Coordinate Pos, int ancestor_idx)
    {
      pos = Pos;
      food = 0.f;
      handle_count = 0;
      forage_count = 0;
      foraging = false;
      handling = false;
	    just_lost = false;
      handle_time = 0;
      ancestor = ancestor_idx;
    }

    bool alive() const { return food >= 0.f; }
    bool handle() const { return handling; }
    void forage(bool decision) {
      foraging = decision;
      if (decision) {
        forage_count += 1.f;
      }
    }

    void pick_item(int h_time) {
      handle_time = -h_time;			//handling time is setted	[WE SHOULD MAKE THIS A PARAMETER IN "CONFIG.INI"]

      handling = true;			//agend handling status is set to true

    }

    //HANDLING FUNCTION (per agent)
    bool do_handle() {
      if (handle_time < 0 && handling) {		//if agent handling time is smaller than zero AND agent is handling 
        ++handle_time;								//handling time is udpated
        handle_count += 1.f;

        return false;
      }
      if (handle_time == 0 && handling) {		//if handling time has reached zero AND agent is handling
        food += 1.0f;								//food is consumed
        handle_count += 1.f;
        handling = false;							//the handling status is resetted (FALSE)

        return true;
      }
      else {
        return false;
      }//ELSE (agent is not handling), do nothig.
    }

    void flee(const Landscape& landscape, int flee_radius) {

      if (handling) {
        std::uniform_int_distribution<int> dxy(-flee_radius, flee_radius);	//uniform distribution of the fleeing distance
        pos = landscape.wrap(pos + Coordinate{ short(dxy(rnd::reng)), short(dxy(rnd::reng)) });		//new position with difference in coordinates sampled form previous distribution

      }
      just_lost = true;
      handling = false;				//handling status reset to false
      handle_time = 0;				//handling time reset to 0 
    };


    void attacker_flee(const Landscape& landscape, int flee_radius) {


      std::uniform_int_distribution<int> dxy(-flee_radius, flee_radius);	//uniform distribution of the fleeing distance
      pos = landscape.wrap(pos + Coordinate{ short(dxy(rnd::reng)), short(dxy(rnd::reng)) });		//new position with difference in coordinates sampled form previous distribution

      handling = false;				//handling status reset to false
      handle_time = 0;				//handling time reset to 0 
    };

    void die() { food = -1.f; }

    Coordinate pos;
    float food;
    bool foraging;
    bool handling;
	  bool just_lost;
    int handle_time;
    float handle_count;
    float forage_count;
    int ancestor;
  };






}



#endif
