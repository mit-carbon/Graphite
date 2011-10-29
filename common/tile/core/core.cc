#include "core.h"
#include "main_core.h"
#include "tile.h"
#include "network.h"
#include "syscall_model.h"
#include "sync_client.h"
#include "network_types.h"
#include "memory_manager_base.h"
#include "pin_memory_manager.h"
#include "clock_skew_minimization_object.h"
#include "core_model.h"
#include "simulator.h"
#include "log.h"

using namespace std;

Lock Core::m_global_core_lock;

Core * Core::create(Tile* tile, core_type_t core_type)
{
   return new MainCore(tile); 
}

Core::Core(Tile *tile)
{
   m_tile = tile;
   m_core_state = IDLE;
   m_sync_client = new SyncClient(this);
}

Core::~Core()
{
   LOG_PRINT("Deleting main core on tile %d", this->getCoreId().tile_id);

   delete m_sync_client;
}

int Core::coreSendW(int sender, int receiver, char* buffer, int size, carbon_network_t net_type)
{
   PacketType pkt_type = getPktTypeFromUserNetType(net_type);

   core_id_t receiver_core = (core_id_t) {receiver, this->getCoreType()};

   SInt32 sent;
   if (receiver == CAPI_ENDPOINT_ALL)
      sent = m_tile->getNetwork()->netBroadcast(pkt_type, buffer, size);
   else
      sent = m_tile->getNetwork()->netSend(receiver_core, pkt_type, buffer, size);
   
   LOG_ASSERT_ERROR(sent == size, "Bytes Sent(%i), Message Size(%i)", sent, size);

   return sent == size ? 0 : -1;
}

int Core::coreRecvW(int sender, int receiver, char* buffer, int size, carbon_network_t net_type)
{
   PacketType pkt_type = getPktTypeFromUserNetType(net_type);

   core_id_t sender_core = (core_id_t) {sender, this->getCoreType()};

   NetPacket packet;
   if (sender == CAPI_ENDPOINT_ANY)
      packet = m_tile->getNetwork()->netRecvType(pkt_type, this->getCoreId());
   else
      packet = m_tile->getNetwork()->netRecv(sender_core, this->getCoreId(), pkt_type);

   LOG_PRINT("Got packet: from {%i, %i}, to {%i, %i}, type %i, len %i", packet.sender.tile_id, packet.sender.core_type, packet.receiver.tile_id, packet.receiver.core_type, (SInt32)packet.type, packet.length);

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
