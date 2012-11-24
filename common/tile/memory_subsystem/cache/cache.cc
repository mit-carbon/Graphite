#include "simulator.h"
#include "cache.h"
#include "cache_set.h"
#include "cache_line_info.h"
#include "cache_replacement_policy.h"
#include "cache_hash_fn.h"
#include "utils.h"
#include "log.h"

// Cache class
// constructors/destructors
Cache::Cache(string name,
             CachingProtocolType caching_protocol_type,
             CacheCategory cache_category,
             SInt32 cache_level,
             WritePolicy write_policy,
             UInt32 cache_size,
             UInt32 associativity,
             UInt32 line_size,
             CacheReplacementPolicy* replacement_policy,
             CacheHashFn* hash_fn,
             UInt32 access_delay,
             float frequency,
             bool track_miss_types)
   : _enabled(false)
   , _name(name)
   , _cache_category(cache_category)
   , _write_policy(write_policy)
   , _cache_size(k_KILO * cache_size)
   , _associativity(associativity)
   , _line_size(line_size)
   , _replacement_policy(replacement_policy)
   , _hash_fn(hash_fn)
   , _power_model(NULL)
   , _area_model(NULL)
   , _track_miss_types(track_miss_types)
{
   _num_sets = _cache_size / (_associativity * _line_size);
   _log_line_size = floorLog2(_line_size);
   
   _sets = new CacheSet*[_num_sets];
   for (UInt32 i = 0; i < _num_sets; i++)
   {
      _sets[i] = new CacheSet(i, caching_protocol_type, cache_level, _replacement_policy, _associativity, _line_size);
   }

   if (Config::getSingleton()->getEnablePowerModeling())
   {
      _power_model = new CachePowerModel("data", _cache_size, _line_size,
            associativity, access_delay, frequency);
   }
   if (Config::getSingleton()->getEnableAreaModeling())
   {
      _area_model = new CacheAreaModel("data", _cache_size, _line_size,
            associativity, access_delay, frequency);
   }

   // Initialize Cache Counters
   // Hit/miss counters
   initializeMissCounters();
   // Initialize eviction counters
   initializeEvictionCounters();
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
   LOG_PRINT("accessCacheLine: Address(%#lx), AccessType(%s), Num Bytes(%u) start",
             address, (access_type == 0) ? "LOAD": "STORE", num_bytes);
   assert((buf == NULL) == (num_bytes == 0));

   CacheSet* set = getSet(address);
   IntPtr tag = getTag(address);
   UInt32 line_offset = getLineOffset(address);
   UInt32 line_index = -1;
  
   // FIXME: This is an expensive operation. Remove if possible 
   __attribute(__unused__) CacheLineInfo* cache_line_info = set->find(tag, &line_index);
   LOG_ASSERT_ERROR(cache_line_info, "Address(%#lx)", address);

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
   LOG_PRINT("accessCacheLine: Address(%#lx), AccessType(%s), Num Bytes(%u) end",
             address, (access_type == 0) ? "LOAD": "STORE", num_bytes);
}

void
Cache::insertCacheLine(IntPtr inserted_address, CacheLineInfo* inserted_cache_line_info, Byte* fill_buf,
                       bool* eviction, IntPtr* evicted_address, CacheLineInfo* evicted_cache_line_info, Byte* writeback_buf)
{
   LOG_PRINT("insertCacheLine: Address(%#lx) start", inserted_address);

   CacheSet* set = getSet(inserted_address);

   // Write into the data array
   set->insert(inserted_cache_line_info, fill_buf,
               eviction, evicted_cache_line_info, writeback_buf);
  
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
         // Read data array only if there is an eviction
         _tag_array_reads ++;
         _data_array_reads ++;
         
         // Increment number of evictions and dirty evictions
         _total_evictions ++;
         // Update number of dirty evictions
         if ( (_write_policy == WRITE_BACK) && (CacheState(evicted_cache_line_info->getCState()).dirty()) )
            _total_dirty_evictions ++;
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
   
   LOG_PRINT("insertCacheLine: Address(%#lx) end", inserted_address);
}

// Single line cache access at address
void
Cache::getCacheLineInfo(IntPtr address, CacheLineInfo* cache_line_info)
{
   LOG_PRINT("getCacheLineInfo: Address(%#lx) start", address);

   CacheLineInfo* line_info = getCacheLineInfo(address);

   // Assign it to the second argument in the function (copies it over) 
   if (line_info)
      cache_line_info->assign(line_info);

   if (_enabled)
   {
      // Update tag/data array reads/writes
      _tag_array_reads ++;
      // Update dynamic energy counters
      if (_power_model)
         _power_model->updateDynamicEnergy();
   }

   LOG_PRINT("getCacheLineInfo: Address(%#lx) end", address);
}

CacheLineInfo*
Cache::getCacheLineInfo(IntPtr address)
{
   CacheSet* set = getSet(address);
   IntPtr tag = getTag(address);

   CacheLineInfo* line_info = set->find(tag);

   return line_info;
}

void
Cache::setCacheLineInfo(IntPtr address, CacheLineInfo* updated_cache_line_info)
{
   LOG_PRINT("setCacheLineInfo: Address(%#lx) start", address);
   CacheLineInfo* cache_line_info = getCacheLineInfo(address);
   LOG_ASSERT_ERROR(cache_line_info, "Address(%#lx)", address);

   // Update exclusive/shared counters
   updateCacheLineStateCounters(cache_line_info->getCState(), updated_cache_line_info->getCState());
  
   // Update _invalidated_address_set
   if ( (updated_cache_line_info->getCState() == CacheState::INVALID) && (_track_miss_types) )
      _invalidated_address_set.insert(address);

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
   LOG_PRINT("setCacheLineInfo: Address(%#lx) end", address);
}

void
Cache::initializeMissCounters()
{
   _total_cache_accesses = 0;
   _total_cache_misses = 0;
   _total_read_accesses = 0;
   _total_read_misses = 0;
   _total_write_accesses = 0;
   _total_write_misses = 0;

   if (_track_miss_types)
      initializeMissTypeCounters();
}

void
Cache::initializeMissTypeCounters()
{
   _total_cold_misses = 0;
   _total_capacity_misses = 0;
   _total_sharing_misses = 0;
}

void
Cache::initializeEvictionCounters()
{
   _total_evictions = 0;
   _total_dirty_evictions = 0;   
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
   _cache_line_state_counters.resize(CacheState::NUM_STATES, 0);
}

Cache::MissType
Cache::updateMissCounters(IntPtr address, Core::mem_op_t mem_op_type, bool cache_miss)
{
   MissType miss_type = INVALID_MISS_TYPE;
   
   if (_enabled)
   {
      _total_cache_accesses ++;

      // Read/Write access
      if ((mem_op_type == Core::READ) || (mem_op_type == Core::READ_EX))
      {
         _total_read_accesses ++;
      }
      else // (mem_op_type == Core::WRITE)
      {
         assert(_cache_category != INSTRUCTION_CACHE);
         _total_write_accesses ++;
      }

      if (cache_miss)
      {
         _total_cache_misses ++;
         
         // Read/Write miss
         if ((mem_op_type == Core::READ) || (mem_op_type == Core::READ_EX))
            _total_read_misses ++;
         else // (mem_op_type == Core::WRITE)
            _total_write_misses ++;
         
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
      return SHARING_MISS;
   else
      return COLD_MISS;
}

void
Cache::updateMissTypeCounters(IntPtr address, MissType miss_type)
{
   assert(_enabled);
   switch (miss_type)
   {
   case COLD_MISS:
      _total_cold_misses ++;
      break;
   case CAPACITY_MISS:
      _total_capacity_misses ++;
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
Cache::updateCacheLineStateCounters(CacheState::Type old_cstate, CacheState::Type new_cstate)
{
   _cache_line_state_counters[old_cstate] --;
   _cache_line_state_counters[new_cstate] ++;
}

void
Cache::getCacheLineStateCounters(vector<UInt64>& cache_line_state_counters) const
{
   cache_line_state_counters = _cache_line_state_counters;
}

void
Cache::outputSummary(ostream& out)
{
   // Cache Miss Summary
   out << "  Cache " << _name << ":\n";
   out << "    Cache Accesses: " << _total_cache_accesses << endl;
   out << "    Cache Misses: " << _total_cache_misses << endl;
   if (_total_cache_accesses > 0)
      out << "    Miss Rate (%): " << 100.0 * _total_cache_misses / _total_cache_accesses << endl;
   else
      out << "    Miss Rate (%): " << endl;
   
   if (_cache_category != INSTRUCTION_CACHE)
   {
      out << "      Read Accesses: " << _total_read_accesses << endl;
      out << "      Read Misses: " << _total_read_misses << endl;
      if (_total_read_accesses > 0)
         out << "      Read Miss Rate (%): " << 100.0 * _total_read_misses / _total_read_accesses << endl;
      else
         out << "      Read Miss Rate (%): " << endl;
      
      out << "      Write Accesses: " << _total_write_accesses << endl;
      out << "      Write Misses: " << _total_write_misses << endl;
      if (_total_write_accesses > 0)
         out << "      Write Miss Rate (%): " << 100.0 * _total_write_misses / _total_write_accesses << endl;
      else
         out << "    Write Miss Rate (%): " << endl;
   }

   // Evictions
   out << "    Evictions: " << _total_evictions << endl;
   if (_write_policy == WRITE_BACK)
   {
      out << "    Dirty Evictions: " << _total_dirty_evictions << endl;
   }
   
   // Output Power and Area Summaries
   if (_power_model)
      _power_model->outputSummary(out);
   if (_area_model)
      _area_model->outputSummary(out);

   // Track miss types
   if (_track_miss_types)
   {
      out << "    Miss Types:" << endl;
      out << "      Cold Misses: " << _total_cold_misses << endl;
      out << "      Capacity Misses: " << _total_capacity_misses << endl;
      out << "      Sharing Misses: " << _total_sharing_misses << endl;
   }

   // Cache Access Counters Summary
   out << "    Access Counters:" << endl;
   out << "      Tag Array Reads: " << _tag_array_reads << endl;
   out << "      Tag Array Writes: " << _tag_array_writes << endl;
   out << "      Data Array Reads: " << _data_array_reads << endl;
   out << "      Data Array Writes: " << _data_array_writes << endl;
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
   UInt32 set_num = _hash_fn->compute(address);
   return _sets[set_num];
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

Cache::MissType
Cache::parseMissType(string miss_type)
{
   if (miss_type == "cold")
      return COLD_MISS;
   else if (miss_type == "capacity")
      return CAPACITY_MISS;
   else if (miss_type == "sharing")
      return SHARING_MISS;
   else
   {
      LOG_PRINT_ERROR("Unrecognized Miss Type(%s)", miss_type.c_str());
      return INVALID_MISS_TYPE;
   }
}
