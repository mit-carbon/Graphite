#pragma once

using namespace std;

#include "tile.h"
#include "core.h"
#include "cache.h"
#include "network.h"
#include "mem_component.h"
#include "shmem_perf_model.h"

void MemoryManagerNetworkCallback(void* obj, NetPacket packet);

class MemoryManager
{
public:
   enum CachingProtocol_t
   {
      PR_L1_PR_L2_DRAM_DIRECTORY_MSI = 0,
      PR_L1_PR_L2_DRAM_DIRECTORY_MOSI,
      SH_L1_SH_L2,
      NUM_CACHING_PROTOCOL_TYPES
   };

   MemoryManager(Tile* tile, Network* network, ShmemPerfModel* shmem_perf_model);
   virtual ~MemoryManager();

   virtual bool coreInitiateMemoryAccess(
         MemComponent::component_t mem_component,
         Core::lock_signal_t lock_signal,
         Core::mem_op_t mem_op_type,
         IntPtr address, UInt32 offset,
         Byte* data_buf, UInt32 data_length,
         bool modeled) = 0;

   virtual void handleMsgFromNetwork(NetPacket& packet) = 0;

   Tile* getTile() { return _tile; }
   virtual UInt32 getCacheLineSize() = 0;
   ShmemPerfModel* getShmemPerfModel() { return _shmem_perf_model; }

   virtual tile_id_t getShmemRequester(const void* pkt_data) = 0;

   virtual void enableModels() = 0;
   virtual void disableModels() = 0;

   // Modeling
   virtual UInt32 getModeledLength(const void* pkt_data) = 0;
   virtual bool isModeled(const void* pkt_data) = 0;

   static CachingProtocol_t parseProtocolType(std::string& protocol_type);
   static MemoryManager* createMMU(std::string protocol_type,
                                       Tile* tile, Network* network,
                                       ShmemPerfModel* shmem_perf_model);
   
   virtual void outputSummary(std::ostream& os) = 0;

   // Cache line replication trace
   static void openCacheLineReplicationTraceFiles();
   static void closeCacheLineReplicationTraceFiles();
   static void outputCacheLineReplicationSummary();

   // Modeling of different miss types
   static bool isMissTypeModeled(Cache::MissType cache_miss_type);

protected:
   Network* getNetwork() { return _network; }

   vector<tile_id_t> getTileListWithMemoryControllers(void);
   void printTileListWithMemoryControllers(vector<tile_id_t>& tile_list_with_memory_controllers);

private:
   static CachingProtocol_t _caching_protocol_type;
   Tile* _tile;
   Network* _network;
   ShmemPerfModel* _shmem_perf_model;
   
   void parseMemoryControllerList(string& memory_controller_positions,
                                  vector<tile_id_t>& tile_list_from_cfg_file,
                                  SInt32 application_tile_count);

   // Handling of different miss types
   static bool _miss_type_modeled[Cache::NUM_MISS_TYPES];

   void initializeModeledMissTypes();
};
