#pragma once

#include <map>
using std::map;

#include "tile.h"
#include "dram_perf_model.h"
#include "shmem_perf_model.h"
#include "fixed_types.h"
#include "time_types.h"

class DramCntlr
{
public:
   enum AccessType
   {
      READ = 0,
      WRITE,
      NUM_ACCESS_TYPES
   };

   DramCntlr(Tile* tile,
             float dram_access_cost,
             float dram_bandwidth,
             bool dram_queue_model_enabled,
             string dram_queue_model_type,
             UInt32 cache_line_size);
   ~DramCntlr();

   DramPerfModel* getDramPerfModel() { return _dram_perf_model; }

   void getDataFromDram(IntPtr address, Byte* data_buf, bool modeled);
   void putDataToDram(IntPtr address, Byte* data_buf, bool modeled);
   
private:
   Tile* _tile;
   map<IntPtr, Byte*> _data_map;
   DramPerfModel* _dram_perf_model;

   typedef std::map<IntPtr,UInt64> AccessCountMap;
   AccessCountMap* _dram_access_count;

   ShmemPerfModel* getShmemPerfModel();
   Latency runDramPerfModel();

   void addToDramAccessCount(IntPtr address, AccessType access_type);
   void printDramAccessCount();

protected:
   UInt32 _cache_line_size;
};
