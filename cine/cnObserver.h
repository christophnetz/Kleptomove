#ifndef CINE2_CNOBSERVER_H_INCLUDED
#define CINE2_CNOBSERVER_H_INCLUDED

#include <string>
#include <cine/observer.h> 


namespace cine2 {

  std::unique_ptr<class Observer> CreateCnObserver(const std::string& folder);

}


#endif
