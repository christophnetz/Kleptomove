#ifndef GAME_WATCHES_HPP_INCLUDED
#define GAME_WATCHES_HPP_INCLUDED


#include <chrono>
#include <cstdint>


namespace game_watches {


  template <typename Clock, typename Duration>
  class StopwatchImpl
  {
  public:
    using clock = Clock;
    using duration = Duration;
    
    StopwatchImpl() 
    { 
      epoch_ = start_ = clock::now();
      reset(); 
    }
    
    void start()
    {
      if (!running_) {
        start_ = clock::now();
        running_ = true;
      }
    }

    void stop()
    {
      if (running_) {
        accum_ += clock::now() - start_;
        running_ = false;
      }
    }

    void reset()
    {
      accum_ = typename clock::duration(0);
      running_ = false;
    }

    bool is_running() { return running_; }

    auto now() const
    {
      return std::chrono::duration_cast<duration>(clock::now());
    }

    duration elapsed() const
    {
      if (!running_) return std::chrono::duration_cast<duration>(accum_);
      return std::chrono::duration_cast<duration>(accum_ + clock::now() - start_);
    }

    duration elapsed_since_epoch() const
    {
      return std::chrono::duration_cast<duration>(clock::now() - epoch_);
    }

  private:
    bool running_;
    typename clock::time_point start_;
    typename clock::duration accum_;
    typename clock::time_point epoch_;
  };


  class Driftwatch
  {
  public:
    Driftwatch() : rtticks_(0), watch_() {}
    
    void tick(bool paused, bool realtime)
    {
      if (paused || !realtime) watch_.stop();
      if (!paused && realtime) { ++rtticks_; watch_.start(); }
    }

    double drift(bool paused, bool realtime, double dt) const
    {
      return (paused || !realtime) ? 0.0 : dt * rtticks_ - watch_.elapsed().count();
    }

    void sync(double dt)
    {
      rtticks_ = static_cast<std::intmax_t>(watch_.elapsed().count() / dt);
    }

  private:
    std::intmax_t rtticks_;
    StopwatchImpl<std::chrono::steady_clock, std::chrono::duration<double>> watch_;
  };


  using Stopwatch = StopwatchImpl<std::chrono::steady_clock, std::chrono::duration<double>>;

}


#endif
