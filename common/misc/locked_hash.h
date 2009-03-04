#ifndef LOCKED_HASH_H
#define LOCKED_HASH_H

#include <map>
#include <utility>
#include <iostream>
#include <assert.h>
#include "fixed_types.h"
#include "lock.h"

class LockedHash
{
   protected:
      typedef std::map<UInt64, UInt64> Bucket;

      UInt64 _size;
      Bucket *_bins;
      Lock *_locks;
   public:
      LockedHash(UInt64 size);
      ~LockedHash();

      std::pair<bool, UInt64> find(UInt64 key);
      bool insert(UInt64 key, UInt64 value);
      void remove(UInt64 key);
};

#endif
