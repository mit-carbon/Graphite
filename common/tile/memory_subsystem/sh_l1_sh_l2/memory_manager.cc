#include "memory_manager.h"
#include "cache.h"
#include "simulator.h"
#include "tile_manager.h"
#include "clock_converter.h"
#include "log.h"

namespace ShL1ShL2
{

MemoryManager::MemoryManager(Tile* tile, Network* network, ShmemPerfModel* shmem_perf_model)
   : ::MemoryManager(tile, network, shmem_perf_model)
   , _dram_directory_cntlr(NULL)
   , _dram_cntlr(NULL)
   , _enabled(false)
{
   // Read Parameters from the Config file
   std::string l2_cache_type;
   UInt32 l2_cache_line_size = 0;
   UInt32 l2_cache_size = 0;
   UInt32 l2_cache_associativity = 0;
   std::string l2_cache_replacement_policy;
   UInt32 l2_cache_data_access_time = 0;
   UInt32 l2_cache_tags_access_time = 0;
   std::string l2_cache_perf_model_type;
   bool l2_cache_track_miss_types = false;

   UInt32 dram_directory_total_entries = 0;
   UInt32 dram_directory_associativity = 0;
   UInt32 dram_directory_max_num_sharers = 0;
   UInt32 dram_directory_max_hw_sharers = 0;
   std::string dram_directory_type_str;
   UInt32 dram_directory_home_lookup_param = 0;
   UInt32 dram_directory_cache_access_time = 0;

   volatile float dram_latency = 0.0;
   volatile float per_dram_controller_bandwidth = 0.0;
   bool dram_queue_model_enabled = false;
   std::string dram_queue_model_type;

   std::string directory_type;

   try
   {
      // L2 Cache
      l2_cache_type = "perf_model/l2_cache/" + Config::getSingleton()->getL2CacheType(getTile()->getId());
      l2_cache_line_size = Sim()->getCfg()->getInt(l2_cache_type + "/cache_line_size");
      l2_cache_size = Sim()->getCfg()->getInt(l2_cache_type + "/cache_size");
      l2_cache_associativity = Sim()->getCfg()->getInt(l2_cache_type + "/associativity");
      l2_cache_replacement_policy = Sim()->getCfg()->getString(l2_cache_type + "/replacement_policy");
      l2_cache_data_access_time = Sim()->getCfg()->getInt(l2_cache_type + "/data_access_time");
      l2_cache_tags_access_time = Sim()->getCfg()->getInt(l2_cache_type + "/tags_access_time");
      l2_cache_perf_model_type = Sim()->getCfg()->getString(l2_cache_type + "/perf_model_type");
      l2_cache_track_miss_types = Sim()->getCfg()->getBool(l2_cache_type + "/track_miss_types");

      // Dram Directory Cache
      dram_directory_total_entries = Sim()->getCfg()->getInt("perf_model/dram_directory/total_entries");
      dram_directory_associativity = Sim()->getCfg()->getInt("perf_model/dram_directory/associativity");
      dram_directory_max_num_sharers = Sim()->getConfig()->getTotalTiles();
      dram_directory_max_hw_sharers = Sim()->getCfg()->getInt("perf_model/dram_directory/max_hw_sharers");
      dram_directory_type_str = Sim()->getCfg()->getString("perf_model/dram_directory/directory_type");
      dram_directory_cache_access_time = Sim()->getCfg()->getInt("perf_model/dram_directory/directory_cache_access_time");

      // Dram Cntlr
      dram_latency = Sim()->getCfg()->getFloat("perf_model/dram/latency");
      per_dram_controller_bandwidth = Sim()->getCfg()->getFloat("perf_model/dram/per_controller_bandwidth");
      dram_queue_model_enabled = Sim()->getCfg()->getBool("perf_model/dram/queue_model/enabled");
      dram_queue_model_type = Sim()->getCfg()->getString("perf_model/dram/queue_model/type");

      // Directory Type
      directory_type = Sim()->getCfg()->getString("perf_model/dram_directory/directory_type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error reading memory system parameters from the config file");
   }

   _cache_line_size = l2_cache_line_size;
   ShmemMsg::setCacheLineSize(_cache_line_size);
   dram_directory_home_lookup_param = ceilLog2(_cache_line_size);
   
   volatile float core_frequency = Config::getSingleton()->getCoreFrequency(getTile()->getMainCoreId());
   
   _dram_cntlr = new DramCntlr(getTile(),
         dram_latency,
         per_dram_controller_bandwidth,
         dram_queue_model_enabled,
         dram_queue_model_type,
         _cache_line_size);

   LOG_PRINT("Instantiated Dram Cntlr");

   _dram_directory_cntlr = new DramDirectoryCntlr(this,
         _dram_cntlr,
         dram_directory_total_entries,
         dram_directory_associativity,
         _cache_line_size,
         dram_directory_max_num_sharers,
         dram_directory_max_hw_sharers,
         dram_directory_type_str,
         dram_directory_cache_access_time,
         Config::getSingleton()->getTotalTiles());
      
   LOG_PRINT("Instantiated Dram Directory Cntlr");

   vector<tile_id_t> tile_list;
   for (tile_id_t i = 0; i < (tile_id_t) Config::getSingleton()->getTotalTiles(); i++)
      tile_list.push_back(i);
   _address_home_lookup = new AddressHomeLookup(ceilLog2(_cache_line_size), tile_list, _cache_line_size);

   LOG_PRINT("Instantiated Dram Directory Home Lookup");

   _l1_cache_cntlr = new L1CacheCntlr(this,
         _address_home_lookup,
         _cache_line_size,
         core_frequency);
   
   LOG_PRINT("Instantiated L1 Cache Cntlr");

   _l2_cache_cntlr = new L2CacheCntlr(this,
         _cache_line_size,
         l2_cache_size,
         l2_cache_associativity,
         l2_cache_replacement_policy,
         l2_cache_data_access_time,
         l2_cache_track_miss_types,
         core_frequency);

   LOG_PRINT("Instantiated L2 Cache Cntlr");

   _l2_cache_cntlr->setDramDirectoryCntlr(_dram_directory_cntlr);
   _dram_directory_cntlr->setL2CacheCntlr(_l2_cache_cntlr);

   // Create Cache Performance Models
   _l2_cache_perf_model = CachePerfModel::create(l2_cache_perf_model_type,
         l2_cache_data_access_time, l2_cache_tags_access_time, core_frequency);
   _dram_directory_cache_perf_model = CachePerfModel::create("parallel",
         dram_directory_cache_access_time, 0, core_frequency);

   LOG_PRINT("Instantiated Cache Performance Models");

   // Register Call-backs
   getNetwork()->registerCallback(SHARED_MEM_1, MemoryManagerNetworkCallback, this);
   getNetwork()->registerCallback(SHARED_MEM_2, MemoryManagerNetworkCallback, this);
}

MemoryManager::~MemoryManager()
{
   getNetwork()->unregisterCallback(SHARED_MEM_1);
   getNetwork()->unregisterCallback(SHARED_MEM_2);

   // Delete the Models
   delete _l2_cache_perf_model;
   delete _dram_directory_cache_perf_model;

   delete _address_home_lookup;
   delete _l1_cache_cntlr;
   delete _l2_cache_cntlr;
   delete _dram_cntlr;
   delete _dram_directory_cntlr;
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
   return _l1_cache_cntlr->processMemOpFromTile(mem_component, 
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

   if (_enabled)
   {
      LOG_PRINT("Got Shmem Msg: type(%i), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), sender(%i,%i), receiver(%i,%i)", 
            shmem_msg->getMsgType(), shmem_msg->getAddress(), sender_mem_component, receiver_mem_component, sender.tile_id, sender.core_type, packet.receiver.tile_id, packet.receiver.core_type);    
   }

   switch (receiver_mem_component)
   {
   case MemComponent::L1_DCACHE:
      switch(sender_mem_component)
      {
      case MemComponent::DRAM_DIRECTORY:
         _l1_cache_cntlr->handleMsgFromDramDirectory(sender.tile_id, shmem_msg);
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized sender component(%u)", sender_mem_component);
         break;
      }
      break;

   case MemComponent::DRAM_DIRECTORY:
      switch(sender_mem_component)
      {
      case MemComponent::L1_DCACHE:
         _dram_directory_cntlr->handleMsgFromL1Cache(sender.tile_id, shmem_msg);
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
   // LOG_PRINT("Finished handling Shmem Msg");
   shmem_msg->release();
}

void
MemoryManager::sendMsg(tile_id_t receiver, ShmemMsg& shmem_msg)
{
   assert((shmem_msg.getDataBuf() == NULL) == (shmem_msg.getDataLength() == 0));

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   if (_enabled)
   {
      LOG_PRINT("Sending Msg: type(%u), address(%#llx), sender_mem_component(%u), receiver_mem_component(%u), sender(%i), receiver(%i)",
                shmem_msg.getMsgType(), shmem_msg.getAddress(), shmem_msg.getSenderMemComponent(), shmem_msg.getReceiverMemComponent(),
                getTile()->getId(), receiver);
   }

   NetPacket packet(msg_time, SHARED_MEM_1,
         getTile()->getId(), receiver,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   getNetwork()->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::incrCycleCount(MemComponent::component_t mem_component, CachePerfModel::CacheAccess_t access_type)
{
   switch (mem_component)
   {
   case MemComponent::L2_CACHE:
      getShmemPerfModel()->incrCycleCount(_l2_cache_perf_model->getLatency(access_type));
      break;

   case MemComponent::DRAM_DIRECTORY:
      getShmemPerfModel()->incrCycleCount(_dram_directory_cache_perf_model->getLatency(access_type));
      break;

   case MemComponent::INVALID_MEM_COMPONENT:
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized mem component type(%u)", mem_component);
      break;
   }
}

void
MemoryManager::enableModels()
{
   _enabled = true;

   _l2_cache_cntlr->getL2Cache()->enable();
   _l2_cache_perf_model->enable();

   _dram_directory_cntlr->getDramDirectoryCache()->enable();
   _dram_directory_cache_perf_model->enable();

   _dram_cntlr->getDramPerfModel()->enable();
}

void
MemoryManager::disableModels()
{
   _enabled = false;

   _l2_cache_cntlr->getL2Cache()->disable();
   _l2_cache_perf_model->disable();

   _dram_directory_cntlr->getDramDirectoryCache()->disable();
   _dram_directory_cache_perf_model->disable();

   _dram_cntlr->getDramPerfModel()->disable();
}

void
MemoryManager::outputSummary(std::ostream &os)
{
   os << "Cache Summary:\n";
   _l2_cache_cntlr->getL2Cache()->outputSummary(os);

   _dram_cntlr->getDramPerfModel()->outputSummary(os);
   os << "Dram Directory Cache Summary:\n";
   _dram_directory_cntlr->getDramDirectoryCache()->outputSummary(os);
}

}
