#include "core.h"
#include "main_core.h"
#include "pep_core.h"
#include "tile.h"
#include "network.h"
#include "syscall_model.h"
#include "sync_client.h"
#include "network_types.h"
#include "memory_manager_base.h"
#include "pin_memory_manager.h"
#include "clock_skew_minimization_object.h"
#include "core_perf_model.h"
#include "simulator.h"
#include "log.h"

using namespace std;

Lock Core::m_global_core_lock;

Core * Core::create(Tile* tile, core_type_t core_type)
{
   if (core_type == MAIN_CORE_TYPE)
   {
      return new MainCore(tile); 
   }
   else
   {
      return new PepCore(tile);
   }
}

Core::Core(Tile *tile)
{
   m_tile = tile;
   m_core_state = IDLE;

   //if (Config::getSingleton()->isSimulatingSharedMemory())
   //{
      //LOG_PRINT("instantiated shared memory performance model");
      //m_shmem_perf_model = new ShmemPerfModel();

      //LOG_PRINT("instantiated memory manager model");
      //m_memory_manager = MemoryManagerBase::createMMU(
            //Sim()->getCfg()->getString("caching_protocol/type"),
            //this, m_tile->getNetwork(), m_shmem_perf_model);

      //m_pin_memory_manager = new PinMemoryManager(this);
   //}
   //else
   //{
      //m_shmem_perf_model = (ShmemPerfModel*) NULL;
      //m_memory_manager = (MemoryManagerBase *) NULL;
      //m_pin_memory_manager = (PinMemoryManager*) NULL;

      //LOG_PRINT("No Memory Manager being used");
   //}

   m_syscall_model = new SyscallMdl(m_tile->getNetwork());
   m_sync_client = new SyncClient(this);
}

Core::~Core()
{
   if (this->getCoreId().second == MAIN_CORE_TYPE){
      LOG_PRINT("Deleting main core on tile %d", this->getCoreId().first);
   }
   else if (this->getCoreId().second == PEP_CORE_TYPE) {
      LOG_PRINT("Deleting PEP core on tile %d", this->getCoreId().first);
   }
   else {
      LOG_PRINT_ERROR("Invalid core type!");
   }

   delete m_sync_client;
   delete m_syscall_model;
}

int Core::coreSendW(int sender, int receiver, char* buffer, int size, carbon_network_t net_type)
{
   PacketType pkt_type = getPktTypeFromUserNetType(net_type);

   SInt32 sent;
   if (receiver == CAPI_ENDPOINT_ALL)
      sent = m_tile->getNetwork()->netBroadcast(pkt_type, buffer, size);
   else
      sent = m_tile->getNetwork()->netSend(receiver, pkt_type, buffer, size);
   
   LOG_ASSERT_ERROR(sent == size, "Bytes Sent(%i), Message Size(%i)", sent, size);

   return sent == size ? 0 : -1;
}

int Core::coreRecvW(int sender, int receiver, char* buffer, int size, carbon_network_t net_type)
{
   PacketType pkt_type = getPktTypeFromUserNetType(net_type);

   NetPacket packet;
   if (sender == CAPI_ENDPOINT_ANY)
      packet = m_tile->getNetwork()->netRecvType(pkt_type);
   else
      packet = m_tile->getNetwork()->netRecv(sender, pkt_type);

   LOG_PRINT("Got packet: from %i, to %i, type %i, len %i", packet.sender, packet.receiver, (SInt32)packet.type, packet.length);

   LOG_ASSERT_ERROR((unsigned)size == packet.length, "Tile: User thread requested packet of size: %d, got a packet from %d of size: %d", size, sender, packet.length);

   memcpy(buffer, packet.data, size);

   // De-allocate dynamic memory
   // Is this the best place to de-allocate packet.data ??
   delete [](Byte*)packet.data;

   return (unsigned)size == packet.length ? 0 : -1;
}

PacketType Core::getPktTypeFromUserNetType(carbon_network_t net_type)
{
   switch(net_type)
   {
      case CARBON_NET_USER_1:
         return USER_1;

      case CARBON_NET_USER_2:
         return USER_2;

      default:
         LOG_PRINT_ERROR("Unrecognized User Network(%u)", net_type);
         return (PacketType) -1;
   }
}

pair<UInt32, UInt64>
Core::nativeMemOp(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size)
{
   if (data_size <= 0)
   {
      return make_pair<UInt32, UInt64>(0,0);
   }

   if (lock_signal == LOCK)
   {
      assert(mem_op_type == READ_EX);
      m_global_core_lock.acquire();
   }

   if ( (mem_op_type == READ) || (mem_op_type == READ_EX) )
   {
      memcpy ((void*) data_buffer, (void*) d_addr, (size_t) data_size);
   }
   else if (mem_op_type == WRITE)
   {
      memcpy ((void*) d_addr, (void*) data_buffer, (size_t) data_size);
   }

   if (lock_signal == UNLOCK)
   {
      assert(mem_op_type == WRITE);
      m_global_core_lock.release();
   }

   return make_pair<UInt32, UInt64>(0,0);
}


int Core::getTileId() 
{ 
   return m_tile->getId(); 
}

Network* Core::getNetwork() 
{ 
   return m_tile->getNetwork(); 
}

Core::State 
Core::getState()
{
   ScopedLock scoped_lock(m_core_state_lock);
   return m_core_state;
}

void
Core::setState(State core_state)
{
   ScopedLock scoped_lock(m_core_state_lock);
   m_core_state = core_state;
}
