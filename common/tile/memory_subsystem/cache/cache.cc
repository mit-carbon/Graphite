#include "simulator.h"
#include "cache.h"
#include "cache_set.h"
#include "cache_line_info.h"
#include "utils.h"
#include "log.h"

using namespace std;

// Cache class
// constructors/destructors
Cache::Cache(string name,
             UInt32 cache_size,
             UInt32 associativity,
             UInt32 cache_line_size,
             string replacement_policy,
             Type cache_type,
             UInt32 access_delay,
             volatile float frequency,
             bool track_miss_types)
   : _enabled(false)
   , _name(name)
   , _cache_size(k_KILO * cache_size)
   , _associativity(associativity)
   , _line_size(cache_line_size)
   , _cache_type(cache_type)
   , _power_model(NULL)
   , _area_model(NULL)
   , _track_miss_types(track_miss_types)
{
   _num_sets = _cache_size / (_associativity * _line_size);
   _log_line_size = floorLog2(_line_size);
   
   _sets = new CacheSet*[_num_sets];
   ReplacementPolicy policy = parseReplacementPolicy(replacement_policy);
   for (UInt32 i = 0; i < _num_sets; i++)
   {
      _sets[i] = CacheSet::createCacheSet(policy, _cache_type, _associativity, _line_size);
   }

   if (Config::getSingleton()->getEnablePowerModeling())
   {
      _power_model = new CachePowerModel("data", k_KILO * cache_size, cache_line_size,
            associativity, access_delay, frequency);
   }
   if (Config::getSingleton()->getEnableAreaModeling())
   {
      _area_model = new CacheAreaModel("data", k_KILO * cache_size, cache_line_size,
            associativity, access_delay, frequency);
   }

   // Initialize Cache Counters
   // Hit/miss counters
   initializeMissCounters();
   // Tracking tag/data read/writes
   initializeTagAndDataArrayCounters();
   // Cache line state counters
   initializeCacheLineStateCounters();
}

Cache::~Cache()
{
   for (SInt32 i = 0; i < (SInt32) _num_sets; i++)
      delete _sets[i];
   delete [] _sets;
}

void
Cache::accessCacheLine(IntPtr address, AccessType access_type, Byte* buf, UInt32 num_bytes)
{
   assert((buf == NULL) == (num_bytes == 0));

   CacheSet* set = getSet(address);
   IntPtr tag = getTag(address);
   UInt32 line_offset = getLineOffset(address);
   UInt32 line_index = -1;
   
   CacheLineInfo* cache_line_info = set->find(tag, &line_index);
   LOG_ASSERT_ERROR(cache_line_info, "Address(%#llx)", address);

   if (access_type == LOAD)
      set->read_line(line_index, line_offset, buf, num_bytes);
   else
      set->write_line(line_index, line_offset, buf, num_bytes);

   if (_enabled)
   {
      // Update data array reads/writes
      if (access_type == LOAD)
         _data_array_reads ++;
      else
         _data_array_writes ++;

      // Update dynamic energy counters
      if (_power_model)
         _power_model->updateDynamicEnergy();
   }
}

void
Cache::insertCacheLine(IntPtr inserted_address, CacheLineInfo* inserted_cache_line_info, Byte* fill_buf,
                       bool* eviction, IntPtr* evicted_address, CacheLineInfo* evicted_cache_line_info, Byte* writeback_buf)
{
   CacheSet* set = getSet(inserted_address);

   // Write into the data array
   set->insert(inserted_cache_line_info, fill_buf, eviction, evicted_cache_line_info, writeback_buf);
  
   // Evicted address 
   *evicted_address = getAddressFromTag(evicted_cache_line_info->getTag());

   // Update Cache Line State Counters and address set for the evicted line
   if (*eviction)
   {
      // Add to evicted set for tracking miss type
      assert(*evicted_address != INVALID_ADDRESS);
      
      if (_track_miss_types)
         _evicted_address_set.insert(*evicted_address);

      // Update exclusive/sharing counters
      updateCacheLineStateCounters(evicted_cache_line_info->getCState(), CacheState::INVALID);
   }

   // Clear the miss type tracking sets for this address
   if (_track_miss_types)
      clearMissTypeTrackingSets(inserted_address);

   // Add to fetched set for tracking miss type
   if (_track_miss_types)
      _fetched_address_set.insert(inserted_address);

   // Update exclusive/sharing counters
   updateCacheLineStateCounters(CacheState::INVALID, inserted_cache_line_info->getCState());

   if (_enabled)
   {
      if (*eviction)
      {
         assert(evicted_cache_line_info->getCState() != CacheState::INVALID);

         // Update tag/data array reads
         _tag_array_reads ++;
         _data_array_reads ++;
      }
      else // (! (*eviction))
      {
         assert(evicted_cache_line_info->getCState() == CacheState::INVALID);
         
         // Update tag array reads
         _tag_array_reads ++;
      }

      // Update tag/data array writes
      _tag_array_writes ++;
      _data_array_writes ++;

      // Update dynamic energy counters
      if (_power_model)
         _power_model->updateDynamicEnergy();
   }
}


// Single line cache access at address
void
Cache::getCacheLineInfo(IntPtr address, CacheLineInfo* cache_line_info)
{
   LOG_PRINT("getCacheLineInfo[Address(%#llx), Cache Line Info Ptr(%p)] start", address, cache_line_info);

   CacheSet* set = getSet(address);
   IntPtr tag = getTag(address);

   CacheLineInfo* line_info = set->find(tag);

   LOG_PRINT("Set Ptr(%p), Tag(%#llx), Line Info Ptr(%p)", set, tag, line_info);

   // Assign it to the second argument in the function (copies it over) 
   if (line_info)
      cache_line_info->assign(line_info);

   LOG_PRINT("Assigned Line Info");
   
   if (_enabled)
   {
      // Update tag/data array reads/writes
      _tag_array_reads ++;
      // Update dynamic energy counters
      if (_power_model)
         _power_model->updateDynamicEnergy();
   }

   LOG_PRINT("getCacheLineInfo[Address(%#llx), Cache Line Info Ptr(%p)] end", address, cache_line_info);
}

void
Cache::setCacheLineInfo(IntPtr address, CacheLineInfo* updated_cache_line_info)
{
   CacheSet* set = getSet(address);
   IntPtr tag = getTag(address);

   CacheLineInfo* cache_line_info = set->find(tag);
   assert(cache_line_info);

   // Update exclusive/shared counters
   updateCacheLineStateCounters(cache_line_info->getCState(), updated_cache_line_info->getCState());
   
   // Update the cache line info   
   cache_line_info->assign(updated_cache_line_info);
   
   if (_enabled)
   {
      // Update tag/data array reads/writes
      _tag_array_writes ++;
      // Update dynamic energy counters
      if (_power_model)
         _power_model->updateDynamicEnergy();
   }
}

bool 
Cache::invalidateCacheLine(IntPtr address)
{
   CacheSet* set = getSet(address);
   IntPtr tag = getTag(address);

   CacheLineInfo* cache_line_info = set->find(tag);

   if (cache_line_info)
   {
      // Update exclusive/sharing counters
      updateCacheLineStateCounters(cache_line_info->getCState(), CacheState::INVALID);
     
      // Invalidate cache line 
      cache_line_info->invalidate();
      
      // Add to invalidated set for tracking miss type
      if (_track_miss_types)
         _invalidated_address_set.insert(address);
      
      if (_enabled)
      {
         // Update tag array writes
         _tag_array_writes ++;
         // Update dynamic energy counters
         if (_power_model)
            _power_model->updateDynamicEnergy();
      }

      return true;
   }
   else
   {
      return false;
   }
}

void
Cache::initializeMissCounters()
{
   _total_cache_accesses = 0;
   _total_cache_misses = 0;

   if (_track_miss_types)
      initializeMissTypeCounters();
}

void
Cache::initializeMissTypeCounters()
{
   _total_cold_misses = 0;
   _total_capacity_misses = 0;
   _total_upgrade_misses = 0;
   _total_sharing_misses = 0;
}

void
Cache::initializeTagAndDataArrayCounters()
{
   _tag_array_reads = 0;
   _tag_array_writes = 0;
   _data_array_reads = 0;
   _data_array_writes = 0;
}

void
Cache::initializeCacheLineStateCounters()
{
   _total_exclusive_lines = 0;
   _total_shared_lines = 0;
}

Cache::MissType
Cache::updateMissCounters(IntPtr address, bool cache_miss)
{
   MissType miss_type = INVALID_MISS_TYPE;
   
   if (_enabled)
   {
      _total_cache_accesses ++;
      if (cache_miss)
      {
         _total_cache_misses ++;
         // Compute the miss type counters for the inserted line
         if (_track_miss_types)
         {
            miss_type = getMissType(address);
            updateMissTypeCounters(address, miss_type);
         }
      }
   }
  
   return miss_type;
}

Cache::MissType
Cache::getMissType(IntPtr address) const
{
   // We maintain three address sets to keep track of miss types
   if (_evicted_address_set.find(address) != _evicted_address_set.end())
      return CAPACITY_MISS;
   else if (_invalidated_address_set.find(address) != _invalidated_address_set.end())
      return SHARING_MISS;
   else if (_fetched_address_set.find(address) != _fetched_address_set.end())
      return UPGRADE_MISS;
   else
      return COLD_MISS;
}

void
Cache::updateMissTypeCounters(IntPtr address, MissType miss_type)
{
   switch (miss_type)
   {
   case COLD_MISS:
      _total_cold_misses ++;
      break;

   case CAPACITY_MISS:
      _total_capacity_misses ++;
      break;

   case UPGRADE_MISS:
      _total_upgrade_misses ++;
      break;

   case SHARING_MISS:
      _total_sharing_misses ++;
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized Cache Miss Type(%i)", miss_type);
      break;
   }
}

void
Cache::clearMissTypeTrackingSets(IntPtr address)
{
   if (_evicted_address_set.erase(address));
   else if (_invalidated_address_set.erase(address));
   else if (_fetched_address_set.erase(address));
}

void
Cache::updateCacheLineStateCounters(CacheState::CState old_cstate, CacheState::CState new_cstate)
{
   if (old_cstate == CacheState::MODIFIED)
      _total_exclusive_lines --;
   else if ((old_cstate == CacheState::OWNED) || (old_cstate == CacheState::SHARED))
      _total_shared_lines --;

   if (new_cstate == CacheState::MODIFIED)
      _total_exclusive_lines ++;
   else if ((new_cstate == CacheState::OWNED) || (new_cstate == CacheState::SHARED))
      _total_shared_lines ++;
}

void
Cache::getCacheLineStateCounters(UInt64& total_exclusive_lines, UInt64& total_shared_lines)
{
   total_exclusive_lines = _total_exclusive_lines;
   total_shared_lines = _total_shared_lines;
}

void
Cache::outputSummary(ostream& out)
{
   // Cache Miss Summary
   out << "  Cache " << _name << ":\n";
   out << "    Total Cache Accesses: " << _total_cache_accesses << endl;
   out << "    Total Cache Misses: " << _total_cache_misses << endl;
   out << "    Miss Rate: " <<
      ((float) _total_cache_misses) * 100 / _total_cache_accesses << endl;

   if (_track_miss_types)
   {
      out << "    Total Cold Misses: " << _total_cold_misses << endl;
      out << "    Total Capacity Misses: " << _total_capacity_misses << endl;
      out << "    Total Upgrade Misses: " << _total_upgrade_misses << endl;
      out << "    Total Sharing Misses: " << _total_sharing_misses << endl;
   }

   // Event Counters Summary
   out << "   Event Counters:" << endl;
   out << "    Tag Array Reads: " << _tag_array_reads << endl;
   out << "    Tag Array Writes: " << _tag_array_writes << endl;
   out << "    Data Array Reads: " << _data_array_reads << endl;
   out << "    Data Array Writes: " << _data_array_writes << endl;
   
   // Output Power and Area Summaries
   if (_power_model)
      _power_model->outputSummary(out);
   if (_area_model)
      _area_model->outputSummary(out);
}

// Utilities
IntPtr
Cache::getTag(IntPtr address) const
{
   return (address >> _log_line_size);
}

CacheSet*
Cache::getSet(IntPtr address) const
{
   UInt32 set_index = (address >> _log_line_size) & (_num_sets-1);
   return _sets[set_index];
}

UInt32
Cache::getLineOffset(IntPtr address) const
{
   return (address & (_line_size-1));
}

IntPtr
Cache::getAddressFromTag(IntPtr tag) const
{
   return tag << _log_line_size;
}

Cache::ReplacementPolicy
Cache::parseReplacementPolicy(string policy)
{
   if (policy == "round_robin")
      return ROUND_ROBIN;
   if (policy == "lru")
      return LRU;
   else
   {
      LOG_PRINT_ERROR("Unrecognized Cache Replacement Policy(%s) for (%s)", policy.c_str(), _name.c_str());
      return NUM_REPLACEMENT_POLICIES;
   }
}

Cache::MissType
Cache::parseMissType(string miss_type)
{
   if (miss_type == "cold")
      return COLD_MISS;
   else if (miss_type == "capacity")
      return CAPACITY_MISS;
   else if (miss_type == "upgrade")
      return UPGRADE_MISS;
   else if (miss_type == "sharing")
      return SHARING_MISS;
   else
   {
      LOG_PRINT_ERROR("Unrecognized Miss Type(%s)", miss_type.c_str());
      return INVALID_MISS_TYPE;
   }
}
