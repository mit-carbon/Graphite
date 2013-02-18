#ifndef CORE_H
#define CORE_H

#include <iostream>
using std::ostream;
using std::pair;

// Some forward declarations for cross includes
class Tile;
class CoreModel;
class SyscallMdl;
class SyncClient;
class ClockSkewMinimizationClient;
class PinMemoryManager;

#include "mem_component.h"
#include "fixed_types.h"
#include "capi.h"
#include "packet_type.h"
#include "lock.h"
#include "time_types.h"

class Core
{
public:
   enum State
   {
      RUNNING = 0,
      INITIALIZING,
      STALLED,
      SLEEPING,
      WAKING_UP,
      IDLE,
      NUM_STATES
   };

   enum lock_signal_t
   {
      NONE = 0,
      LOCK,
      UNLOCK
   };

   enum mem_op_t
   {
      READ = 0,
      READ_EX,
      WRITE
   };

   Core(Tile *tile, core_type_t core_type);
   virtual ~Core();

   int coreSendW(int sender, int receiver, char *buffer, int size, carbon_network_t net_type);
   int coreRecvW(int sender, int receiver, char *buffer, int size, carbon_network_t net_type);
   
   virtual Time readInstructionMemory(IntPtr address, UInt32 instruction_size);

   virtual pair<UInt32, Time> initiateMemoryAccess(MemComponent::Type mem_component,
                                                     lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr address,
                                                     Byte* data_buf, UInt32 data_size, bool push_info = false,
                                                     Time time = Time(0));
   
   virtual pair<UInt32, Time> accessMemory(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr address,
                                             char* data_buffer, UInt32 data_size, bool push_info = false);

   core_id_t getId()                         { return _id; }
   Tile *getTile()                           { return _tile; }
   CoreModel *getModel()                     { return _core_model; }
   SyncClient *getSyncClient()               { return _sync_client; }
   SyscallMdl *getSyscallMdl()               { return _syscall_model; }
   ClockSkewMinimizationClient* getClockSkewMinimizationClient() { return _clock_skew_minimization_client; }
   PinMemoryManager *getPinMemoryManager()   { return _pin_memory_manager; }

   State getState()                          { return _state; }
   void setState(State state)                { _state = state; }
  
   void outputSummary(ostream& os);

   void updateInternalVariablesOnFrequencyChange(float old_frequency, float new_frequency);

   void enableModels();
   void disableModels();

   void acquireLock()                        { _lock.acquire(); }
   void releaseLock()                        { _lock.release(); }

private:
   core_id_t _id;
   Tile *_tile;
   CoreModel *_core_model;
   SyncClient *_sync_client;
   SyscallMdl *_syscall_model;
   ClockSkewMinimizationClient *_clock_skew_minimization_client;
   State _state;
   PinMemoryManager *_pin_memory_manager;
   bool _enabled;

   Lock _lock;

   UInt64 _num_memory_accesses;
   Time _total_memory_access_latency;

   void initializeMemoryAccessLatencyCounters();
   void incrTotalMemoryAccessLatency(Time memory_access_latency);
   PacketType getPacketTypeFromUserNetType(carbon_network_t net_type);
};

#endif
