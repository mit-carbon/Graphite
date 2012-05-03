#pragma once

#include <string>
#include <cassert>

#include "fixed_types.h"

using namespace std;

class CacheState
{
public:
   enum Type
   {
      INVALID = 0,
      SHARED,
      OWNED,
      MODIFIED,
      DATA_INVALID,
      CLEAN,
      DIRTY,
      NUM_STATES
   };

   CacheState(Type cstate = INVALID) : _cstate(cstate) {}
   ~CacheState() {}

   // These functions are only called in a private cache
   bool readable()
   {
      return (_cstate == MODIFIED) || (_cstate == OWNED) || (_cstate == SHARED);
   }
   
   bool writable()
   {
      return (_cstate == MODIFIED);
   }

private:
   Type _cstate;
};
