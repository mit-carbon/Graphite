#ifndef CACHE_STATE_H
#define CACHE_STATE_H

#include <string>
#include <cassert>

#include "fixed_types.h"

using namespace std;

class CacheState
{
   public:
      enum cstate_t
      {
         INVALID = 0,
         SHARED,
         OWNED,
         MODIFIED,
         NUM_CSTATE_STATES
      };

      CacheState(cstate_t state = INVALID) : cstate(state) {}
      ~CacheState() {}

      bool readable()
      {
         return (cstate == MODIFIED) || (cstate == SHARED);
      }
      
      bool writable()
      {
         return (cstate == MODIFIED);
      }

   private:
      cstate_t cstate;

};

#endif /* CACHE_STATE_H */
