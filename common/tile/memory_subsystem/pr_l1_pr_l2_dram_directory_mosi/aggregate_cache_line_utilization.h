#pragma once

#include "cache_line_utilization.h"

namespace PrL1PrL2DramDirectoryMOSI
{

class AggregateCacheLineUtilization
{
public:
   AggregateCacheLineUtilization() {}
   AggregateCacheLineUtilization(const AggregateCacheLineUtilization& util)
      : L1_I(util.L1_I), L1_D(util.L1_D), L2(util.L2) {}
   ~AggregateCacheLineUtilization() {}

   CacheLineUtilization L1_I;
   CacheLineUtilization L1_D;
   CacheLineUtilization L2;
  
   UInt64 total()
   {
      return L1_I.total() + L1_D.total() + L2.total();
   }

   void operator+=(const AggregateCacheLineUtilization& util)
   {
      L1_I += util.L1_I;
      L1_D += util.L1_D;
      L2 += util.L2;
   }

   AggregateCacheLineUtilization operator/(UInt64 divisor)
   {
      AggregateCacheLineUtilization res;
      res.L1_I = L1_I / divisor;
      res.L1_D = L1_D / divisor;
      res.L2 = L2 / divisor;
      return res;
   }

   AggregateCacheLineUtilization operator*(UInt64 multiplier)
   {
      AggregateCacheLineUtilization res;
      res.L1_I = L1_I * multiplier;
      res.L1_D = L1_D * multiplier;
      res.L2 = L2 * multiplier;
      return res;
   }
};

}
