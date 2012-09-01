#pragma once

#include <string>
using std::string;

#include "fixed_types.h"

class CacheState
{
public:
   enum Type
   {
      INVALID = 0,
      SHARED,
      OWNED,
      EXCLUSIVE,
      MODIFIED,
      DATA_INVALID,
      CLEAN,
      DIRTY,
      NUM_STATES
   };

   CacheState(Type cstate = INVALID) : _cstate(cstate) {}
   ~CacheState() {}

   // readable() and writable() functions are only called in a private cache
   bool readable()
   {
      return (_cstate == MODIFIED) || (_cstate == EXCLUSIVE) || (_cstate == OWNED) || (_cstate == SHARED);
   }
   bool writable()
   {
      return (_cstate == MODIFIED) || (_cstate == EXCLUSIVE);
   }

   bool dirty()
   {
      return (_cstate == MODIFIED) || (_cstate == OWNED) || (_cstate == DIRTY);
   }

   static string getName(Type type);

private:
   Type _cstate;
};

#define SPELL_CSTATE(x)       (CacheState::getName(x).c_str())
