#include <cmath>
#include <cstring>
#include "core.h"
#include "tile.h"
#include "core_model.h"
#include "syscall_model.h"
#include "sync_client.h"
#include "network_types.h"
#include "memory_manager.h"
#include "pin_memory_manager.h"
#include "clock_skew_minimization_object.h"
#include "simulator.h"
#include "config.h"
#include "log.h"

Core::Core(Tile *tile, core_type_t core_type)
   : _tile(tile)
   , _core_model(NULL)
   , _state(IDLE)
   , _pin_memory_manager(NULL)
   , _enabled(false)
{
   _id = (core_id_t) {_tile->getId(), core_type};
   if (Config::getSingleton()->getEnableCoreModeling())
      _core_model = CoreModel::create(this);

   _sync_client = new SyncClient(this);
   _syscall_model = new SyscallMdl(this);
   _clock_skew_minimization_client =
      ClockSkewMinimizationClient::create(Sim()->getCfg()->getString("clock_skew_minimization/scheme","none"), this);
 
   if (Config::getSingleton()->isSimulatingSharedMemory())
      _pin_memory_manager = new PinMemoryManager(this);

   initializeMemoryAccessLatencyCounters();
}

Core::~Core()
{
   if (_pin_memory_manager)
      delete _pin_memory_manager;

   if (_clock_skew_minimization_client)
      delete _clock_skew_minimization_client;

   delete _syscall_model;
   delete _sync_client;
   delete _core_model;
}

int
Core::coreSendW(int sender, int receiver, char* buffer, int size, carbon_network_t net_type)
{
   PacketType packet_type = getPacketTypeFromUserNetType(net_type);

   core_id_t receiver_core = (core_id_t) {receiver, _id.core_type};

   SInt32 sent = (receiver == CAPI_ENDPOINT_ALL) ?
                 _tile->getNetwork()->netBroadcast(packet_type, buffer, size) :
                 _tile->getNetwork()->netSend(receiver_core, packet_type, buffer, size);
   
   LOG_ASSERT_ERROR(sent == size, "Bytes Sent(%i), Message Size(%i)", sent, size);

   return (sent == size) ? 0 : -1;
}

int
Core::coreRecvW(int sender, int receiver, char* buffer, int size, carbon_network_t net_type)
{
   PacketType packet_type = getPacketTypeFromUserNetType(net_type);

   core_id_t sender_core = (core_id_t) {sender, _id.core_type};

   NetPacket packet = (sender == CAPI_ENDPOINT_ANY) ?
                      _tile->getNetwork()->netRecvType(packet_type, _id) :
                      _tile->getNetwork()->netRecv(sender_core, _id, packet_type);

   LOG_PRINT("Got packet: from (%i,%i), to (%i,%i), type (%i), len (%i)",
             packet.sender.tile_id, packet.sender.core_type, packet.receiver.tile_id, packet.receiver.core_type,
             (SInt32) packet.type, packet.length);

   LOG_ASSERT_ERROR((unsigned)size == packet.length,
                    "Application requested packet of size: %d, got a packet from %d of size: %d",
                    size, sender, packet.length);

   memcpy(buffer, packet.data, size);

   // De-allocate dynamic memory
   // Is this the best place to de-allocate packet.data ??
   delete [] (Byte*) packet.data;

   return (unsigned)size == packet.length ? 0 : -1;
}

// accessMemory(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr address, char* data_buffer, UInt32 data_size, bool push_info)
//
// Arguments:
//   lock_signal :: NONE, LOCK, or UNLOCK
//   mem_op_type :: READ, READ_EX, or WRITE
//   address :: address of location we want to access (read or write)
//   data_buffer :: buffer holding data for WRITE or buffer which must be written on a READ
//   data_size :: size of data we must read/write
//   push_info :: says whether memory info must be pushed to the core model
//
// Return Value:
//   (number of misses, memory access latency) :: the number of cache misses and memory access latency

pair<UInt32, UInt64>
Core::accessMemory(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr address, char* data_buffer, UInt32 data_size, bool push_info)
{
   return initiateMemoryAccess(MemComponent::L1_DCACHE, lock_signal, mem_op_type, address, (Byte*) data_buffer, data_size, push_info);
}

UInt64
Core::readInstructionMemory(IntPtr address, UInt32 instruction_size)
{
   LOG_PRINT("Instruction: Address(%#lx), Size(%u), Start READ", address, instruction_size);

   Byte buf[instruction_size];
   return initiateMemoryAccess(MemComponent::L1_ICACHE, Core::NONE, Core::READ, address, buf, instruction_size).second;
}

pair<UInt32, UInt64>
Core::initiateMemoryAccess(MemComponent::Type mem_component, lock_signal_t lock_signal, mem_op_t mem_op_type,
                           IntPtr address, Byte* data_buf, UInt32 data_size,
                           bool push_info, UInt64 time)
{
   LOG_ASSERT_ERROR(Config::getSingleton()->isSimulatingSharedMemory(), "Shared Memory Disabled");

   if (data_size == 0)
   {
      if (push_info)
      {
         DynamicInstructionInfo info = DynamicInstructionInfo::createMemoryInfo(0, address, (mem_op_type == WRITE) ? Operand::WRITE : Operand::READ, 0);
         if (_core_model)
            _core_model->pushDynamicInstructionInfo(info);
      }
      return make_pair<UInt32, UInt64>(0,0);
   }

   // Setting the initial time
   UInt64 initial_time = (time == 0) ? _core_model->getCycleCount() : time;
   UInt64 curr_time = initial_time;

   LOG_PRINT("Time(%llu), %s - ADDR(%#lx), data_size(%u), START",
             initial_time, ((mem_op_type == READ) ? "READ" : "WRITE"), address, data_size);

   UInt32 num_misses = 0;
   UInt32 cache_line_size = _tile->getMemoryManager()->getCacheLineSize();

   IntPtr begin_addr = address;
   IntPtr end_addr = address + data_size;
   IntPtr begin_addr_aligned = begin_addr - (begin_addr % cache_line_size);
   IntPtr end_addr_aligned = end_addr - (end_addr % cache_line_size);
   Byte *curr_data_buffer_head = (Byte*) data_buf;

   for (IntPtr curr_addr_aligned = begin_addr_aligned; curr_addr_aligned <= end_addr_aligned; curr_addr_aligned += cache_line_size)
   {
      // Access the cache one line at a time
      UInt32 curr_offset;
      UInt32 curr_size;

      // Determine the offset
      if (curr_addr_aligned == begin_addr_aligned)
      {
         curr_offset = begin_addr % cache_line_size;
      }
      else
      {
         curr_offset = 0;
      }

      // Determine the size
      if (curr_addr_aligned == end_addr_aligned)
      {
         curr_size = (end_addr % cache_line_size) - (curr_offset);
         if (curr_size == 0)
         {
            continue;
         }
      }
      else
      {
         curr_size = cache_line_size - (curr_offset);
      }

      LOG_PRINT("Start coreInitiateMemoryAccess: ADDR(%#lx), offset(%u), curr_size(%u), core_id(%i, %i)",
                curr_addr_aligned, curr_offset, curr_size, getId().tile_id, getId().core_type);

      if (!_tile->getMemoryManager()->__coreInitiateMemoryAccess(mem_component, lock_signal, mem_op_type, 
                                                                 curr_addr_aligned, curr_offset, 
                                                                 curr_data_buffer_head, curr_size,
                                                                 curr_time, push_info))
      {
         // If it is a READ or READ_EX operation, 
         //    'coreInitiateMemoryAccess' causes curr_data_buffer_head to be automatically filled in
         // If it is a WRITE operation, 
         //    'coreInitiateMemoryAccess' reads the data from curr_data_buffer_head
         num_misses ++;
      }

      LOG_PRINT("End InitiateSharedMemReq: ADDR(%#lx), offset(%u), curr_size(%u), core_id(%i,%i)",
                curr_addr_aligned, curr_offset, curr_size, getId().tile_id, getId().core_type);

      // Increment the buffer head
      curr_data_buffer_head += curr_size;
   }

   // Get the final cycle time
   UInt64 final_time = curr_time;
   LOG_ASSERT_ERROR(final_time >= initial_time, "final_time(%llu) < initial_time(%llu)", final_time, initial_time);
   
   LOG_PRINT("Time(%llu), %s - ADDR(%#lx), data_size(%u), END", 
             final_time, ((mem_op_type == READ) ? "READ" : "WRITE"), address, data_size);

   // Calculate the round-trip time
   UInt64 memory_access_latency = final_time - initial_time;
   incrTotalMemoryAccessLatency(memory_access_latency);
   
   if (push_info)
   {
      DynamicInstructionInfo info = DynamicInstructionInfo::createMemoryInfo(memory_access_latency, address, (mem_op_type == WRITE) ? Operand::WRITE : Operand::READ, num_misses);
      if (_core_model)
         _core_model->pushDynamicInstructionInfo(info);
   }

   return make_pair<UInt32, UInt64>(num_misses, memory_access_latency);
}
PacketType
Core::getPacketTypeFromUserNetType(carbon_network_t net_type)
{
   switch (net_type)
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

void
Core::outputSummary(ostream& os)
{
   if (_core_model)
      _core_model->outputSummary(os);
   
   _total_memory_access_latency_in_ns +=
      (UInt64) (ceil(1.0 * _total_memory_access_latency_in_clock_cycles / _core_model->getFrequency()));
   
   os << "Shared Memory Model summary: " << endl;
   os << "    Total Memory Accesses: " << _num_memory_accesses << endl;
   os << "    Average Memory Access Latency (in ns): " << 
      (1.0 * _total_memory_access_latency_in_ns / _num_memory_accesses) << endl;
   
}

void
Core::enableModels()
{
   _enabled = true;
   if (_core_model)
      _core_model->enable();
   if (_clock_skew_minimization_client)
      _clock_skew_minimization_client->enable();
}

void
Core::disableModels()
{
   _enabled = false;
   if (_core_model)
      _core_model->disable();
   if (_clock_skew_minimization_client)
      _clock_skew_minimization_client->disable();
}

void
Core::updateInternalVariablesOnFrequencyChange(volatile float frequency)
{
   if (_core_model)
      _core_model->updateInternalVariablesOnFrequencyChange(frequency);

   _total_memory_access_latency_in_ns +=
      (UInt64) (ceil(1.0 * _total_memory_access_latency_in_clock_cycles / frequency));
   _total_memory_access_latency_in_clock_cycles = 0;
}

void
Core::initializeMemoryAccessLatencyCounters()
{
   _num_memory_accesses = 0;
   _total_memory_access_latency_in_clock_cycles = 0;
   _total_memory_access_latency_in_ns = 0;
}

void
Core::incrTotalMemoryAccessLatency(UInt64 memory_access_latency)
{
   if (_enabled)
   {
      _num_memory_accesses ++;
      _total_memory_access_latency_in_clock_cycles += memory_access_latency;
   }
}
