#pragma once

#include "fixed_types.h"

namespace PrL1PrL2DramDirectoryMOSI
{

class AggregateCacheLineLifetime
{
public:
   AggregateCacheLineLifetime()
      : L1_I(0), L1_D(0), L2(0) {}
   AggregateCacheLineLifetime(const AggregateCacheLineLifetime& lifetime)
      : L1_I(lifetime.L1_I), L1_D(lifetime.L1_D), L2(lifetime.L2) {}
   ~AggregateCacheLineLifetime() {}

   UInt64 L1_I;
   UInt64 L1_D;
   UInt64 L2;
  
   void operator+=(const AggregateCacheLineLifetime& lifetime)
   {
      L1_I += lifetime.L1_I;
      L1_D += lifetime.L1_D;
      L2 += lifetime.L2;
   }
};

}
