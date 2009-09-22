#include "memory_manager.h"
#include "cache_base.h"
#include "simulator.h"
#include "log.h"

namespace PrL1PrL2DramDirectory
{

MemoryManager::MemoryManager(Core* core, 
      Network* network, ShmemPerfModel* shmem_perf_model):
   MemoryManagerBase(core, network, shmem_perf_model),
   m_dram_cntlr(NULL),
   m_num_dram_cntlrs(0),
   m_inter_dram_cntlr_distance(0)
{
   // Read Parameters from the Config file
   float core_frequency = 0.0;

   UInt32 l1_icache_size = 0;
   UInt32 l1_icache_associativity = 0;
   std::string l1_icache_replacement_policy;
   UInt32 l1_icache_data_access_time = 0;
   UInt32 l1_icache_tags_access_time = 0;
   std::string l1_icache_perf_model_type;

   UInt32 l1_dcache_size = 0;
   UInt32 l1_dcache_associativity = 0;
   std::string l1_dcache_replacement_policy;
   bool l1_dcache_track_detailed_counters = false;
   UInt32 l1_dcache_data_access_time = 0;
   UInt32 l1_dcache_tags_access_time = 0;
   std::string l1_dcache_perf_model_type;

   UInt32 l2_cache_size = 0;
   UInt32 l2_cache_associativity = 0;
   std::string l2_cache_replacement_policy;
   bool l2_cache_track_detailed_counters = false;
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

   float dram_access_cost = 0.0;
   float total_dram_bandwidth = 0.0;
   bool dram_controller_present_on_every_core = false;
   bool dram_queue_model_enabled = false;
   UInt32 dram_queue_model_moving_avg_window_size = 0;
   std::string dram_queue_model_moving_avg_type;

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
      l1_dcache_track_detailed_counters = Sim()->getCfg()->getBool("perf_model/l1_dcache/track_detailed_cache_counters");
      l1_dcache_data_access_time = Sim()->getCfg()->getInt("perf_model/l1_dcache/data_access_time");
      l1_dcache_tags_access_time = Sim()->getCfg()->getInt("perf_model/l1_dcache/tags_access_time");
      l1_dcache_perf_model_type = Sim()->getCfg()->getString("perf_model/l1_dcache/perf_model_type");

      // L2 Cache
      l2_cache_size = Sim()->getCfg()->getInt("perf_model/l2_cache/cache_size");
      l2_cache_associativity = Sim()->getCfg()->getInt("perf_model/l2_cache/associativity");
      l2_cache_replacement_policy = Sim()->getCfg()->getString("perf_model/l2_cache/replacement_policy");
      l2_cache_track_detailed_counters = Sim()->getCfg()->getBool("perf_model/l2_cache/track_detailed_cache_counters");
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
      dram_queue_model_enabled = Sim()->getCfg()->getBool("perf_model/dram/queue_model_enabled");
      dram_queue_model_moving_avg_window_size = Sim()->getCfg()->getInt("perf_model/dram/moving_avg_window_size");
      dram_queue_model_moving_avg_type = Sim()->getCfg()->getString("perf_model/dram/moving_avg_type");
      
      dram_controller_present_on_every_core = Sim()->getCfg()->getBool("perf_model/dram/controller_present_on_every_core");
      if (dram_controller_present_on_every_core)
         m_num_dram_cntlrs = Config::getSingleton()->getTotalCores();
      else
         m_num_dram_cntlrs = Sim()->getCfg()->getInt("perf_model/dram/num_controllers");
      m_inter_dram_cntlr_distance = (Config::getSingleton()->getTotalCores()) / m_num_dram_cntlrs;


   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error reading memory system parameters from the config file");
   }

   m_mmu_sem = new Semaphore(0);

   m_dram_directory_home_lookup = new AddressHomeLookup(dram_directory_home_lookup_param, Config::getSingleton()->getTotalCores(), getCacheBlockSize());

   m_dram_home_lookup = new AddressHomeLookup(dram_directory_home_lookup_param, m_num_dram_cntlrs, getCacheBlockSize());

   m_l1_cache_cntlr = new L1CacheCntlr(m_core->getId(),
         this,
         m_mmu_sem,
         getCacheBlockSize(),
         l1_icache_size, l1_icache_associativity,
         l1_icache_replacement_policy,
         l1_icache_data_access_time,
         l1_icache_tags_access_time,
         l1_icache_perf_model_type,
         l1_dcache_size, l1_dcache_associativity,
         l1_dcache_replacement_policy,
         l1_dcache_track_detailed_counters,
         l1_dcache_data_access_time,
         l1_dcache_tags_access_time,
         l1_dcache_perf_model_type,
         getShmemPerfModel());
   
   m_l2_cache_cntlr = new L2CacheCntlr(m_core->getId(),
         this,
         m_l1_cache_cntlr,
         m_dram_directory_home_lookup,
         m_mmu_sem,
         getCacheBlockSize(),
         l2_cache_size, l2_cache_associativity,
         l2_cache_replacement_policy,
         l2_cache_track_detailed_counters,
         l2_cache_data_access_time,
         l2_cache_tags_access_time,
         l2_cache_perf_model_type,
         getShmemPerfModel());

   m_l1_cache_cntlr->setL2CacheCntlr(m_l2_cache_cntlr);

   if (isDramCntlrPresent())
   {
      float single_dram_bandwidth = total_dram_bandwidth / m_num_dram_cntlrs;
      m_dram_cntlr = new DramCntlr(this,
            dram_access_cost,
            single_dram_bandwidth,
            core_frequency,
            dram_queue_model_enabled,
            dram_queue_model_moving_avg_window_size,
            dram_queue_model_moving_avg_type,
            getCacheBlockSize(),
            getShmemPerfModel());
   }

   m_dram_directory_cntlr = new DramDirectoryCntlr(m_core->getId(),
         this,
         m_dram_cntlr,
         m_dram_home_lookup,
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

   delete m_mmu_sem;
   delete m_dram_directory_home_lookup;
   delete m_dram_home_lookup;
   delete m_l1_cache_cntlr;
   delete m_l2_cache_cntlr;
   if (m_dram_cntlr)
      delete m_dram_cntlr;
   delete m_dram_directory_cntlr;
}


bool
MemoryManager::coreInitiateMemoryAccess(
      MemComponent::component_t mem_component,
      Core::lock_signal_t lock_signal,
      Core::mem_op_t mem_op_type,
      IntPtr address, UInt32 offset,
      Byte* data_buf, UInt32 data_length)
{
   return m_l1_cache_cntlr->processMemOpFromCore(mem_component, 
         lock_signal, 
         mem_op_type, 
         address, offset, 
         data_buf, data_length);
}

void
MemoryManager::handleMsgFromNetwork(NetPacket& packet)
{
   core_id_t sender = packet.sender;
   ShmemMsg* shmem_msg = ShmemMsg::getShmemMsg((Byte*) packet.data);
   UInt64 msg_time = packet.time;

   getShmemPerfModel()->setCycleCount(msg_time);

   MemComponent::component_t receiver_mem_component = shmem_msg->m_receiver_mem_component;
   MemComponent::component_t sender_mem_component = shmem_msg->m_sender_mem_component;

   LOG_PRINT("Got Shmem Msg: type(%i), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), sender(%i), receiver(%i)", shmem_msg->m_msg_type, shmem_msg->m_address, sender_mem_component, receiver_mem_component, sender, packet.receiver);    

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

            case MemComponent::DRAM:
               m_dram_directory_cntlr->handleMsgFromDram(shmem_msg);
               break;

            default:
               LOG_PRINT_ERROR("Unrecognized sender component(%u)",
                     sender_mem_component);
               break;
         }
         break;

      case MemComponent::DRAM:
         LOG_ASSERT_ERROR(sender_mem_component == MemComponent::DRAM_DIR,
               "Unrecognized sender component(%u)", sender_mem_component)
         assert(m_dram_cntlr);
         m_dram_cntlr->handleMsgFromDramDir(sender, shmem_msg);
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized receiver component(%u)",
               receiver_mem_component);
         break;
   }

   // Delete the allocated Shared Memory Message
   // First delete 'data_buf' if it is present
   LOG_PRINT("Finished handling Shmem Msg");

   if (shmem_msg->m_data_length > 0)
   {
      assert(shmem_msg->m_data_buf);
      delete [] shmem_msg->m_data_buf;
   }
   delete shmem_msg;
}

void
MemoryManager::sendMsg(ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, MemComponent::component_t receiver_mem_component, core_id_t receiver, IntPtr address, Byte* data_buf, UInt32 data_length)
{
   assert((data_buf == NULL) == (data_length == 0));
   ShmemMsg shmem_msg(msg_type, sender_mem_component, receiver_mem_component, address, data_buf, data_length);

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   LOG_PRINT("Sending Msg: type(%u), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), sender(%i), receiver(%i)", msg_type, address, sender_mem_component, receiver_mem_component, m_core->getId(), receiver);

   // TODO: Change the constructors of NetPacket
   NetPacket packet(msg_time, SHARED_MEM_NET1,
         m_core->getId(), receiver,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   m_network->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::broadcastMsg(ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, MemComponent::component_t receiver_mem_component, IntPtr address, Byte* data_buf, UInt32 data_length)
{
   assert((data_buf == NULL) == (data_length == 0));
   ShmemMsg shmem_msg(msg_type, sender_mem_component, receiver_mem_component, address, data_buf, data_length);

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   LOG_PRINT("Sending Msg: type(%u), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), sender(%i), receiver(%i)", msg_type, address, sender_mem_component, receiver_mem_component, m_core->getId(), NetPacket::BROADCAST);

   // TODO: Change the constructors of NetPacket
   NetPacket packet(msg_time, SHARED_MEM_NET1,
         m_core->getId(), NetPacket::BROADCAST,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   m_network->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

core_id_t
MemoryManager::getCoreIdFromDramCntlrNum(SInt32 dram_cntlr_num)
{
   assert(dram_cntlr_num < (SInt32) m_num_dram_cntlrs);
   return dram_cntlr_num * m_inter_dram_cntlr_distance;
}

bool 
MemoryManager::isDramCntlrPresent()
{
   if ((m_core->getId() % m_inter_dram_cntlr_distance == 0) &&
      (m_core->getId() < (SInt32) (m_inter_dram_cntlr_distance * m_num_dram_cntlrs)))
      return true;
   else
      return false;
}

}
