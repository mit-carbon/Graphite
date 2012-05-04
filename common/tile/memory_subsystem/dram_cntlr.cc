#include "dram_cntlr.h"
#include "tile.h"
#include "memory_manager.h"
#include "clock_converter.h"
#include "log.h"

DramCntlr::DramCntlr(Tile* tile,
      float dram_access_cost,
      float dram_bandwidth,
      bool dram_queue_model_enabled,
      string dram_queue_model_type,
      UInt32 cache_line_size)
   : _tile(tile)
   , _cache_line_size(cache_line_size)
{
   _dram_perf_model = new DramPerfModel(dram_access_cost, 
                                        dram_bandwidth,
                                        dram_queue_model_enabled,
                                        dram_queue_model_type,
                                        cache_line_size);

   _dram_access_count = new AccessCountMap[NUM_ACCESS_TYPES];
}

DramCntlr::~DramCntlr()
{
   printDramAccessCount();
   delete [] _dram_access_count;

   delete _dram_perf_model;
}

void
DramCntlr::getDataFromDram(IntPtr address, Byte* data_buf, bool modeled)
{
   if (_data_map[address] == NULL)
   {
      LOG_PRINT("Creating new array(%#lx)", address)
      _data_map[address] = new Byte[_cache_line_size];
      memset((void*) _data_map[address], 0x00, _cache_line_size);
      LOG_PRINT("Created new array(%p)", _data_map[address])
   }
   memcpy((void*) data_buf, (void*) _data_map[address], _cache_line_size);

   UInt64 dram_access_latency = modeled ? runDramPerfModel() : 0;
   LOG_PRINT("Dram Access Latency(%llu)", dram_access_latency);
   getShmemPerfModel()->incrCycleCount(dram_access_latency);

   addToDramAccessCount(address, READ);
}

void
DramCntlr::putDataToDram(IntPtr address, Byte* data_buf, bool modeled)
{
   if (_data_map[address] == NULL)
   {
      LOG_PRINT_ERROR("Data Buffer does not exist");
   }
   memcpy((void*) _data_map[address], (void*) data_buf, _cache_line_size);

   __attribute__((__unused__)) UInt64 dram_access_latency = modeled ? runDramPerfModel() : 0;
   
   addToDramAccessCount(address, WRITE);
}

UInt64
DramCntlr::runDramPerfModel()
{
   UInt64 pkt_cycle_count = getShmemPerfModel()->getCycleCount();
   UInt64 pkt_size = (UInt64) _cache_line_size;

   volatile float tile_frequency = _tile->getCore()->getPerformanceModel()->getFrequency();
   UInt64 pkt_time = convertCycleCount(pkt_cycle_count, tile_frequency, 1.0);

   UInt64 dram_access_latency = _dram_perf_model->getAccessLatency(pkt_time, pkt_size);
   
   return convertCycleCount(dram_access_latency, 1.0, tile_frequency);
}

void
DramCntlr::addToDramAccessCount(IntPtr address, AccessType access_type)
{
   _dram_access_count[access_type][address] = _dram_access_count[access_type][address] + 1;
}

void
DramCntlr::printDramAccessCount()
{
   for (UInt32 k = 0; k < NUM_ACCESS_TYPES; k++)
   {
      for (AccessCountMap::iterator i = _dram_access_count[k].begin(); i != _dram_access_count[k].end(); i++)
      {
         if ((*i).second > 100)
         {
            LOG_PRINT("Dram Cntlr(%i), Address(0x%x), Access Count(%llu), Access Type(%s)", 
                  _tile->getId(), (*i).first, (*i).second,
                  (k == READ)? "READ" : "WRITE");
         }
      }
   }
}

ShmemPerfModel*
DramCntlr::getShmemPerfModel()
{
   return _tile->getMemoryManager()->getShmemPerfModel();
}
