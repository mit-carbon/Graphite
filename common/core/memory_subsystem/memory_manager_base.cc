#include "memory_manager_base.h"
#include "pr_l1_pr_l2_dram_directory_msi/memory_manager.h"
#include "pr_l1_pr_l2_dram_directory_mosi/memory_manager.h"
#include "log.h"

MemoryManagerBase* 
MemoryManagerBase::createMMU(std::string protocol_type,
      Core* core, Network* network, ShmemPerfModel* shmem_perf_model)
{
   CachingProtocol_t caching_protocol = parseProtocolType(protocol_type);

   switch (caching_protocol)
   {
      case PR_L1_PR_L2_DRAM_DIRECTORY_MSI:
         return new PrL1PrL2DramDirectoryMSI::MemoryManager(core, network, shmem_perf_model);

      case PR_L1_PR_L2_DRAM_DIRECTORY_MOSI:
         return new PrL1PrL2DramDirectoryMOSI::MemoryManager(core, network, shmem_perf_model);

      default:
         LOG_PRINT_ERROR("Unsupported Caching Protocol (%u)", caching_protocol);
         return NULL;
   }
}

MemoryManagerBase::CachingProtocol_t
MemoryManagerBase::parseProtocolType(std::string& protocol_type)
{
   if (protocol_type == "msi")
      return PR_L1_PR_L2_DRAM_DIRECTORY_MSI;
   else if (protocol_type == "mosi")
      return PR_L1_PR_L2_DRAM_DIRECTORY_MOSI;
   else
      return NUM_CACHING_PROTOCOL_TYPES;
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
