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
   //if (Config::getSingleton()->isSimulatingSharedMemory())
   //{
      //delete m_pin_memory_manager;
      //delete m_memory_manager;
      //delete m_shmem_perf_model;
   //}
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



UInt64
Core::readInstructionMemory(IntPtr address, UInt32 instruction_size)
{
   LOG_PRINT("Instruction: Address(0x%x), Size(%u), Start READ", 
           address, instruction_size);

   Byte buf[instruction_size];
   return (initiateMemoryAccess(MemComponent::L1_ICACHE,
         Core::NONE, Core::READ, address, buf, instruction_size).second);
}

pair<UInt32, UInt64>
Core::initiateMemoryAccess(MemComponent::component_t mem_component, 
      lock_signal_t lock_signal, 
      mem_op_t mem_op_type, 
      IntPtr address, 
      Byte* data_buf, UInt32 data_size,
      bool modeled,
      UInt64 time)
{
   if (data_size <= 0)
   {
      if (modeled)
      {
         DynamicInstructionInfo info = DynamicInstructionInfo::createMemoryInfo(0, address, (mem_op_type == WRITE) ? Operand::WRITE : Operand::READ, 0);
         m_core_perf_model->pushDynamicInstructionInfo(info);
      }
      return make_pair<UInt32, UInt64>(0,0);
   }

   // Setting the initial time
   UInt64 initial_time = time;
   if (time == 0)
      initial_time = getPerformanceModel()->getCycleCount();

   getShmemPerfModel()->setCycleCount(initial_time);

   LOG_PRINT("Time(%llu), %s - ADDR(0x%x), data_size(%u), START",
        initial_time,
        ((mem_op_type == READ) ? "READ" : "WRITE"), 
        address, data_size);

   UInt32 num_misses = 0;
   UInt32 cache_block_size = getMemoryManager()->getCacheBlockSize();

   IntPtr begin_addr = address;
   IntPtr end_addr = address + data_size;
   IntPtr begin_addr_aligned = begin_addr - (begin_addr % cache_block_size);
   IntPtr end_addr_aligned = end_addr - (end_addr % cache_block_size);
   Byte *curr_data_buffer_head = (Byte*) data_buf;

   for (IntPtr curr_addr_aligned = begin_addr_aligned; curr_addr_aligned <= end_addr_aligned; curr_addr_aligned += cache_block_size)
   {
      // Access the cache one line at a time
      UInt32 curr_offset;
      UInt32 curr_size;

      // Determine the offset
      if (curr_addr_aligned == begin_addr_aligned)
      {
         curr_offset = begin_addr % cache_block_size;
      }
      else
      {
         curr_offset = 0;
      }

      // Determine the size
      if (curr_addr_aligned == end_addr_aligned)
      {
         curr_size = (end_addr % cache_block_size) - (curr_offset);
         if (curr_size == 0)
         {
            continue;
         }
      }
      else
      {
         curr_size = cache_block_size - (curr_offset);
      }

      LOG_PRINT("Start InitiateSharedMemReq: ADDR(0x%x), offset(%u), curr_size(%u)", curr_addr_aligned, curr_offset, curr_size);

      if (!getMemoryManager()->coreInitiateMemoryAccess(
               mem_component,
               lock_signal, 
               mem_op_type, 
               curr_addr_aligned, curr_offset, 
               curr_data_buffer_head, curr_size,
               modeled))
      {
         // If it is a READ or READ_EX operation, 
         // 'initiateSharedMemReq' causes curr_data_buffer_head 
         // to be automatically filled in
         // If it is a WRITE operation, 
         // 'initiateSharedMemReq' reads the data 
         // from curr_data_buffer_head
         num_misses ++;
      }

      LOG_PRINT("End InitiateSharedMemReq: ADDR(0x%x), offset(%u), curr_size(%u)", curr_addr_aligned, curr_offset, curr_size);

      // Increment the buffer head
      curr_data_buffer_head += curr_size;
   }

   // Get the final cycle time
   UInt64 final_time = getShmemPerfModel()->getCycleCount();
   LOG_ASSERT_ERROR(final_time >= initial_time,
         "final_time(%llu) < initial_time(%llu)",
         final_time, initial_time);
   
   LOG_PRINT("Time(%llu), %s - ADDR(0x%x), data_size(%u), END\n", 
        final_time,
        ((mem_op_type == READ) ? "READ" : "WRITE"), 
        address, data_size);

   // Calculate the round-trip time
   UInt64 memory_access_latency = final_time - initial_time;

   if (modeled)
   {
      DynamicInstructionInfo info = DynamicInstructionInfo::createMemoryInfo(memory_access_latency, \
            address, (mem_op_type == WRITE) ? Operand::WRITE : Operand::READ, num_misses);
      m_core_perf_model->pushDynamicInstructionInfo(info);

      getShmemPerfModel()->incrTotalMemoryAccessLatency(memory_access_latency);
   }

   return make_pair<UInt32, UInt64>(num_misses, memory_access_latency);
}

// FIXME: This should actually be 'accessDataMemory()'
/*
 * accessMemory (lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size, bool modeled)
 *
 * Arguments:
 *   lock_signal :: NONE, LOCK, or UNLOCK
 *   mem_op_type :: READ, READ_EX, or WRITE
 *   d_addr :: address of location we want to access (read or write)
 *   data_buffer :: buffer holding data for WRITE or buffer which must be written on a READ
 *   data_size :: size of data we must read/write
 *   modeled :: says whether it is modeled or not
 *
 * Return Value:
 *   number of misses :: State the number of cache misses
 */
pair<UInt32, UInt64>
Core::accessMemory(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size, bool modeled)
{
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      return initiateMemoryAccess(MemComponent::L1_DCACHE, lock_signal, mem_op_type, d_addr, (Byte*) data_buffer, data_size, modeled);
   }
   
   else
   {   
      return nativeMemOp (lock_signal, mem_op_type, d_addr, data_buffer, data_size);
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
