#pragma once

using namespace std;

#include "tile.h"
#include "core.h"
#include "cache.h"
#include "network.h"
#include "mem_component.h"
#include "caching_protocol_type.h"
#include "shmem_perf_model.h"

void MemoryManagerNetworkCallback(void* obj, NetPacket packet);

class MemoryManager
{
public:
   MemoryManager(Tile* tile);
   virtual ~MemoryManager();

   virtual bool coreInitiateMemoryAccess(MemComponent::Type mem_component,
                                         Core::lock_signal_t lock_signal,
                                         Core::mem_op_t mem_op_type,
                                         IntPtr address, UInt32 offset,
                                         Byte* data_buf, UInt32 data_length,
                                         UInt64& curr_time, bool modeled) = 0;

   virtual void handleMsgFromNetwork(NetPacket& packet) = 0;

   Tile* getTile()   { return _tile; }
   virtual UInt32 getCacheLineSize() = 0;
   ShmemPerfModel* getShmemPerfModel() { return _shmem_perf_model; }

   virtual tile_id_t getShmemRequester(const void* pkt_data) = 0;

   virtual void enableModels()   { _shmem_perf_model->enable();   }
   virtual void disableModels()  { _shmem_perf_model->disable();  }

   // Modeling
   // getModeledLength() returns the length of the msg in bits
   virtual UInt32 getModeledLength(const void* pkt_data) = 0;
   virtual bool isModeled(const void* pkt_data) = 0;

   static CachingProtocolType parseProtocolType(std::string& protocol_type);
   static MemoryManager* createMMU(std::string protocol_type, Tile* tile);
   
   virtual void outputSummary(std::ostream& os) = 0;

   virtual void computeEnergy() = 0;

   virtual double getDynamicEnergy() = 0;
   virtual double getStaticPower() = 0;

   // Cache line replication trace
   static void openCacheLineReplicationTraceFiles();
   static void closeCacheLineReplicationTraceFiles();
   static void outputCacheLineReplicationSummary();

protected:
   Network* getNetwork() { return _network; }

   vector<tile_id_t> getTileListWithMemoryControllers();
   void printTileListWithMemoryControllers(vector<tile_id_t>& tile_list_with_memory_controllers);

private:
   static CachingProtocolType _caching_protocol_type;
   Tile* _tile;
   Network* _network;
   ShmemPerfModel* _shmem_perf_model;
   
   void parseMemoryControllerList(string& memory_controller_positions,
                                  vector<tile_id_t>& tile_list_from_cfg_file,
                                  SInt32 application_tile_count);
};
