#include "memory_manager.h"
#include "cache.h"
#include "simulator.h"
#include "tile_manager.h"
#include "l2_directory_cfg.h"
#include "network.h"
#include "log.h"

namespace PrL1ShL2MSI
{

MemoryManager::MemoryManager(Tile* tile)
   : ::MemoryManager(tile)
   , _dram_cntlr(NULL)
   , _dram_cntlr_present(false)
{
   // Read Parameters from the Config file
   std::string L1_icache_type;
   UInt32 L1_icache_line_size = 0;
   UInt32 L1_icache_size = 0;
   UInt32 L1_icache_associativity = 0;
   UInt32 L1_icache_num_banks = 0;
   std::string L1_icache_replacement_policy;
   UInt32 L1_icache_data_access_cycles = 0;
   UInt32 L1_icache_tags_access_cycles = 0;
   std::string L1_icache_perf_model_type;
   bool L1_icache_track_miss_types = false;

   std::string L1_dcache_type;
   UInt32 L1_dcache_line_size = 0;
   UInt32 L1_dcache_size = 0;
   UInt32 L1_dcache_associativity = 0;
   UInt32 L1_dcache_num_banks = 0;
   std::string L1_dcache_replacement_policy;
   UInt32 L1_dcache_data_access_cycles = 0;
   UInt32 L1_dcache_tags_access_cycles = 0;
   std::string L1_dcache_perf_model_type;
   bool L1_dcache_track_miss_types = false;

   std::string L2_cache_type;
   UInt32 L2_cache_line_size = 0;
   UInt32 L2_cache_size = 0;
   UInt32 L2_cache_associativity = 0;
   UInt32 L2_cache_num_banks = 0;
   std::string L2_cache_replacement_policy;
   UInt32 L2_cache_data_access_cycles = 0;
   UInt32 L2_cache_tags_access_cycles = 0;
   std::string L2_cache_perf_model_type;
   bool L2_cache_track_miss_types = false;
   
   // L2 Directory
   SInt32 L2_directory_max_num_sharers = 0;
   SInt32 L2_directory_max_hw_sharers = 0;
   std::string L2_directory_type_str;
   
   // Dram
   float dram_latency = 0.0;
   float per_dram_controller_bandwidth = 0.0;
   bool dram_queue_model_enabled = false;
   std::string dram_queue_model_type;

   try
   {
      // L1 ICache
      L1_icache_type = "l1_icache/" + Config::getSingleton()->getL1ICacheType(getTile()->getId());
      L1_icache_line_size = Sim()->getCfg()->getInt(L1_icache_type + "/cache_line_size");
      L1_icache_size = Sim()->getCfg()->getInt(L1_icache_type + "/cache_size");
      L1_icache_associativity = Sim()->getCfg()->getInt(L1_icache_type + "/associativity");
      L1_icache_num_banks = Sim()->getCfg()->getInt(L1_icache_type + "/num_banks");
      L1_icache_replacement_policy = Sim()->getCfg()->getString(L1_icache_type + "/replacement_policy");
      L1_icache_data_access_cycles = Sim()->getCfg()->getInt(L1_icache_type + "/data_access_time");
      L1_icache_tags_access_cycles = Sim()->getCfg()->getInt(L1_icache_type + "/tags_access_time");
      L1_icache_perf_model_type = Sim()->getCfg()->getString(L1_icache_type + "/perf_model_type");
      L1_icache_track_miss_types = Sim()->getCfg()->getBool(L1_icache_type + "/track_miss_types");

      // L1 DCache
      L1_dcache_type = "l1_dcache/" + Config::getSingleton()->getL1DCacheType(getTile()->getId());
      L1_dcache_line_size = Sim()->getCfg()->getInt(L1_dcache_type + "/cache_line_size");
      L1_dcache_size = Sim()->getCfg()->getInt(L1_dcache_type + "/cache_size");
      L1_dcache_associativity = Sim()->getCfg()->getInt(L1_dcache_type + "/associativity");
      L1_dcache_num_banks = Sim()->getCfg()->getInt(L1_dcache_type + "/num_banks");
      L1_dcache_replacement_policy = Sim()->getCfg()->getString(L1_dcache_type + "/replacement_policy");
      L1_dcache_data_access_cycles = Sim()->getCfg()->getInt(L1_dcache_type + "/data_access_time");
      L1_dcache_tags_access_cycles = Sim()->getCfg()->getInt(L1_dcache_type + "/tags_access_time");
      L1_dcache_perf_model_type = Sim()->getCfg()->getString(L1_dcache_type + "/perf_model_type");
      L1_dcache_track_miss_types = Sim()->getCfg()->getBool(L1_dcache_type + "/track_miss_types");

      // L2 Cache
      L2_cache_type = "l2_cache/" + Config::getSingleton()->getL2CacheType(getTile()->getId());
      L2_cache_line_size = Sim()->getCfg()->getInt(L2_cache_type + "/cache_line_size");
      L2_cache_size = Sim()->getCfg()->getInt(L2_cache_type + "/cache_size");
      L2_cache_associativity = Sim()->getCfg()->getInt(L2_cache_type + "/associativity");
      L2_cache_num_banks = Sim()->getCfg()->getInt(L2_cache_type + "/num_banks");
      L2_cache_replacement_policy = Sim()->getCfg()->getString(L2_cache_type + "/replacement_policy");
      L2_cache_data_access_cycles = Sim()->getCfg()->getInt(L2_cache_type + "/data_access_time");
      L2_cache_tags_access_cycles = Sim()->getCfg()->getInt(L2_cache_type + "/tags_access_time");
      L2_cache_perf_model_type = Sim()->getCfg()->getString(L2_cache_type + "/perf_model_type");
      L2_cache_track_miss_types = Sim()->getCfg()->getBool(L2_cache_type + "/track_miss_types");

      // Directory
      L2_directory_max_num_sharers = Sim()->getConfig()->getTotalTiles();
      L2_directory_max_hw_sharers = Sim()->getCfg()->getInt("l2_directory/max_hw_sharers");
      L2_directory_type_str = Sim()->getCfg()->getString("l2_directory/directory_type");

      // Dram Cntlr
      dram_latency = Sim()->getCfg()->getFloat("dram/latency");
      per_dram_controller_bandwidth = Sim()->getCfg()->getFloat("dram/per_controller_bandwidth");
      dram_queue_model_enabled = Sim()->getCfg()->getBool("dram/queue_model/enabled");
      dram_queue_model_type = Sim()->getCfg()->getString("dram/queue_model/type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error reading memory system parameters from the config file");
   }

   // Check if all cache line sizes are the same
   LOG_ASSERT_ERROR((L1_icache_line_size == L1_dcache_line_size) && (L1_dcache_line_size == L2_cache_line_size),
                    "Cache Line Sizes of L1-I, L1-D and L2 Caches must be the same. "
                    "Currently, L1-I Cache Line Size(%u), L1-D Cache Line Size(%u), L2 Cache Line Size(%u)",
                    L1_icache_line_size, L1_dcache_line_size, L2_cache_line_size);
   
   _cache_line_size = L1_icache_line_size;

   UInt32 dram_home_lookup_param = ceilLog2(_cache_line_size);
   std::vector<tile_id_t> tile_list_with_dram_controllers = getTileListWithMemoryControllers();
   _dram_home_lookup = new AddressHomeLookup(dram_home_lookup_param, tile_list_with_dram_controllers, getCacheLineSize());
   
   UInt32 L2_cache_home_lookup_param = ceilLog2(_cache_line_size);
   std::vector<tile_id_t> tile_list;
   for (tile_id_t i = 0; i < (tile_id_t) Config::getSingleton()->getApplicationTiles(); i++)
      tile_list.push_back(i);
   _L2_cache_home_lookup = new AddressHomeLookup(L2_cache_home_lookup_param, tile_list, getCacheLineSize());

   if (find(tile_list_with_dram_controllers.begin(), tile_list_with_dram_controllers.end(), getTile()->getId())
         != tile_list_with_dram_controllers.end())
   {
      _dram_cntlr_present = true;

      _dram_cntlr = new DramCntlr(this,
            dram_latency,
            per_dram_controller_bandwidth,
            dram_queue_model_enabled,
            dram_queue_model_type,
            getCacheLineSize());
   }
   
   // Set L2 directory params
   L2DirectoryCfg::setDirectoryType(DirectoryEntry::parseDirectoryType(L2_directory_type_str));
   L2DirectoryCfg::setMaxHWSharers(L2_directory_max_hw_sharers);
   L2DirectoryCfg::setMaxNumSharers(L2_directory_max_num_sharers);

   // Instantiate L1 cache cntlr
   _L1_cache_cntlr = new L1CacheCntlr(this,
         _L2_cache_home_lookup,
         getCacheLineSize(),
         L1_icache_size,
         L1_icache_associativity,
         L1_icache_num_banks,
         L1_icache_replacement_policy,
         L1_icache_data_access_cycles,
         L1_icache_tags_access_cycles,
         L1_icache_perf_model_type,
         L1_icache_track_miss_types,
         L1_dcache_size,
         L1_dcache_associativity,
         L1_dcache_num_banks,
         L1_dcache_replacement_policy,
         L1_dcache_data_access_cycles,
         L1_dcache_tags_access_cycles,
         L1_dcache_perf_model_type,
         L1_dcache_track_miss_types);
   
   // Instantiate L2 cache cntlr
   _L2_cache_cntlr = new L2CacheCntlr(this,
         _dram_home_lookup,
         getCacheLineSize(),
         L2_cache_size,
         L2_cache_associativity,
         L2_cache_num_banks,
         L2_cache_replacement_policy,
         L2_cache_data_access_cycles,
         L2_cache_tags_access_cycles,
         L2_cache_perf_model_type,
         L2_cache_track_miss_types);
}

MemoryManager::~MemoryManager()
{
   // Delete home lookup functions
   delete _dram_home_lookup;
   delete _L2_cache_home_lookup;

   // Delele L1,L2 Cache Cntlr, Dram Cntlr
   delete _L1_cache_cntlr;
   delete _L2_cache_cntlr;
   if (_dram_cntlr_present)
   {
      delete _dram_cntlr;
   }
}

bool
MemoryManager::coreInitiateMemoryAccess(MemComponent::Type mem_component,
                                        Core::lock_signal_t lock_signal,
                                        Core::mem_op_t mem_op_type,
                                        IntPtr address, UInt32 offset,
                                        Byte* data_buf, UInt32 data_length,
                                        bool modeled)
{
   return _L1_cache_cntlr->processMemOpFromCore(mem_component, lock_signal, mem_op_type,
                                                address, offset, data_buf, data_length,
                                                modeled);
}

void
MemoryManager::handleMsgFromNetwork(NetPacket& packet)
{
   core_id_t sender = packet.sender;
   ShmemMsg* shmem_msg = ShmemMsg::getShmemMsg((Byte*) packet.data);
   Time msg_time = packet.time;

   MemComponent::Type receiver_mem_component = shmem_msg->getReceiverMemComponent();
   MemComponent::Type sender_mem_component = shmem_msg->getSenderMemComponent();

   LOG_PRINT("Time(%llu), Got Shmem Msg: type(%i), address(%#lx), sender_mem_component(%u), receiver_mem_component(%u), "
             "sender(%i,%i), receiver(%i,%i), modeled(%s)", 
             msg_time.toNanosec(), shmem_msg->getType(), shmem_msg->getAddress(),
             sender_mem_component, receiver_mem_component,
             sender.tile_id, sender.core_type, packet.receiver.tile_id, packet.receiver.core_type,
             shmem_msg->isModeled() ? "TRUE" : "FALSE");

   switch (receiver_mem_component)
   {
   case MemComponent::L1_ICACHE:
   case MemComponent::L1_DCACHE:
      switch (sender_mem_component)
      {
      case MemComponent::CORE:
         assert(sender.tile_id == getTile()->getId());
         _L1_cache_cntlr->handleMsgFromCore(shmem_msg);
         break;
      case MemComponent::L2_CACHE:
         _L1_cache_cntlr->handleMsgFromL2Cache(sender.tile_id, shmem_msg);
         break;
      default:
         LOG_PRINT_ERROR("Unrecognized sender component(%u)", sender_mem_component);
         break;
      }
      break;

   case MemComponent::L2_CACHE:
      switch(sender_mem_component)
      {
      case MemComponent::L1_ICACHE:
      case MemComponent::L1_DCACHE:
         _L2_cache_cntlr->handleMsgFromL1Cache(sender.tile_id, shmem_msg);
         break;
      case MemComponent::DRAM_CNTLR:
         _L2_cache_cntlr->handleMsgFromDram(sender.tile_id, shmem_msg);
         break;
      default:
         LOG_PRINT_ERROR("Unrecognized sender component(%u)", sender_mem_component);
         break;
      }
      break;

   case MemComponent::DRAM_CNTLR:
      LOG_ASSERT_ERROR(_dram_cntlr_present, "Dram Cntlr NOT present");
      switch(sender_mem_component)
      {
      case MemComponent::L2_CACHE:
         _dram_cntlr->handleMsgFromL2Cache(sender.tile_id, shmem_msg);
         break;
      default:
         LOG_PRINT_ERROR("Unrecognized sender component(%u)", sender_mem_component);
         break;
      }
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized receiver component(%u)", receiver_mem_component);
      break;
   }

   // Delete the allocated Shared Memory Message
   // First delete 'data_buf' if it is present

   if (shmem_msg->getDataLength() > 0)
   {
      assert(shmem_msg->getDataBuf());
      delete [] shmem_msg->getDataBuf();
   }
   delete shmem_msg;
}

void
MemoryManager::sendMsg(tile_id_t receiver, ShmemMsg& shmem_msg)
{
   LOG_ASSERT_ERROR((shmem_msg.getDataBuf() == NULL) == (shmem_msg.getDataLength() == 0),
                    "Address(%#lx), Type(%u), Sender Component(%u), Receiver Component(%u)",
                    shmem_msg.getAddress(), shmem_msg.getType(), shmem_msg.getSenderMemComponent(), shmem_msg.getReceiverMemComponent());

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   Time msg_time = getShmemPerfModel()->getCurrTime();

   LOG_PRINT("Time(%llu), Sending Msg: type(%u), address(%#lx), sender_mem_component(%u), receiver_mem_component(%u), "
             "requester(%i), sender(%i), receiver(%i), modeled(%s)",
             msg_time.toNanosec(), shmem_msg.getType(), shmem_msg.getAddress(),
             shmem_msg.getSenderMemComponent(), shmem_msg.getReceiverMemComponent(),
             shmem_msg.getRequester(), getTile()->getId(), receiver,
             shmem_msg.isModeled() ? "TRUE" : "FALSE");

   NetPacket packet(msg_time, SHARED_MEM,
         getTile()->getId(), receiver,
         shmem_msg.getMsgLen(), (const void*) msg_buf);

   if ((getTile()->getId() == receiver) || (shmem_msg.getSenderMemComponent() == MemComponent::DRAM_CNTLR)){
      getNetwork()->netSend(packet);
   }
   else{
      getNetwork()->netSend(DVFSManager::convertToModule(shmem_msg.getSenderMemComponent()), packet);
   }

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::broadcastMsg(ShmemMsg& shmem_msg)
{
   assert((shmem_msg.getDataBuf() == NULL) == (shmem_msg.getDataLength() == 0));

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   Time msg_time = getShmemPerfModel()->getCurrTime();

   LOG_PRINT("Time(%llu), Broadcasting Msg: type(%u), address(%#lx), sender_mem_component(%u), receiver_mem_component(%u), "
             "requester(%i), sender(%i), modeled(%s)",
             msg_time.toNanosec(), shmem_msg.getType(), shmem_msg.getAddress(),
             shmem_msg.getSenderMemComponent(), shmem_msg.getReceiverMemComponent(),
             shmem_msg.getRequester(), getTile()->getId(),
             shmem_msg.isModeled() ? "TRUE" : "FALSE");

   NetPacket packet(msg_time, SHARED_MEM,
         getTile()->getId(), NetPacket::BROADCAST,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   getNetwork()->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::incrCurrTime(MemComponent::Type mem_component, CachePerfModel::AccessType access_type)
{
   switch (mem_component)
   {
   case MemComponent::L1_ICACHE:
      getShmemPerfModel()->incrCurrTime(_L1_cache_cntlr->getL1ICache()->getPerfModel()->getLatency(access_type));
      break;

   case MemComponent::L1_DCACHE:
      getShmemPerfModel()->incrCurrTime(_L1_cache_cntlr->getL1DCache()->getPerfModel()->getLatency(access_type));
      break;

   case MemComponent::L2_CACHE:
      getShmemPerfModel()->incrCurrTime(_L2_cache_cntlr->getL2Cache()->getPerfModel()->getLatency(access_type));
      break;

   case MemComponent::INVALID:
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized mem component type(%s)", SPELL_MEMCOMP(mem_component));
      break;
   }
}

void
MemoryManager::enableModels()
{
   _L1_cache_cntlr->getL1ICache()->enable();
   _L1_cache_cntlr->getL1DCache()->enable();
   _L2_cache_cntlr->getL2Cache()->enable();

   _L2_cache_cntlr->enable();

   if (_dram_cntlr_present)
   {
      _dram_cntlr->getDramPerfModel()->enable();
   }

   ::MemoryManager::enableModels();
}

void
MemoryManager::disableModels()
{
   _L1_cache_cntlr->getL1ICache()->disable();
   _L1_cache_cntlr->getL1DCache()->disable();
   _L2_cache_cntlr->getL2Cache()->disable();
   _L2_cache_cntlr->disable();

   if (_dram_cntlr_present)
   {
      _dram_cntlr->getDramPerfModel()->disable();
   }

   ::MemoryManager::disableModels();
}

void
MemoryManager::outputSummary(std::ostream &os, const Time& target_completion_time)
{
   ::MemoryManager::outputSummary(os, target_completion_time);
   
   os << "Cache Summary:\n";
   _L1_cache_cntlr->getL1ICache()->outputSummary(os, target_completion_time);
   _L1_cache_cntlr->getL1DCache()->outputSummary(os, target_completion_time);
   _L2_cache_cntlr->getL2Cache()->outputSummary(os, target_completion_time);

   if (_dram_cntlr_present)
   {
      _dram_cntlr->getDramPerfModel()->outputSummary(os);
   }
   else
   {
      DramPerfModel::dummyOutputSummary(os);
   }
}

void
MemoryManager::computeEnergy(const Time& curr_time)
{
   _L1_cache_cntlr->getL1ICache()->computeEnergy(curr_time);
   _L1_cache_cntlr->getL1DCache()->computeEnergy(curr_time);
   _L2_cache_cntlr->getL2Cache()->computeEnergy(curr_time);
}

double
MemoryManager::getDynamicEnergy()
{
   double dynamic_energy = _L1_cache_cntlr->getL1ICache()->getDynamicEnergy() +
                           _L1_cache_cntlr->getL1DCache()->getDynamicEnergy() +
                           _L2_cache_cntlr->getL2Cache()->getDynamicEnergy();
   return dynamic_energy;
}

double
MemoryManager::getLeakageEnergy()
{
   double leakage_energy = _L1_cache_cntlr->getL1ICache()->getLeakageEnergy() +
                           _L1_cache_cntlr->getL1DCache()->getLeakageEnergy() +
                           _L2_cache_cntlr->getL2Cache()->getLeakageEnergy();
   return leakage_energy;
}

int
MemoryManager::getDVFS(module_t module_type, double &frequency, double &voltage)
{
   int rc = 0;
   switch (module_type)
   {
      case L1_ICACHE:
         getL1ICache()->getDVFS(frequency,voltage);
         break;

      case L1_DCACHE:
         getL1DCache()->getDVFS(frequency,voltage);
         break;

      case L2_CACHE:
         getL2Cache()->getDVFS(frequency,voltage);
         break;

      default:
         rc = -2;
         break;
   }
   return rc;
}

int
MemoryManager::setDVFS(module_t module_type, double frequency, voltage_option_t voltage_flag, const Time& curr_time)
{
   int rc = 0;
   switch (module_type)
   {
      case L1_ICACHE:
         rc = getL1ICache()->setDVFS(frequency, voltage_flag, curr_time);
         break;

      case L1_DCACHE:
         rc = getL1DCache()->setDVFS(frequency, voltage_flag, curr_time);
         break;

      case L2_CACHE:
         rc = getL2Cache()->setDVFS(frequency, voltage_flag, curr_time);
         break;

      default:
         rc = -2;
         break;
   }
   return rc;
}

}

