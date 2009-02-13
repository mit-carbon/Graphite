#ifndef CACHE_STATE_H
#define CACHE_STATE_H

#include <string>
#include <iostream>
using namespace std;

class CacheState
{
   public:
      enum cstate_t
      {
         INVALID,
         EXCLUSIVE,
         SHARED,
         NUM_CSTATE_STATES
      };

      CacheState(cstate_t state = INVALID) : cstate(state) {}
      ~CacheState() {}

      bool readable()
      {
         return (cstate == EXCLUSIVE) || (cstate == SHARED);
      }
      bool writable()
      {
         return (cstate == EXCLUSIVE);
      }

      string toString()
      {
         switch (cstate)
         {
         case INVALID:
            return   "INVALID  ";
         case EXCLUSIVE:
            return "EXCLUSIVE";
         case SHARED:
            return    "SHARED   ";
         default:
            return "ERROR: BAD CACHE STATE";
         }
         return "ERROR: BAD CACHE STATE out of default";
      }

      static string cStateToString(cstate_t state)
      {
         switch (state)
         {
         case INVALID:
            return   "INVALID  ";
         case EXCLUSIVE:
            return "EXCLUSIVE";
         case SHARED:
            return    "SHARED   ";
         default:
            return "ERROR: BAD CACHE STATE";
         }
         return "ERROR: SOMETHING BAD CACHE STATE";
      }

   private:
      cstate_t cstate;

};

#endif
