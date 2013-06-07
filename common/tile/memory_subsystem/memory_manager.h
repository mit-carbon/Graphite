#pragma once

using namespace std;

#include "tile.h"
#include "core.h"
#include "cache.h"
#include "directory_cache.h"
#include "network.h"
#include "mem_component.h"
#include "caching_protocol_type.h"
#include "shmem_perf_model.h"
#include "dvfs.h"

void MemoryManagerNetworkCallback(void* obj, NetPacket packet);

class MemoryManager
{
public:
   MemoryManager(Tile* tile);
   virtual ~MemoryManager();

   bool __coreInitiateMemoryAccess(MemComponent::Type mem_component,
                                   Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type,
                                   IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length,
                                   Time& curr_time, bool modeled);

   void __handleMsgFromNetwork(NetPacket& packet);

   virtual void outputSummary(std::ostream& os, const Time& target_completion_time);

   Tile* getTile()                        { return _tile; }
   ShmemPerfModel* getShmemPerfModel()    { return _shmem_perf_model; }
   virtual UInt32 getCacheLineSize() = 0;

   virtual void enableModels();
   virtual void disableModels();
   bool isEnabled()                       { return _enabled;  }
  
   // APP + SIM thread synchronization 
   void waitForAppThread();
   void wakeUpAppThread();
   void waitForSimThread();
   void wakeUpSimThread();

   virtual tile_id_t getShmemRequester(const void* pkt_data) = 0;
   // getModeledLength() returns the length of the msg in bits
   virtual UInt32 getModeledLength(const void* pkt_data) = 0;
   virtual bool isModeled(const void* pkt_data) = 0;

   static CachingProtocolType parseProtocolType(std::string& protocol_type);
   static MemoryManager* createMMU(std::string protocol_type, Tile* tile);
   
   virtual void computeEnergy(const Time& curr_time) = 0;

   virtual double getDynamicEnergy() = 0;
   virtual double getLeakageEnergy() = 0;

   // Cache line replication trace
   static void openCacheLineReplicationTraceFiles();
   static void closeCacheLineReplicationTraceFiles();
   static void outputCacheLineReplicationSummary();

   virtual int getDVFS(module_t module, double &frequency, double &voltage) = 0;
   virtual int setDVFS(module_t module, double frequency, voltage_option_t voltage_flag, const Time& curr_time) = 0;

   static CachingProtocolType getCachingProtocolType(){return _caching_protocol_type;}

protected:
   Network* getNetwork() { return _network; }

   vector<tile_id_t> getTileListWithMemoryControllers();
   void printTileListWithMemoryControllers(vector<tile_id_t>& tile_list_with_memory_controllers);

private:
   static CachingProtocolType _caching_protocol_type;
   Tile* _tile;
   Network* _network;
   ShmemPerfModel* _shmem_perf_model;
   
   // App + Sim thread Synchronization
   Lock _lock;
   Semaphore _app_thread_sem;
   Semaphore _sim_thread_sem;

   // Enabled
   bool _enabled;

   virtual bool coreInitiateMemoryAccess(MemComponent::Type mem_component,
                                         Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type,
                                         IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length,
                                         bool modeled) = 0;
   virtual void handleMsgFromNetwork(NetPacket& packet) = 0;
   
   void parseMemoryControllerList(string& memory_controller_positions,
                                  vector<tile_id_t>& tile_list_from_cfg_file,
                                  SInt32 application_tile_count);
};
