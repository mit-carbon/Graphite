#ifndef LOCKED_HASH_H
#define LOCKED_HASH_H

#include <map>
#include <utility>
#include <iostream>
#include <assert.h>
#include "fixed_types.h"
#include "pin.H"

class LockedHash 
{
   protected:
      typedef std::map<UInt64, UInt64> Bucket;

      UInt64 _size;
      Bucket *_bins;
      PIN_LOCK *_locks;
   public:
      LockedHash(UInt64 size);
      ~LockedHash();

      std::pair<bool, UInt64> find(UInt64 key); 
      bool insert(UInt64 key, UInt64 value);
};

#endif
