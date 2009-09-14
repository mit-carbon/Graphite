#include "core.h"
#include "network.h"
#include "syscall_model.h"
#include "sync_client.h"
#include "network_types.h"
#include "memory_manager_base.h"
#include "pin_memory_manager.h"
#include "performance_model.h"
#include "log.h"

using namespace std;

Lock Core::m_global_core_lock;

Core::Core(SInt32 id)
   : m_core_id(id)
{
   LOG_PRINT("Core ctor for: %d", id);

   m_network = new Network(this);

   m_performance_model = PerformanceModel::create();

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      LOG_PRINT("instantiated memory manager model");
      m_memory_manager = MemoryManagerBase::createMMU(
            MemoryManagerBase::PR_L1_PR_L2_DRAM_DIR, this, m_network);
      m_pin_memory_manager = new PinMemoryManager(this);
      LOG_PRINT("instantiated shared memory performance model");
      m_shmem_perf_model = new ShmemPerfModel();
   }
   else
   {
      m_memory_manager = (MemoryManagerBase *) NULL;
      m_pin_memory_manager = (PinMemoryManager*) NULL;
      m_shmem_perf_model = (ShmemPerfModel*) NULL;

      LOG_PRINT("No Memory Manager being used");
   }

   m_syscall_model = new SyscallMdl(m_network);
   m_sync_client = new SyncClient(this);
}

Core::~Core()
{
   delete m_sync_client;
   delete m_syscall_model;
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      delete m_shmem_perf_model;
      delete m_pin_memory_manager;
      delete m_memory_manager;
   }
   delete m_performance_model;
   delete m_network;
}

void Core::outputSummary(std::ostream &os)
{
   if (Config::getSingleton()->getEnablePerformanceModeling())
   {
      getPerformanceModel()->outputSummary(os);
      getNetwork()->outputSummary(os);
   
      if (Config::getSingleton()->isSimulatingSharedMemory())
      {
         getShmemPerfModel()->outputSummary(os);
      }
   }
}

int Core::coreSendW(int sender, int receiver, char* buffer, int size)
{
   // Create a net packet
   NetPacket packet;
   packet.sender= sender;
   packet.receiver= receiver;
   packet.type = USER;
   packet.length = size;
   packet.data = buffer;

   SInt32 sent = m_network->netSend(packet);

   assert(sent == size);

   return sent == size ? 0 : -1;
}

int Core::coreRecvW(int sender, int receiver, char* buffer, int size)
{
   NetPacket packet;

   packet = m_network->netRecv(sender, USER);

   LOG_PRINT("Got packet: from %i, to %i, type %i, len %i", packet.sender, packet.receiver, (SInt32)packet.type, packet.length);

   LOG_ASSERT_ERROR((unsigned)size == packet.length, "Core: User thread requested packet of size: %d, got a packet from %d of size: %d", size, sender, packet.length);

   memcpy(buffer, packet.data, size);

   // De-allocate dynamic memory
   // Is this the best place to de-allocate packet.data ??
   delete [](Byte*)packet.data;

   return (unsigned)size == packet.length ? 0 : -1;
}

void Core::enablePerformanceModels()
{
   getShmemPerfModel()->enable();
   getNetwork()->enableModels();
   getPerformanceModel()->enable();
}

void Core::disablePerformanceModels()
{
   getShmemPerfModel()->disable();
   getNetwork()->disableModels();
   getPerformanceModel()->disable();
}

UInt32 Core::initiateMemoryAccess(
      MemComponent::component_t mem_component, 
      lock_signal_t lock_signal, 
      mem_op_t mem_op_type, 
      IntPtr address, 
      Byte* data_buf, UInt32 data_size,
      bool modeled)
{
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      UInt32 num_misses = 0;
      LOG_PRINT("%s - ADDR(0x%x), data_size(%u), START", 
               ((mem_op_type == READ) ? "READ" : "WRITE"), 
               address, data_size);

      if (data_size <= 0)
      {
         if (modeled)
         {
            DynamicInstructionInfo info = DynamicInstructionInfo::createMemoryInfo(0, address, (mem_op_type == WRITE) ? Operand::WRITE : Operand::READ, 0);
            m_performance_model->pushDynamicInstructionInfo(info);
         }
         return (num_misses);
      }

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

         LOG_PRINT("Start InitiateSharedMemReq: ADDR: %x, offset: %u, curr_size: %u", curr_addr_aligned, curr_offset, curr_size);

         if (!getMemoryManager()->coreInitiateMemoryAccess(
                  mem_component,
                  lock_signal, 
                  mem_op_type, 
                  curr_addr_aligned, curr_offset, 
                  curr_data_buffer_head, curr_size))
         {
            // If it is a READ or READ_EX operation, 'initiateSharedMemReq' causes curr_data_buffer_head to be automatically filled in
            // If it is a WRITE operation, 'initiateSharedMemReq' reads the data from curr_data_buffer_head
            num_misses ++;
         }

         LOG_PRINT("End InitiateSharedMemReq: ADDR: %x, offset: %u, curr_size: %u", curr_addr_aligned, curr_offset, curr_size);

         // Increment the buffer head
         curr_data_buffer_head += curr_size;
      }

      LOG_PRINT("%s - ADDR(0x%x), data_size(%u), END\n", 
               ((mem_op_type == READ) ? "READ" : "WRITE"), 
               address, data_size);

      if (modeled)
      {
         DynamicInstructionInfo info = DynamicInstructionInfo::createMemoryInfo(0, address, (mem_op_type == WRITE) ? Operand::WRITE : Operand::READ, num_misses);
         m_performance_model->pushDynamicInstructionInfo(info);
      }

      return (num_misses);
   }

   return 0;
}

/*
 * accessMemory (lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size)
 *
 * Arguments:
 *   lock_signal :: NONE, LOCK, or UNLOCK
 *   mem_op_type :: READ, READ_EX, or WRITE
 *   d_addr :: address of location we want to access (read or write)
 *   data_buffer :: buffer holding data for WRITE or buffer which must be written on a READ
 *   data_size :: size of data we must read/write
 *
 * Return Value:
 *   number of misses :: State the number of cache misses
 */
UInt32 Core::accessMemory(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size, bool modeled)
{
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
#ifdef REDIRECT_MEMORY

      UInt32 num_misses = 0;
      LOG_PRINT("%s - ADDR: 0x%x, data_size: %u, START!!", 
               ((mem_op_type == READ) ? " READ " : " WRITE "), d_addr, data_size);

      if (data_size <= 0)
      {
         if (modeled)
         {
            DynamicInstructionInfo info = DynamicInstructionInfo::createMemoryInfo(0, d_addr, (mem_op_type == WRITE) ? Operand::WRITE : Operand::READ, 0);
            m_performance_model->pushDynamicInstructionInfo(info);
         }
         return (num_misses);
      }

      UInt32 cache_block_size = getMemoryManager()->getCacheBlockSize();

      IntPtr begin_addr = d_addr;
      IntPtr end_addr = d_addr + data_size;
      IntPtr begin_addr_aligned = begin_addr - (begin_addr % cache_block_size);
      IntPtr end_addr_aligned = end_addr - (end_addr % cache_block_size);
      Byte *curr_data_buffer_head = (Byte*) data_buffer;

      for (IntPtr curr_addr_aligned = begin_addr_aligned ; curr_addr_aligned <= end_addr_aligned; curr_addr_aligned += cache_block_size)
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

         LOG_PRINT("Start InitiateSharedMemReq: ADDR: %x, offset: %u, curr_size: %u", curr_addr_aligned, curr_offset, curr_size);

         if (!getMemoryManager()->coreInitiateMemoryAccess(MemComponent::L1_DCACHE, lock_signal, mem_op_type, curr_addr_aligned, curr_offset, curr_data_buffer_head, curr_size))
         {
            // If it is a READ or READ_EX operation, 'initiateSharedMemReq' causes curr_data_buffer_head to be automatically filled in
            // If it is a WRITE operation, 'initiateSharedMemReq' reads the data from curr_data_buffer_head
            num_misses ++;
         }

         LOG_PRINT("End InitiateSharedMemReq: ADDR: %x, offset: %u, curr_size: %u", curr_addr_aligned, curr_offset, curr_size);

         // Increment the buffer head
         curr_data_buffer_head += curr_size;
      }

      LOG_PRINT("%s - ADDR: %x, data_size: %u, END!!", 
               ((mem_op_type == READ) ? "READ" : "WRITE"), 
               d_addr, data_size);

      if (modeled)
      {
         DynamicInstructionInfo info = DynamicInstructionInfo::createMemoryInfo(0, d_addr, (mem_op_type == WRITE) ? Operand::WRITE : Operand::READ, num_misses);
         m_performance_model->pushDynamicInstructionInfo(info);
      }

      return (num_misses);
#else
      return nativeMemOp (lock_signal, mem_op_type, d_addr, data_buffer, data_size);
#endif
   }
   
   else
   {   
      return nativeMemOp (lock_signal, mem_op_type, d_addr, data_buffer, data_size);
   }
}


UInt32 Core::nativeMemOp(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size)
{
   if (data_size <= 0)
   {
      return 0;
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

   return 0;
}
