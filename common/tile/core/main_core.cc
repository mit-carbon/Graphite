#include "tile.h"
#include "core.h"
#include "main_core.h"
#include "network.h"
#include "memory_manager_base.h"
#include "pin_memory_manager.h"
#include "core_model.h"
#include "sync_client.h"
#include "simulator.h"
#include "log.h"
#include "tile_manager.h"

using namespace std;

MainCore::MainCore(Tile* tile) : Core(tile)
{
   m_core_id = tile->getMainCoreId();
   m_core_model = CoreModel::createMainPerfModel((Core *) this);

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      m_pin_memory_manager = new PinMemoryManager(this);
   }
   else
   {
      m_pin_memory_manager = (PinMemoryManager*) NULL;
      LOG_PRINT("No Memory Manager being used for main core");
   }

}

MainCore::~MainCore()
{
   delete m_core_model;
   
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      delete m_pin_memory_manager;
   }
}

/*
 * accessMemory (lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size, bool push_info)
 *
 * Arguments:
 *   lock_signal :: NONE, LOCK, or UNLOCK
 *   mem_op_type :: READ, READ_EX, or WRITE
 *   d_addr :: address of location we want to access (read or write)
 *   data_buffer :: buffer holding data for WRITE or buffer which must be written on a READ
 *   data_size :: size of data we must read/write
 *   push_info :: says whether we need to push the info to the core model
 *   
 * Return Value:
 *   number of misses :: State the number of cache misses
 */
pair<UInt32, UInt64>
MainCore::accessMemory(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size, bool push_info)
{
   if (Config::getSingleton()->isSimulatingSharedMemory() || (Config::getSingleton()->getSimulationMode() == Config::LITE) )
   {
      pair<UInt32, UInt64> res = initiateMemoryAccess(MemComponent::L1_DCACHE, lock_signal, mem_op_type, d_addr, (Byte*) data_buffer, data_size, push_info);
      return res;
   }
   
   else
   {   
      return nativeMemOp (lock_signal, mem_op_type, d_addr, data_buffer, data_size);
   }
}

UInt64
MainCore::readInstructionMemory(IntPtr address, UInt32 instruction_size)
{
   LOG_PRINT("Instruction: Address(%#lx), Size(%u), Start READ", address, instruction_size);

   Byte buf[instruction_size];
   return initiateMemoryAccess(MemComponent::L1_ICACHE, Core::NONE, Core::READ, address, buf, instruction_size).second;
}

pair<UInt32, UInt64>
MainCore::initiateMemoryAccess(MemComponent::component_t mem_component,
                               lock_signal_t lock_signal,
                               mem_op_t mem_op_type,
                               IntPtr address,
                               Byte* data_buf, UInt32 data_size,
                               bool push_info,
                               UInt64 time)
{
   if (data_size <= 0)
   {
      if (push_info)
      {
         DynamicInstructionInfo info = DynamicInstructionInfo::createMemoryInfo(0, address, (mem_op_type == WRITE) ? Operand::WRITE : Operand::READ, 0);
         m_core_model->pushDynamicInstructionInfo(info);
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

      LOG_PRINT("Start coreInitiateMemoryAccess: ADDR(0x%x), offset(%u), curr_size(%u), core_id(%d, %d)", curr_addr_aligned, curr_offset, curr_size, getCoreId().tile_id, getCoreId().core_type);

      if (!getMemoryManager()->coreInitiateMemoryAccess(
               mem_component,
               lock_signal, 
               mem_op_type, 
               curr_addr_aligned, curr_offset, 
               curr_data_buffer_head, curr_size,
               push_info))
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

   if (push_info)
   {
      DynamicInstructionInfo info = DynamicInstructionInfo::createMemoryInfo(memory_access_latency, address, (mem_op_type == WRITE) ? Operand::WRITE : Operand::READ, num_misses);

      m_core_model->pushDynamicInstructionInfo(info);

      getShmemPerfModel()->incrTotalMemoryAccessLatency(memory_access_latency);
   }

   return make_pair<UInt32, UInt64>(num_misses, memory_access_latency);
}
