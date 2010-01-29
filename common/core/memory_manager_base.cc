#include "memory_manager_base.h"
#include "pr_l1_pr_l2_dram_directory/memory_manager.h"
#include "log.h"

MemoryManagerBase* 
MemoryManagerBase::createMMU(CachingProtocol_t caching_protocol,
      Core* core, Network* network, ShmemPerfModel* shmem_perf_model)
{
   switch (caching_protocol)
   {
      case PR_L1_PR_L2_DRAM_DIR:
         return new PrL1PrL2DramDirectory::MemoryManager(core, network, shmem_perf_model);

      default:
         LOG_PRINT_ERROR("Unsupported Caching Protocol (%u)", caching_protocol);
         return NULL;
   }
}

void MemoryManagerNetworkCallback(void* obj, NetPacket packet)
{
   MemoryManagerBase *mm = (MemoryManagerBase*) obj;
   assert(mm != NULL);

   switch (packet.type)
   {
      case SHARED_MEM_1:
      case SHARED_MEM_2:
         mm->handleMsgFromNetwork(packet);
         break;

      default:
         LOG_PRINT_ERROR("Got unrecognized packet type(%u)", packet.type);
         break;
   }
}
