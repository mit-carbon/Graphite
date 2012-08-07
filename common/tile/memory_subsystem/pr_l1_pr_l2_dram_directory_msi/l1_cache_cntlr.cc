#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h" 
#include "memory_manager.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMSI
{

L1CacheCntlr::L1CacheCntlr(MemoryManager* memory_manager,
                           Semaphore* app_thread_sem,
                           Semaphore* sim_thread_sem,
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
                           float frequency)
   : _memory_manager(memory_manager)
   , _l2_cache_cntlr(NULL)
   , _app_thread_sem(app_thread_sem)
   , _sim_thread_sem(sim_thread_sem)
{
   _l1_icache_replacement_policy_obj = 
      CacheReplacementPolicy::create(l1_icache_replacement_policy, l1_icache_size, l1_icache_associativity, cache_line_size);
   _l1_dcache_replacement_policy_obj = 
      CacheReplacementPolicy::create(l1_dcache_replacement_policy, l1_dcache_size, l1_dcache_associativity, cache_line_size);
   _l1_icache_hash_fn_obj = new CacheHashFn(l1_icache_size, l1_icache_associativity, cache_line_size);
   _l1_dcache_hash_fn_obj = new CacheHashFn(l1_dcache_size, l1_dcache_associativity, cache_line_size);

   _l1_icache = new Cache("L1-I",
         PR_L1_PR_L2_DRAM_DIRECTORY_MSI,
         Cache::INSTRUCTION_CACHE,
         L1,
         Cache::UNDEFINED_WRITE_POLICY,
         l1_icache_size,
         l1_icache_associativity, 
         cache_line_size,
         _l1_icache_replacement_policy_obj,
         _l1_icache_hash_fn_obj,
         l1_icache_access_delay,
         frequency,
         l1_icache_track_miss_types);
   _l1_dcache = new Cache("L1-D",
         PR_L1_PR_L2_DRAM_DIRECTORY_MSI,
         Cache::DATA_CACHE,
         L1,
         Cache::WRITE_THROUGH,
         l1_dcache_size,
         l1_dcache_associativity, 
         cache_line_size,
         _l1_dcache_replacement_policy_obj,
         _l1_icache_hash_fn_obj,
         l1_dcache_access_delay,
         frequency,
         l1_icache_track_miss_types);
}

L1CacheCntlr::~L1CacheCntlr()
{
   delete _l1_icache;
   delete _l1_dcache;
   delete _l1_icache_replacement_policy_obj;
   delete _l1_dcache_replacement_policy_obj;
   delete _l1_icache_hash_fn_obj;
   delete _l1_dcache_hash_fn_obj;
}      

void
L1CacheCntlr::setL2CacheCntlr(L2CacheCntlr* l2_cache_cntlr)
{
   _l2_cache_cntlr = l2_cache_cntlr;
}

bool
L1CacheCntlr::processMemOpFromTile(MemComponent::Type mem_component,
                                   Core::lock_signal_t lock_signal,
                                   Core::mem_op_t mem_op_type, 
                                   IntPtr ca_address, UInt32 offset,
                                   Byte* data_buf, UInt32 data_length,
                                   bool modeled)
{
   LOG_PRINT("processMemOpFromTile(), lock_signal(%u), mem_op_type(%u), ca_address(0x%x)",
         lock_signal, mem_op_type, ca_address);

   bool l1_cache_hit = true;
   UInt32 access_num = 0;

   while(1)
   {
      access_num ++;
      LOG_ASSERT_ERROR((access_num == 1) || (access_num == 2),
                       "access_num(%u)", access_num);

      if (lock_signal != Core::UNLOCK)
         acquireLock(mem_component);

      // Wake up the network thread after acquiring the lock
      if (access_num == 2)
      {
         wakeUpSimThread();
      }

      if (operationPermissibleinL1Cache(mem_component, ca_address, mem_op_type, access_num))
      {
         // Increment Shared Mem Perf model cycle counts
         // L1 Cache
         getMemoryManager()->incrCycleCount(mem_component, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);

         accessCache(mem_component, mem_op_type, ca_address, offset, data_buf, data_length);
                 
         if (lock_signal != Core::LOCK)
            releaseLock(mem_component);
         return l1_cache_hit;
      }

      getMemoryManager()->incrCycleCount(mem_component, CachePerfModel::ACCESS_CACHE_TAGS);
     
      // Miss in the L1 cache 
      l1_cache_hit = false;
      
      LOG_ASSERT_ERROR(lock_signal != Core::UNLOCK, "Expected to find address(%#lx) in L1 Cache", ca_address);

      // Invalidate the cache line before passing the request to L2 Cache
      invalidateCacheLine(mem_component, ca_address);

      _l2_cache_cntlr->acquireLock();
 
      // (1) Is cache miss? (2) Cache miss type (COLD, CAPACITY, UPGRADE, SHARING)
      pair<bool,Cache::MissType> l2_cache_miss_info = _l2_cache_cntlr->processShmemRequestFromL1Cache(mem_component, mem_op_type, ca_address);
      bool l2_cache_miss = l2_cache_miss_info.first;
      Cache::MissType l2_cache_miss_type = l2_cache_miss_info.second;

      // Is cache hit?
      if (!l2_cache_miss)
      {
         _l2_cache_cntlr->releaseLock();
         
         // Increment Shared Mem Perf model cycle counts
         // L2 Cache
         getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);
         // L1 Cache
         getMemoryManager()->incrCycleCount(mem_component, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);

         accessCache(mem_component, mem_op_type, ca_address, offset, data_buf, data_length);

         if (lock_signal != Core::LOCK)
            releaseLock(mem_component);
         return false;
      }

      // Increment shared mem perf model cycle counts
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_TAGS);
      
      _l2_cache_cntlr->releaseLock();
      releaseLock(mem_component);
    
      // Is the miss type modeled? If yes, all the msgs' created by this miss are modeled 
      bool msg_modeled = ::MemoryManager::isMissTypeModeled(l2_cache_miss_type) &&
                         Config::getSingleton()->isApplicationTile(getMemoryManager()->getTile()->getId());
      LOG_PRINT("msg_modeled(%s), miss type modeled(%s), application tile(%s)",
                msg_modeled ? "TRUE" : "FALSE",
                ::MemoryManager::isMissTypeModeled(l2_cache_miss_type) ? "TRUE" : "FALSE",
                Config::getSingleton()->isApplicationTile(getMemoryManager()->getTile()->getId()) ? "TRUE" : "FALSE");

      ShmemMsg::Type shmem_msg_type = getShmemMsgType(mem_op_type);

      // Construct the message and send out a request to the SIM thread for the cache data
      ShmemMsg shmem_msg(shmem_msg_type, mem_component, MemComponent::L2_CACHE, getTileId(), ca_address, msg_modeled);
      getMemoryManager()->sendMsg(getTileId(), shmem_msg);

      waitForSimThread();
   }

   LOG_PRINT_ERROR("Should not reach here");
   return false;
}

void
L1CacheCntlr::accessCache(MemComponent::Type mem_component,
      Core::mem_op_t mem_op_type, IntPtr ca_address, UInt32 offset,
      Byte* data_buf, UInt32 data_length)
{
   Cache* l1_cache = getL1Cache(mem_component);
   switch (mem_op_type)
   {
   case Core::READ:
   case Core::READ_EX:
      l1_cache->accessCacheLine(ca_address + offset, Cache::LOAD, data_buf, data_length);
      break;

   case Core::WRITE:
      l1_cache->accessCacheLine(ca_address + offset, Cache::STORE, data_buf, data_length);
      // Write-through cache - Write the L2 Cache also
      _l2_cache_cntlr->acquireLock();
      _l2_cache_cntlr->writeCacheLine(ca_address, offset, data_buf, data_length);
      _l2_cache_cntlr->releaseLock();
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Mem Op Type: %u", mem_op_type);
      break;
   }
}

bool
L1CacheCntlr::operationPermissibleinL1Cache(MemComponent::Type mem_component, 
                                            IntPtr address, Core::mem_op_t mem_op_type,
                                            UInt32 access_num)
{
   LOG_PRINT("operationPermissibleinL1Cache[Mem Component(%u), Address(%#llx), MemOp Type(%u), Access Num(%u)]",
             mem_component, address, mem_op_type, access_num);

   bool cache_hit = false;
   CacheState::Type cstate = getCacheLineState(mem_component, address);
   LOG_PRINT("Cache line state(%u)", cstate);

   switch (mem_op_type)
   {
   case Core::READ:
      cache_hit = CacheState(cstate).readable();
      break;

   case Core::READ_EX:
   case Core::WRITE:
      cache_hit = CacheState(cstate).writable();
      break;

   default:
      LOG_PRINT_ERROR("Unsupported mem_op_type: %u", mem_op_type);
      break;
   }

   if (access_num == 1)
   {
      // Update the Cache Counters
      getL1Cache(mem_component)->updateMissCounters(address, mem_op_type, !cache_hit);
   }

   LOG_PRINT("operationPermissibleinL1Cache returns(%s)", cache_hit ? "true" : "false");
   return cache_hit;
}

void
L1CacheCntlr::insertCacheLine(MemComponent::Type mem_component,
                              IntPtr address, CacheState::Type cstate, Byte* fill_buf,
                              bool* eviction, IntPtr* evicted_address)
{
   Cache* l1_cache = getL1Cache(mem_component);
   assert(l1_cache);
   
   PrL1CacheLineInfo evicted_cache_line_info;

   PrL1CacheLineInfo l1_cache_line_info;
   l1_cache_line_info.setTag(l1_cache->getTag(address));
   l1_cache_line_info.setCState(cstate);
   
   l1_cache->insertCacheLine(address, &l1_cache_line_info, fill_buf,
                             eviction, evicted_address, &evicted_cache_line_info, NULL);
}

CacheState::Type
L1CacheCntlr::getCacheLineState(MemComponent::Type mem_component, IntPtr address)
{
   LOG_PRINT("getCacheLineState[Mem Component(%u), Address(%#lx)] start", mem_component, address);

   Cache* l1_cache = getL1Cache(mem_component);
   assert(l1_cache);

   PrL1CacheLineInfo l1_cache_line_info;
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);

   LOG_PRINT("getCacheLineState[Mem Component(%u), Address(%#lx)] returns(%u)", mem_component, address, l1_cache_line_info.getCState());
   return l1_cache_line_info.getCState(); 
}

void
L1CacheCntlr::setCacheLineState(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate)
{
   Cache* l1_cache = getL1Cache(mem_component);

   // Get the old cache line info
   PrL1CacheLineInfo l1_cache_line_info;
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);
   assert(l1_cache_line_info.getCState() != CacheState::INVALID);

   // Set the new cache line info
   l1_cache_line_info.setCState(cstate);
   l1_cache->setCacheLineInfo(address, &l1_cache_line_info);
}

void
L1CacheCntlr::invalidateCacheLine(MemComponent::Type mem_component, IntPtr address)
{
   Cache* l1_cache = getL1Cache(mem_component);

   PrL1CacheLineInfo l1_cache_line_info;
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);
   if (l1_cache_line_info.isValid())
   {
      l1_cache_line_info.invalidate();
      l1_cache->setCacheLineInfo(address, &l1_cache_line_info);
   }
}

ShmemMsg::Type
L1CacheCntlr::getShmemMsgType(Core::mem_op_t mem_op_type)
{
   switch (mem_op_type)
   {
   case Core::READ:
      return ShmemMsg::SH_REQ;

   case Core::READ_EX:
   case Core::WRITE:
      return ShmemMsg::EX_REQ;

   default:
      LOG_PRINT_ERROR("Unsupported Mem Op Type(%u)", mem_op_type);
      return ShmemMsg::INVALID_MSG_TYPE;
   }
}

Cache*
L1CacheCntlr::getL1Cache(MemComponent::Type mem_component)
{
   switch (mem_component)
   {
   case MemComponent::L1_ICACHE:
      return _l1_icache;

   case MemComponent::L1_DCACHE:
      return _l1_dcache;

   default:
      LOG_PRINT_ERROR("Unrecognized Memory Component(%u)", mem_component);
      return NULL;
   }
}

void
L1CacheCntlr::acquireLock(MemComponent::Type mem_component)
{
   switch (mem_component)
   {
   case MemComponent::L1_ICACHE:
      _l1_icache_lock.acquire();
      break;
   case MemComponent::L1_DCACHE:
      _l1_dcache_lock.acquire();
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized mem_component(%u)", mem_component);
      break;
   }

}

void
L1CacheCntlr::releaseLock(MemComponent::Type mem_component)
{
   switch (mem_component)
   {
   case MemComponent::L1_ICACHE:
      _l1_icache_lock.release();
      break;
   case MemComponent::L1_DCACHE:
      _l1_dcache_lock.release();
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized mem_component(%u)", mem_component);
      break;
   }
}

void
L1CacheCntlr::waitForSimThread()
{
   _app_thread_sem->wait();
}

void
L1CacheCntlr::wakeUpSimThread()
{
   _sim_thread_sem->signal();
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
