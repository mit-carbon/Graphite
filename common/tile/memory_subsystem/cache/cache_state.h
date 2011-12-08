#pragma once

#include <string>
#include <cassert>

#include "fixed_types.h"

using namespace std;

class CacheState
{
public:
   enum CState
   {
      INVALID = 0,
      SHARED,
      OWNED,
      MODIFIED,
      NUM_CSTATE_STATES
   };

   CacheState(CState cstate = INVALID) : _cstate(cstate) {}
   ~CacheState() {}

   bool readable()
   {
      return (_cstate == MODIFIED) || (_cstate == OWNED) || (_cstate == SHARED);
   }
   
   bool writable()
   {
      return (_cstate == MODIFIED);
   }

private:
   CState _cstate;
};
