#include "memory_manager.h"
#include "cache_base.h"
#include "simulator.h"
#include "log.h"

namespace PrL1PrL2DramDirectory
{

MemoryManager::MemoryManager(Core* core, 
      Network* network, ShmemPerfModel* shmem_perf_model):
   MemoryManagerBase(core, network, shmem_perf_model)
{
   // Read Parameters from the Config file
   volatile float core_frequency = 0.0;

   UInt32 l1_icache_size = 0;
   UInt32 l1_icache_associativity = 0;
   std::string l1_icache_replacement_policy;
   UInt32 l1_icache_data_access_time = 0;
   UInt32 l1_icache_tags_access_time = 0;
   std::string l1_icache_perf_model_type;

   UInt32 l1_dcache_size = 0;
   UInt32 l1_dcache_associativity = 0;
   std::string l1_dcache_replacement_policy;
   UInt32 l1_dcache_data_access_time = 0;
   UInt32 l1_dcache_tags_access_time = 0;
   std::string l1_dcache_perf_model_type;

   UInt32 l2_cache_size = 0;
   UInt32 l2_cache_associativity = 0;
   std::string l2_cache_replacement_policy;
   UInt32 l2_cache_data_access_time = 0;
   UInt32 l2_cache_tags_access_time = 0;
   std::string l2_cache_perf_model_type;

   UInt32 dram_directory_total_entries = 0;
   UInt32 dram_directory_associativity = 0;
   UInt32 dram_directory_max_num_sharers = 0;
   UInt32 dram_directory_max_hw_sharers = 0;
   std::string dram_directory_type_str;
   UInt32 dram_directory_home_lookup_param = 0;
   UInt32 dram_directory_cache_access_time = 0;

   volatile float dram_access_cost = 0.0;
   volatile float total_dram_bandwidth = 0.0;
   bool dram_queue_model_enabled = false;
   std::string dram_queue_model_type;

   try
   {
      core_frequency = Sim()->getCfg()->getFloat("perf_model/core/frequency");

      m_cache_block_size = Sim()->getCfg()->getInt("perf_model/l1_icache/cache_block_size");

      // L1 ICache
      l1_icache_size = Sim()->getCfg()->getInt("perf_model/l1_icache/cache_size");
      l1_icache_associativity = Sim()->getCfg()->getInt("perf_model/l1_icache/associativity");
      l1_icache_replacement_policy = Sim()->getCfg()->getString("perf_model/l1_icache/replacement_policy");
      l1_icache_data_access_time = Sim()->getCfg()->getInt("perf_model/l1_icache/data_access_time");
      l1_icache_tags_access_time = Sim()->getCfg()->getInt("perf_model/l1_icache/tags_access_time");
      l1_icache_perf_model_type = Sim()->getCfg()->getString("perf_model/l1_icache/perf_model_type");

      // L1 DCache
      l1_dcache_size = Sim()->getCfg()->getInt("perf_model/l1_dcache/cache_size");
      l1_dcache_associativity = Sim()->getCfg()->getInt("perf_model/l1_dcache/associativity");
      l1_dcache_replacement_policy = Sim()->getCfg()->getString("perf_model/l1_dcache/replacement_policy");
      l1_dcache_data_access_time = Sim()->getCfg()->getInt("perf_model/l1_dcache/data_access_time");
      l1_dcache_tags_access_time = Sim()->getCfg()->getInt("perf_model/l1_dcache/tags_access_time");
      l1_dcache_perf_model_type = Sim()->getCfg()->getString("perf_model/l1_dcache/perf_model_type");

      // L2 Cache
      l2_cache_size = Sim()->getCfg()->getInt("perf_model/l2_cache/cache_size");
      l2_cache_associativity = Sim()->getCfg()->getInt("perf_model/l2_cache/associativity");
      l2_cache_replacement_policy = Sim()->getCfg()->getString("perf_model/l2_cache/replacement_policy");
      l2_cache_data_access_time = Sim()->getCfg()->getInt("perf_model/l2_cache/data_access_time");
      l2_cache_tags_access_time = Sim()->getCfg()->getInt("perf_model/l2_cache/tags_access_time");
      l2_cache_perf_model_type = Sim()->getCfg()->getString("perf_model/l2_cache/perf_model_type");

      // Dram Directory Cache
      dram_directory_total_entries = Sim()->getCfg()->getInt("perf_model/dram_directory/total_entries");
      dram_directory_associativity = Sim()->getCfg()->getInt("perf_model/dram_directory/associativity");
      dram_directory_max_num_sharers = Sim()->getConfig()->getTotalCores();
      dram_directory_max_hw_sharers = Sim()->getCfg()->getInt("perf_model/dram_directory/max_hw_sharers");
      dram_directory_type_str = Sim()->getCfg()->getString("perf_model/dram_directory/directory_type");
      dram_directory_home_lookup_param = Sim()->getCfg()->getInt("perf_model/dram_directory/home_lookup_param");
      dram_directory_cache_access_time = Sim()->getCfg()->getInt("perf_model/dram_directory/directory_cache_access_time");

      // Dram Cntlr
      dram_access_cost = Sim()->getCfg()->getFloat("perf_model/dram/access_cost");
      total_dram_bandwidth = Sim()->getCfg()->getFloat("perf_model/dram/total_bandwidth");
      dram_queue_model_enabled = Sim()->getCfg()->getBool("perf_model/dram/queue_model/enabled");
      dram_queue_model_type = Sim()->getCfg()->getString("perf_model/dram/queue_model/type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error reading memory system parameters from the config file");
   }

   m_user_thread_sem = new Semaphore(0);
   m_network_thread_sem = new Semaphore(0);

   m_dram_directory_home_lookup = new AddressHomeLookup(dram_directory_home_lookup_param, Config::getSingleton()->getTotalCores(), getCacheBlockSize());

   m_l1_cache_cntlr = new L1CacheCntlr(m_core->getId(),
         this,
         m_user_thread_sem,
         m_network_thread_sem,
         getCacheBlockSize(),
         l1_icache_size, l1_icache_associativity,
         l1_icache_replacement_policy,
         l1_icache_data_access_time,
         l1_icache_tags_access_time,
         l1_icache_perf_model_type,
         l1_dcache_size, l1_dcache_associativity,
         l1_dcache_replacement_policy,
         l1_dcache_data_access_time,
         l1_dcache_tags_access_time,
         l1_dcache_perf_model_type,
         getShmemPerfModel());
   
   m_l2_cache_cntlr = new L2CacheCntlr(m_core->getId(),
         this,
         m_l1_cache_cntlr,
         m_dram_directory_home_lookup,
         m_user_thread_sem,
         m_network_thread_sem,
         getCacheBlockSize(),
         l2_cache_size, l2_cache_associativity,
         l2_cache_replacement_policy,
         l2_cache_data_access_time,
         l2_cache_tags_access_time,
         l2_cache_perf_model_type,
         getShmemPerfModel());

   m_l1_cache_cntlr->setL2CacheCntlr(m_l2_cache_cntlr);

   volatile float single_dram_bandwidth = total_dram_bandwidth / Config::getSingleton()->getTotalCores();
   m_dram_cntlr = new DramCntlr(this,
         dram_access_cost,
         single_dram_bandwidth,
         core_frequency,
         dram_queue_model_enabled,
         dram_queue_model_type,
         getCacheBlockSize(),
         getShmemPerfModel());

   m_dram_directory_cntlr = new DramDirectoryCntlr(m_core->getId(),
         this,
         m_dram_cntlr,
         dram_directory_total_entries,
         dram_directory_associativity,
         getCacheBlockSize(),
         dram_directory_max_num_sharers,
         dram_directory_max_hw_sharers,
         dram_directory_type_str,
         dram_directory_cache_access_time,
         getShmemPerfModel());

   // Register Call-backs
   m_network->registerCallback(SHARED_MEM_NET1, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(SHARED_MEM_NET2, MemoryManagerNetworkCallback, this);
}

MemoryManager::~MemoryManager()
{
   m_network->unregisterCallback(SHARED_MEM_NET1);
   m_network->unregisterCallback(SHARED_MEM_NET2);

   delete m_user_thread_sem;
   delete m_network_thread_sem;
   delete m_dram_directory_home_lookup;
   delete m_l1_cache_cntlr;
   delete m_l2_cache_cntlr;
   delete m_dram_cntlr;
   delete m_dram_directory_cntlr;
}


bool
MemoryManager::coreInitiateMemoryAccess(
      MemComponent::component_t mem_component,
      Core::lock_signal_t lock_signal,
      Core::mem_op_t mem_op_type,
      IntPtr address, UInt32 offset,
      Byte* data_buf, UInt32 data_length,
      bool modeled)
{
   return m_l1_cache_cntlr->processMemOpFromCore(mem_component, 
         lock_signal, 
         mem_op_type, 
         address, offset, 
         data_buf, data_length,
         modeled);
}

void
MemoryManager::handleMsgFromNetwork(NetPacket& packet)
{
   core_id_t sender = packet.sender;
   ShmemMsg* shmem_msg = ShmemMsg::getShmemMsg((Byte*) packet.data);
   UInt64 msg_time = packet.time;

   getShmemPerfModel()->setCycleCount(msg_time);

   MemComponent::component_t receiver_mem_component = shmem_msg->getReceiverMemComponent();
   MemComponent::component_t sender_mem_component = shmem_msg->getSenderMemComponent();

   LOG_PRINT("Got Shmem Msg: type(%i), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), sender(%i), receiver(%i)", 
         shmem_msg->getMsgType(), shmem_msg->getAddress(), sender_mem_component, receiver_mem_component, sender, packet.receiver);    

   switch (receiver_mem_component)
   {
      case MemComponent::L2_CACHE:
         switch(sender_mem_component)
         {
            case MemComponent::L1_ICACHE:
            case MemComponent::L1_DCACHE:
               assert(sender == m_core->getId());
               m_l2_cache_cntlr->handleMsgFromL1Cache(shmem_msg);
               break;

            case MemComponent::DRAM_DIR:
               m_l2_cache_cntlr->handleMsgFromDramDirectory(sender, shmem_msg);
               break;

            default:
               LOG_PRINT_ERROR("Unrecognized sender component(%u)",
                     sender_mem_component);
               break;
         }
         break;

      case MemComponent::DRAM_DIR:
         switch(sender_mem_component)
         {
            case MemComponent::L2_CACHE:
               m_dram_directory_cntlr->handleMsgFromL2Cache(sender, shmem_msg, msg_time);
               break;

            default:
               LOG_PRINT_ERROR("Unrecognized sender component(%u)",
                     sender_mem_component);
               break;
         }
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized receiver component(%u)",
               receiver_mem_component);
         break;
   }

   // Delete the allocated Shared Memory Message
   // First delete 'data_buf' if it is present
   LOG_PRINT("Finished handling Shmem Msg");

   if (shmem_msg->getDataLength() > 0)
   {
      assert(shmem_msg->getDataBuf());
      delete [] shmem_msg->getDataBuf();
   }
   delete shmem_msg;
}

void
MemoryManager::sendMsg(ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, MemComponent::component_t receiver_mem_component, core_id_t requester, core_id_t receiver, IntPtr address, Byte* data_buf, UInt32 data_length)
{
   assert((data_buf == NULL) == (data_length == 0));
   ShmemMsg shmem_msg(msg_type, sender_mem_component, receiver_mem_component, requester, address, data_buf, data_length);

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   LOG_PRINT("Sending Msg: type(%u), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), requester(%i), sender(%i), receiver(%i)", msg_type, address, sender_mem_component, receiver_mem_component, requester, m_core->getId(), receiver);

   NetPacket packet(msg_time, SHARED_MEM_NET1,
         m_core->getId(), receiver,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   m_network->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::broadcastMsg(ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, MemComponent::component_t receiver_mem_component, core_id_t requester, IntPtr address, Byte* data_buf, UInt32 data_length)
{
   assert((data_buf == NULL) == (data_length == 0));
   ShmemMsg shmem_msg(msg_type, sender_mem_component, receiver_mem_component, requester, address, data_buf, data_length);

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   LOG_PRINT("Sending Msg: type(%u), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), requester(%i), sender(%i), receiver(%i)", msg_type, address, sender_mem_component, receiver_mem_component, requester, m_core->getId(), NetPacket::BROADCAST);

   NetPacket packet(msg_time, SHARED_MEM_NET1,
         m_core->getId(), NetPacket::BROADCAST,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   m_network->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::enableModels()
{
   m_l1_cache_cntlr->getL1ICache()->enable();
   m_l1_cache_cntlr->getL1DCache()->enable();
   m_l2_cache_cntlr->getL2Cache()->enable();

   m_dram_cntlr->getDramPerfModel()->enable();
}

void
MemoryManager::disableModels()
{
   m_l1_cache_cntlr->getL1ICache()->disable();
   m_l1_cache_cntlr->getL1DCache()->disable();
   m_l2_cache_cntlr->getL2Cache()->disable();

   m_dram_cntlr->getDramPerfModel()->disable();
}

void
MemoryManager::outputSummary(std::ostream &os)
{
   os << "Cache Summary:\n";
   m_l1_cache_cntlr->getL1ICache()->outputSummary(os);
   m_l1_cache_cntlr->getL1DCache()->outputSummary(os);
   m_l2_cache_cntlr->getL2Cache()->outputSummary(os);

   m_dram_cntlr->getDramPerfModel()->outputSummary(os);
}

}
