#include "memory_manager.h"
#include "cache.h"
#include "simulator.h"
#include "tile_manager.h"
#include "clock_converter.h"
#include "utils.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMSI
{

MemoryManager::MemoryManager(Tile* tile)
   : ::MemoryManager(tile)
   , _dram_directory_cntlr(NULL)
   , _dram_cntlr(NULL)
   , _dram_cntlr_present(false)
{
   // Read Parameters from the Config file
   std::string l1_icache_type;
   UInt32 l1_icache_line_size = 0;
   UInt32 l1_icache_size = 0;
   UInt32 l1_icache_associativity = 0;
   std::string l1_icache_replacement_policy;
   UInt32 l1_icache_data_access_time = 0;
   UInt32 l1_icache_tags_access_time = 0;
   std::string l1_icache_perf_model_type;
   bool l1_icache_track_miss_types = false;

   std::string l1_dcache_type;
   UInt32 l1_dcache_line_size = 0;
   UInt32 l1_dcache_size = 0;
   UInt32 l1_dcache_associativity = 0;
   std::string l1_dcache_replacement_policy;
   UInt32 l1_dcache_data_access_time = 0;
   UInt32 l1_dcache_tags_access_time = 0;
   std::string l1_dcache_perf_model_type;
   bool l1_dcache_track_miss_types = false;

   std::string l2_cache_type;
   UInt32 l2_cache_line_size = 0;
   UInt32 l2_cache_size = 0;
   UInt32 l2_cache_associativity = 0;
   std::string l2_cache_replacement_policy;
   UInt32 l2_cache_data_access_time = 0;
   UInt32 l2_cache_tags_access_time = 0;
   std::string l2_cache_perf_model_type;
   bool l2_cache_track_miss_types = false;

   std::string dram_directory_total_entries_str;
   UInt32 dram_directory_associativity = 0;
   UInt32 dram_directory_max_num_sharers = 0;
   UInt32 dram_directory_max_hw_sharers = 0;
   std::string dram_directory_type_str;
   UInt32 dram_directory_home_lookup_param = 0;
   std::string dram_directory_access_time_str;

   volatile float dram_latency = 0.0;
   volatile float per_dram_controller_bandwidth = 0.0;
   bool dram_queue_model_enabled = false;
   std::string dram_queue_model_type;

   std::string directory_type;

   try
   {
      // L1 ICache
      l1_icache_type = "l1_icache/" + Config::getSingleton()->getL1ICacheType(getTile()->getId());
      l1_icache_line_size = Sim()->getCfg()->getInt(l1_icache_type + "/cache_line_size");
      l1_icache_size = Sim()->getCfg()->getInt(l1_icache_type + "/cache_size");
      l1_icache_associativity = Sim()->getCfg()->getInt(l1_icache_type + "/associativity");
      l1_icache_replacement_policy = Sim()->getCfg()->getString(l1_icache_type + "/replacement_policy");
      l1_icache_data_access_time = Sim()->getCfg()->getInt(l1_icache_type + "/data_access_time");
      l1_icache_tags_access_time = Sim()->getCfg()->getInt(l1_icache_type + "/tags_access_time");
      l1_icache_perf_model_type = Sim()->getCfg()->getString(l1_icache_type + "/perf_model_type");
      l1_icache_track_miss_types = Sim()->getCfg()->getBool(l1_icache_type + "/track_miss_types");

      // L1 DCache
      l1_dcache_type = "l1_dcache/" + Config::getSingleton()->getL1DCacheType(getTile()->getId());
      l1_dcache_line_size = Sim()->getCfg()->getInt(l1_dcache_type + "/cache_line_size");
      l1_dcache_size = Sim()->getCfg()->getInt(l1_dcache_type + "/cache_size");
      l1_dcache_associativity = Sim()->getCfg()->getInt(l1_dcache_type + "/associativity");
      l1_dcache_replacement_policy = Sim()->getCfg()->getString(l1_dcache_type + "/replacement_policy");
      l1_dcache_data_access_time = Sim()->getCfg()->getInt(l1_dcache_type + "/data_access_time");
      l1_dcache_tags_access_time = Sim()->getCfg()->getInt(l1_dcache_type + "/tags_access_time");
      l1_dcache_perf_model_type = Sim()->getCfg()->getString(l1_dcache_type + "/perf_model_type");
      l1_dcache_track_miss_types = Sim()->getCfg()->getBool(l1_dcache_type + "/track_miss_types");

      // L2 Cache
      l2_cache_type = "l2_cache/" + Config::getSingleton()->getL2CacheType(getTile()->getId());
      l2_cache_line_size = Sim()->getCfg()->getInt(l2_cache_type + "/cache_line_size");
      l2_cache_size = Sim()->getCfg()->getInt(l2_cache_type + "/cache_size");
      l2_cache_associativity = Sim()->getCfg()->getInt(l2_cache_type + "/associativity");
      l2_cache_replacement_policy = Sim()->getCfg()->getString(l2_cache_type + "/replacement_policy");
      l2_cache_data_access_time = Sim()->getCfg()->getInt(l2_cache_type + "/data_access_time");
      l2_cache_tags_access_time = Sim()->getCfg()->getInt(l2_cache_type + "/tags_access_time");
      l2_cache_perf_model_type = Sim()->getCfg()->getString(l2_cache_type + "/perf_model_type");
      l2_cache_track_miss_types = Sim()->getCfg()->getBool(l2_cache_type + "/track_miss_types");

      // Dram Directory Cache
      dram_directory_total_entries_str = Sim()->getCfg()->getString("dram_directory/total_entries");
      dram_directory_associativity = Sim()->getCfg()->getInt("dram_directory/associativity");
      dram_directory_max_num_sharers = Sim()->getConfig()->getTotalTiles();
      dram_directory_max_hw_sharers = Sim()->getCfg()->getInt("dram_directory/max_hw_sharers");
      dram_directory_type_str = Sim()->getCfg()->getString("dram_directory/directory_type");
      dram_directory_access_time_str = Sim()->getCfg()->getString("dram_directory/access_time");

      // Dram Cntlr
      dram_latency = Sim()->getCfg()->getFloat("dram/latency");
      per_dram_controller_bandwidth = Sim()->getCfg()->getFloat("dram/per_controller_bandwidth");
      dram_queue_model_enabled = Sim()->getCfg()->getBool("dram/queue_model/enabled");
      dram_queue_model_type = Sim()->getCfg()->getString("dram/queue_model/type");

      // Directory Type
      directory_type = Sim()->getCfg()->getString("dram_directory/directory_type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error reading memory system parameters from the config file");
   }

   LOG_ASSERT_ERROR(directory_type != "limited_broadcast",
         "limited_broadcast directory scheme CANNOT be used with the pr_l1_pr_l2_dram_directory_msi protocol.");

   // Check if all cache line sizes are the same
   LOG_ASSERT_ERROR((l1_icache_line_size == l1_dcache_line_size) && (l1_dcache_line_size == l2_cache_line_size),
      "Cache Line Sizes of L1-I, L1-D and L2 Caches must be the same. "
      "Currently, L1-I Cache Line Size(%u), L1-D Cache Line Size(%u), L2 Cache Line Size(%u)",
      l1_icache_line_size, l1_dcache_line_size, l2_cache_line_size);
   
   _cache_line_size = l1_icache_line_size;
   dram_directory_home_lookup_param = ceilLog2(_cache_line_size);

   float frequency = getTile()->getFrequency();
  
   std::vector<tile_id_t> tile_list_with_memory_controllers = getTileListWithMemoryControllers();
   UInt32 num_memory_controllers = tile_list_with_memory_controllers.size();

   if (find(tile_list_with_memory_controllers.begin(), tile_list_with_memory_controllers.end(), getTile()->getId())
         != tile_list_with_memory_controllers.end())
   {
      _dram_cntlr_present = true;

      _dram_cntlr = new DramCntlr(getTile(),
            dram_latency,
            per_dram_controller_bandwidth,
            dram_queue_model_enabled,
            dram_queue_model_type,
            getCacheLineSize());

      LOG_PRINT("Instantiated Dram Cntlr");

      _dram_directory_cntlr = new DramDirectoryCntlr(this,
            _dram_cntlr,
            dram_directory_total_entries_str,
            dram_directory_associativity,
            getCacheLineSize(),
            dram_directory_max_num_sharers,
            dram_directory_max_hw_sharers,
            dram_directory_type_str,
            dram_directory_access_time_str,
            num_memory_controllers);
      
      LOG_PRINT("Instantiated Dram Directory Cntlr");
   }

   _dram_directory_home_lookup = new AddressHomeLookup(dram_directory_home_lookup_param, tile_list_with_memory_controllers, getCacheLineSize());

   LOG_PRINT("Instantiated Dram Directory Home Lookup");

   _l1_cache_cntlr = new L1CacheCntlr(this,
         getCacheLineSize(),
         l1_icache_size,
         l1_icache_associativity,
         l1_icache_replacement_policy,
         l1_icache_data_access_time,
         l1_icache_track_miss_types,
         l1_dcache_size,
         l1_dcache_associativity,
         l1_dcache_replacement_policy,
         l1_dcache_data_access_time,
         l1_dcache_track_miss_types,
         frequency);
   
   LOG_PRINT("Instantiated L1 Cache Cntlr");

   _l2_cache_cntlr = new L2CacheCntlr(this,
         _l1_cache_cntlr,
         _dram_directory_home_lookup,
         getCacheLineSize(),
         l2_cache_size,
         l2_cache_associativity,
         l2_cache_replacement_policy,
         l2_cache_data_access_time,
         l2_cache_track_miss_types,
         frequency);

   LOG_PRINT("Instantiated L2 Cache Cntlr");

   _l1_cache_cntlr->setL2CacheCntlr(_l2_cache_cntlr);

   // Create Cache Performance Models
   _l1_icache_perf_model = CachePerfModel::create(l1_icache_perf_model_type,
         l1_icache_data_access_time, l1_icache_tags_access_time, frequency);
   _l1_dcache_perf_model = CachePerfModel::create(l1_dcache_perf_model_type,
         l1_dcache_data_access_time, l1_dcache_tags_access_time, frequency);
   _l2_cache_perf_model = CachePerfModel::create(l2_cache_perf_model_type,
         l2_cache_data_access_time, l2_cache_tags_access_time, frequency);

   LOG_PRINT("Instantiated Cache Performance Models");
}

MemoryManager::~MemoryManager()
{
   // Delete the Models
   delete _l1_icache_perf_model;
   delete _l1_dcache_perf_model;
   delete _l2_cache_perf_model;

   delete _dram_directory_home_lookup;
   delete _l1_cache_cntlr;
   delete _l2_cache_cntlr;
   if (_dram_cntlr_present)
   {
      delete _dram_cntlr;
      delete _dram_directory_cntlr;
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
   return _l1_cache_cntlr->processMemOpFromCore(mem_component, lock_signal, mem_op_type, 
                                                address, offset, data_buf, data_length,
                                                modeled);
}

void
MemoryManager::handleMsgFromNetwork(NetPacket& packet)
{
   core_id_t sender = packet.sender;
   ShmemMsg* shmem_msg = ShmemMsg::getShmemMsg((Byte*) packet.data);

   MemComponent::Type receiver_mem_component = shmem_msg->getReceiverMemComponent();
   MemComponent::Type sender_mem_component = shmem_msg->getSenderMemComponent();

   LOG_PRINT("Got Shmem Msg: type(%i), address(%#lx), sender_mem_component(%u), receiver_mem_component(%u), sender(%i,%i), receiver(%i,%i)", 
             shmem_msg->getType(), shmem_msg->getAddress(), sender_mem_component, receiver_mem_component,
             sender.tile_id, sender.core_type, packet.receiver.tile_id, packet.receiver.core_type);    

   switch (receiver_mem_component)
   {
   case MemComponent::L2_CACHE:
      switch(sender_mem_component)
      {
         case MemComponent::L1_ICACHE:
         case MemComponent::L1_DCACHE:
            assert(sender.tile_id == getTile()->getId());
            _l2_cache_cntlr->handleMsgFromL1Cache(shmem_msg);
            break;

         case MemComponent::DRAM_DIRECTORY:
            _l2_cache_cntlr->handleMsgFromDramDirectory(sender.tile_id, shmem_msg);
            break;

         default:
            LOG_PRINT_ERROR("Unrecognized sender component(%u)",
                  sender_mem_component);
            break;
      }
      break;

   case MemComponent::DRAM_DIRECTORY:
      switch(sender_mem_component)
      {
         LOG_ASSERT_ERROR(_dram_cntlr_present, "Dram Cntlr NOT present");

         case MemComponent::L2_CACHE:
            _dram_directory_cntlr->handleMsgFromL2Cache(sender.tile_id, shmem_msg);
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
   // LOG_PRINT("Finished handling Shmem Msg");

   if (shmem_msg->getDataLength() > 0)
   {
      assert(shmem_msg->getDataBuf());
      delete [] shmem_msg->getDataBuf();
   }
   delete shmem_msg;
}

// Update internal variables when frequency is changed
// Variables that need to be updated include all variables that are expressed in terms of cycles
//  e.g., total memory access latency, packet arrival time, etc.
void
MemoryManager::updateInternalVariablesOnFrequencyChange(float old_frequency, float new_frequency)
{
   _l2_cache_cntlr->updateInternalVariablesOnFrequencyChange(old_frequency, new_frequency);
   _dram_directory_cntlr->updateInternalVariablesOnFrequencyChange(old_frequency, new_frequency);
}      

void
MemoryManager::sendMsg(tile_id_t receiver, ShmemMsg& shmem_msg)
{
   assert((shmem_msg.getDataBuf() == NULL) == (shmem_msg.getDataLength() == 0));

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   LOG_PRINT("Sending Msg: type(%u), address(%#lx), sender_mem_component(%u), receiver_mem_component(%u), "
             "requester(%i), sender(%i), receiver(%i)",
             shmem_msg.getType(), shmem_msg.getAddress(), shmem_msg.getSenderMemComponent(), shmem_msg.getReceiverMemComponent(),
             shmem_msg.getRequester(), getTile()->getId(), receiver);

   NetPacket packet(msg_time, SHARED_MEM_1,
         getTile()->getId(), receiver,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   getNetwork()->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::broadcastMsg(ShmemMsg& shmem_msg)
{
   assert((shmem_msg.getDataBuf() == NULL) == (shmem_msg.getDataLength() == 0));

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   LOG_PRINT("Broadcasting Msg: type(%u), address(%#lx), sender_mem_component(%u), receiver_mem_component(%u), "
             "requester(%i), sender(%i)",
             shmem_msg.getType(), shmem_msg.getAddress(), shmem_msg.getSenderMemComponent(), shmem_msg.getReceiverMemComponent(),
             shmem_msg.getRequester(), getTile()->getId());

   NetPacket packet(msg_time, SHARED_MEM_1,
         getTile()->getId(), NetPacket::BROADCAST,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   getNetwork()->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::incrCycleCount(MemComponent::Type mem_component, CachePerfModel::CacheAccess_t access_type)
{
   switch (mem_component)
   {
   case MemComponent::L1_ICACHE:
      getShmemPerfModel()->incrCycleCount(_l1_icache_perf_model->getLatency(access_type));
      break;

   case MemComponent::L1_DCACHE:
      getShmemPerfModel()->incrCycleCount(_l1_dcache_perf_model->getLatency(access_type));
      break;

   case MemComponent::L2_CACHE:
      getShmemPerfModel()->incrCycleCount(_l2_cache_perf_model->getLatency(access_type));
      break;

   case MemComponent::INVALID:
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized mem component type(%u)", mem_component);
      break;
   }
}

void
MemoryManager::enableModels()
{
   _l1_cache_cntlr->getL1ICache()->enable();
   _l1_icache_perf_model->enable();
   
   _l1_cache_cntlr->getL1DCache()->enable();
   _l1_dcache_perf_model->enable();
   
   _l2_cache_cntlr->getL2Cache()->enable();
   _l2_cache_perf_model->enable();

   if (_dram_cntlr_present)
   {
      _dram_directory_cntlr->getDramDirectoryCache()->enable();
      _dram_cntlr->getDramPerfModel()->enable();
   }

   ::MemoryManager::enableModels();
}

void
MemoryManager::disableModels()
{
   _l1_cache_cntlr->getL1ICache()->disable();
   _l1_icache_perf_model->disable();

   _l1_cache_cntlr->getL1DCache()->disable();
   _l1_dcache_perf_model->disable();

   _l2_cache_cntlr->getL2Cache()->disable();
   _l2_cache_perf_model->disable();

   if (_dram_cntlr_present)
   {
      _dram_directory_cntlr->getDramDirectoryCache()->disable();
      _dram_cntlr->getDramPerfModel()->disable();
   }

   ::MemoryManager::disableModels();
}

void
MemoryManager::outputSummary(std::ostream &os)
{
   os << "Cache Summary:\n";
   _l1_cache_cntlr->getL1ICache()->outputSummary(os);
   _l1_cache_cntlr->getL1DCache()->outputSummary(os);
   _l2_cache_cntlr->getL2Cache()->outputSummary(os);

   if (_dram_cntlr_present)
   {      
      _dram_cntlr->getDramPerfModel()->outputSummary(os);
      os << "Dram Directory Cache Summary:\n";
      _dram_directory_cntlr->getDramDirectoryCache()->outputSummary(os);
   }
   else
   {
      DramPerfModel::dummyOutputSummary(os);
      os << "Dram Directory Cache Summary:\n";
      DirectoryCache::dummyOutputSummary(os, getTile()->getId());
   }
   
   ::MemoryManager::outputSummary(os);
}

void
MemoryManager::computeEnergy()
{
   _l1_cache_cntlr->getL1ICache()->computeEnergy();
   _l1_cache_cntlr->getL1DCache()->computeEnergy();
   _l2_cache_cntlr->getL2Cache()->computeEnergy();
}

double
MemoryManager::getDynamicEnergy()
{
   double dynamic_energy = _l1_cache_cntlr->getL1ICache()->getDynamicEnergy() +
                           _l1_cache_cntlr->getL1DCache()->getDynamicEnergy() +
                           _l2_cache_cntlr->getL2Cache()->getDynamicEnergy();

   return dynamic_energy;
}

double
MemoryManager::getStaticPower()
{
   double static_power = _l1_cache_cntlr->getL1ICache()->getStaticPower() +
                         _l1_cache_cntlr->getL1DCache()->getStaticPower() +
                         _l2_cache_cntlr->getL2Cache()->getStaticPower();

   return static_power;
}

}
