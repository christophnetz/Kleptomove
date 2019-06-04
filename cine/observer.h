#ifndef OBSERVER_H_INCLUDED
#define OBSERVER_H_INCLUDED

#include <memory>


namespace cine2 {

  
  // Poor mans observer.
  class Observer
  {
  public:
    Observer(): next_observer_(nullptr) {}
    virtual ~Observer() {}

    // notification
    virtual bool notify(void* userdata, long long msg)
    {
      return notify_next(userdata, msg);
    }


    // chain obs at the end of the list of this if not nullptr
    Observer& chain_back(Observer* obs) 
    {
      if (obs == this || obs == nullptr) return *this;
      Observer* last = this;
      while (last->next_observer_) last = last->next_observer_;
      last->next_observer_ = obs;
      return *this;
    }

  protected:
    bool notify_next(void* userdata, long long msg)
    {
      return (next_observer_ ? next_observer_->notify(userdata, msg) : true);
    }

  private:
    Observer* next_observer_;
  };


}


#endif

