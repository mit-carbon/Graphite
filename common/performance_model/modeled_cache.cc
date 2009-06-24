#include <algorithm>

#include "modeled_cache.h"

#include "utils.h"
#include "log.h"

ModeledCache::ModeledCache(UInt32 lineSize,
                           UInt32 numSets,
                           UInt32 numWays,
                           UInt32 victimCacheSize,
                           ReplacementPolicy replacementPolicy)
   : m_line_size_base_2(floorLog2(lineSize))
   , m_num_sets_base_2(floorLog2(numSets))
   , m_sets()
   , m_victims(victimCacheSize)
   , m_next_victim_victim(0)
   , m_policy(replacementPolicy)
   , m_random_counter(0)
   , m_size(lineSize * numSets * numWays)
{
   m_sets.resize(numSets);
   for (UInt32 i = 0; i < m_sets.size(); i++)
   {
      m_sets[i].resize(numWays);
   }

   assert(isPower2(lineSize));
   assert(isPower2(numSets));
}

ModeledCache::~ModeledCache()
{
}

UInt32 ModeledCache::getSize()
{
   return m_size;
}

bool ModeledCache::access(IntPtr addr)
{
   UInt32 index;
   IntPtr tag;
   splitAddress(addr, index, tag);

   LOG_ASSERT_ERROR(index < m_sets.size(), "Bad index: %d >= %d", index, m_sets.size());

   // check main cache
   Set &set = m_sets[index];
   Set::iterator set_itr = std::find(set.begin(), set.end(), tag);
   if (set_itr != set.end())
   {
      update(set, set_itr - set.begin(), tag);
      return true;
   }

   // check victim cache
   set_itr = std::find(m_victims.begin(), m_victims.end(), tag);
   if (set_itr != m_victims.end())
   {
      swap(set, set_itr - m_victims.begin(), tag);
      return true;
   }

   // not found, add to set
   insert(set, tag);
   return false;
}

void ModeledCache::splitAddress(IntPtr addr, UInt32 &index, IntPtr &tag)
{
   index = (addr >> m_line_size_base_2) & ((1 << m_num_sets_base_2) - 1);
   tag = (addr >> (m_line_size_base_2 + m_num_sets_base_2));
}

// called when we hit in the main cache
void ModeledCache::update(Set &s, UInt32 way, IntPtr tag)
{
   switch (m_policy)
   {
   case RANDOM:
   {
      // do nothing
   } break;

   case LRU:
   {
      // move to front of set
      for (UInt32 i = way; way > 0; way--)
      {
         s[i] = s[i-1];
      }
      s[0] = tag;
   } break;

   default:
      LOG_PRINT_ERROR("No replacement policy?");
   }
}

// called when we hit in the victim cache
void ModeledCache::swap(Set &s, UInt32 victim_way, IntPtr tag)
{
   switch (m_policy)
   {
   case RANDOM:
   {
      UInt32 way = (m_random_counter++) % s.size();
      m_victims[victim_way] = s[way];
      s[way] = tag;
   } break;

   case LRU:
   {
      m_victims[victim_way] = s[s.size()-1];
      for (UInt32 i = s.size()-1; i > 0; i--)
      {
         s[i] = s[i-1];
      }
      s[0] = tag;
   } break;

   default:
      LOG_PRINT_ERROR("No replacement policy?");
   }
}

// called when we miss
void ModeledCache::insert(Set &s, IntPtr tag)
{
   switch (m_policy)
   {
   case RANDOM:
   {
      UInt32 way = (m_random_counter++) % s.size();
      m_victims[++m_next_victim_victim % m_victims.size()] = s[way];
      s[way] = tag;
   } break;

   case LRU:
   {
      m_victims[++m_next_victim_victim % m_victims.size()] = s[s.size()-1];
      for (UInt32 i = s.size()-1; i > 0; i--)
      {
         s[i] = s[i-1];
      }
      s[0] = tag;
   } break;

   default:
      LOG_PRINT_ERROR("No replacement policy?");
   }
}
