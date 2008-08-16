#ifndef CACHE_DIRECTORY_ENTRY_H
#define CACHE_DIRECTORY_ENTRY_H

#include <string>
#include <iostream>
using namespace std;

class CacheDirectoryEntry
{
 public:
  enum cstate_t {
    INVALID, 
    EXCLUSIVE,
    SHARED,
    NUM_CSTATE_STATES
  };
  
  CacheDirectoryEntry();
  virtual ~CacheDirectoryEntry();
  
  void setCState(cstate_t state);
  
  bool readable();
  bool writable();
  
  //FIXME: return string (don't cout)
  void toString();
  
 private:
  cstate_t cstate;
  
};

#endif
