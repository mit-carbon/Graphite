#ifndef MODELED_CACHE_H
#define MODELED_CACHE_H

#include <vector>
#include "fixed_types.h"

class ModeledCache
{
public:
   enum ReplacementPolicy
   {
      RANDOM,
      LRU
   };

   ModeledCache(UInt32 lineSize,
                UInt32 numSets,
                UInt32 numWays,
                UInt32 victimCacheSize,
                ReplacementPolicy replacementPolicy);
   ~ModeledCache();

   bool access(IntPtr addr);

   UInt32 getSize();

private:

   void splitAddress(IntPtr addr, UInt32 &index, IntPtr &tag);

   typedef std::vector<IntPtr> Set;
   typedef std::vector<Set> SetVector;

   void update(Set &s, UInt32 way, IntPtr tag);
   void swap(Set &s, UInt32 victim_way, IntPtr tag);
   void insert(Set &s, IntPtr tag);

   UInt32 m_line_size_base_2;
   UInt32 m_num_sets_base_2;
   SetVector m_sets;
   Set m_victims;
   UInt32 m_next_victim_victim;

   ReplacementPolicy m_policy;

   UInt32 m_random_counter;

   UInt32 m_size;
};

#endif
