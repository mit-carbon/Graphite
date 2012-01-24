#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h" 
#include "memory_manager.h"
#include "log.h"

namespace ShL1ShL2
{

L1CacheCntlr::L1CacheCntlr(MemoryManager* memory_manager,
                           AddressHomeLookup* address_home_lookup,
                           UInt32 cache_line_size,
                           UInt32 l1_icache_size,
                           UInt32 l1_icache_associativity,
                           string l1_icache_replacement_policy,
                           UInt32 l1_icache_access_delay,
                           bool l1_icache_track_miss_types,
                           UInt32 l1_dcache_size,
                           UInt32 l1_dcache_associativity,
                           string l1_dcache_replacement_policy,
                           UInt32 l1_dcache_access_delay,
                           bool l1_dcache_track_miss_types,
                           volatile float frequency)
   : _memory_manager(memory_manager)
   , _address_home_lookup(address_home_lookup)
   , _outstanding_data_buf(NULL)
{
   _l1_icache = new Cache("L1-I",
         l1_icache_size,
         l1_icache_associativity, 
         cache_line_size,
         l1_icache_replacement_policy,
         Cache::PR_L1_CACHE,
         l1_icache_access_delay,
         frequency,
         l1_icache_track_miss_types);
   _l1_dcache = new Cache("L1-D",
         l1_dcache_size,
         l1_dcache_associativity, 
         cache_line_size,
         l1_dcache_replacement_policy,
         Cache::PR_L1_CACHE,
         l1_dcache_access_delay,
         frequency,
         l1_icache_track_miss_types);
}

L1CacheCntlr::~L1CacheCntlr()
{
   delete _l1_icache;
   delete _l1_dcache;
}      

bool
L1CacheCntlr::processMemOpFromTile(MemComponent::component_t mem_component,
                                   Core::lock_signal_t lock_signal,
                                   Core::mem_op_t mem_op_type, 
                                   IntPtr ca_address, UInt32 offset,
                                   Byte* data_buf, UInt32 data_length,
                                   bool modeled)
{
   LOG_PRINT("processMemOpFromTile(), lock_signal(%u), mem_op_type(%u), ca_address(%#lx)",
             lock_signal, mem_op_type, ca_address);

   ShmemMsg::Type shmem_msg_type = ShmemMsg::INVALID_MSG_TYPE;
   Byte* shmem_data_buf = NULL;
   UInt32 shmem_data_length = 0;

   if (mem_op_type == Core::READ)
   {
      shmem_msg_type = ShmemMsg::READ_REQ;
   }
   else if (mem_op_type == Core::READ_EX)
   {
      shmem_msg_type = ShmemMsg::READ_EX_REQ;
   }
   else if (mem_op_type == Core::WRITE)
   {
      shmem_msg_type = ShmemMsg::WRITE_REQ;
      shmem_data_buf = data_buf;
      shmem_data_length = data_length;
   }
   else
   {
      LOG_PRINT_ERROR("Unrecognized Mem Op Type(%u)", mem_op_type);
   }

   // Construct ShmemMsg
   ShmemMsg shmem_msg(shmem_msg_type, MemComponent::L1_DCACHE, MemComponent::DRAM_DIRECTORY, getTileId(), ca_address, offset, data_length,
                      shmem_data_buf, shmem_data_length);
   assert(!_outstanding_data_buf);
   _outstanding_data_buf = data_buf;
   getMemoryManager()->sendMsg(_address_home_lookup->getHome(ca_address), shmem_msg);

   waitForNetworkThread();

   return false;
}

void
L1CacheCntlr::handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   assert(_outstanding_data_buf);
   if (shmem_msg->getMsgType() == ShmemMsg::READ_REP)
      memcpy(_outstanding_data_buf, shmem_msg->getDataBuf(), shmem_msg->getDataLength());
   _outstanding_data_buf = NULL;

   getShmemPerfModel()->setCycleCount(ShmemPerfModel::_USER_THREAD,
                                      getShmemPerfModel()->getCycleCount());

   wakeUpAppThread();
}

void
L1CacheCntlr::waitForNetworkThread()
{
   _app_thread_sem.wait();
}

void
L1CacheCntlr::wakeUpAppThread()
{
   _app_thread_sem.signal();
}

tile_id_t
L1CacheCntlr::getTileId()
{
   return _memory_manager->getTile()->getId();
}

UInt32
L1CacheCntlr::getCacheLineSize()
{ 
   return _memory_manager->getCacheLineSize();
}
 
ShmemPerfModel*
L1CacheCntlr::getShmemPerfModel()
{ 
   return _memory_manager->getShmemPerfModel();
}

}
