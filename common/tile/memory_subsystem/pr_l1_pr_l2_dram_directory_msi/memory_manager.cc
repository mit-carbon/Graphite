#include "memory_manager.h"
#include "cache.h"
#include "simulator.h"
#include "tile_manager.h"
#include "clock_converter.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMSI
{

// Static variables
ofstream MemoryManager::_cache_line_replication_file;

MemoryManager::MemoryManager(Tile* tile, Network* network, ShmemPerfModel* shmem_perf_model)
   : ::MemoryManager(tile, network, shmem_perf_model)
   , _dram_directory_cntlr(NULL)
   , _dram_cntlr(NULL)
   , _dram_cntlr_present(false)
   , _enabled(false)
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
      dram_directory_total_entries = Sim()->getCfg()->getInt("dram_directory/total_entries");
      dram_directory_associativity = Sim()->getCfg()->getInt("dram_directory/associativity");
      dram_directory_max_num_sharers = Sim()->getConfig()->getTotalTiles();
      dram_directory_max_hw_sharers = Sim()->getCfg()->getInt("dram_directory/max_hw_sharers");
      dram_directory_type_str = Sim()->getCfg()->getString("dram_directory/directory_type");
      dram_directory_cache_access_time = Sim()->getCfg()->getInt("dram_directory/access_time");

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

   if (getTile()->getId() == 0)
   {
      LOG_ASSERT_ERROR(directory_type != "limited_broadcast",
            "Limited Broadcast directory scheme CANNOT be used with the MSI protocol.");
   }

   // Check if all cache line sizes are the same
   LOG_ASSERT_ERROR((l1_icache_line_size == l1_dcache_line_size) && (l1_dcache_line_size == l2_cache_line_size),
      "Cache Line Sizes of L1-I, L1-D and L2 Caches must be the same. "
      "Currently, L1-I Cache Line Size(%u), L1-D Cache Line Size(%u), L2 Cache Line Size(%u)",
      l1_icache_line_size, l1_dcache_line_size, l2_cache_line_size);
   
   _cache_line_size = l1_icache_line_size;
   dram_directory_home_lookup_param = ceilLog2(_cache_line_size);

   volatile float core_frequency = Config::getSingleton()->getCoreFrequency(getTile()->getMainCoreId());
  
   _user_thread_sem = new Semaphore(0);
   _network_thread_sem = new Semaphore(0);
   
   std::vector<tile_id_t> tile_list_with_dram_controllers = getTileListWithMemoryControllers();
   // if (getTile()->getId() == 0)
      // printTileListWithMemoryControllers(tile_list_with_dram_controllers);

   if (find(tile_list_with_dram_controllers.begin(), tile_list_with_dram_controllers.end(), getTile()->getId())
         != tile_list_with_dram_controllers.end())
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
            dram_directory_total_entries,
            dram_directory_associativity,
            getCacheLineSize(),
            dram_directory_max_num_sharers,
            dram_directory_max_hw_sharers,
            dram_directory_type_str,
            dram_directory_cache_access_time,
            tile_list_with_dram_controllers.size());
      
      LOG_PRINT("Instantiated Dram Directory Cntlr");
   }

   _dram_directory_home_lookup = new AddressHomeLookup(dram_directory_home_lookup_param, tile_list_with_dram_controllers, getCacheLineSize());

   LOG_PRINT("Instantiated Dram Directory Home Lookup");

   _l1_cache_cntlr = new L1CacheCntlr(this,
         _user_thread_sem,
         _network_thread_sem,
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
         core_frequency);
   
   LOG_PRINT("Instantiated L1 Cache Cntlr");

   _l2_cache_cntlr = new L2CacheCntlr(this,
         _l1_cache_cntlr,
         _dram_directory_home_lookup,
         _user_thread_sem,
         _network_thread_sem,
         getCacheLineSize(),
         l2_cache_size,
         l2_cache_associativity,
         l2_cache_replacement_policy,
         l2_cache_data_access_time,
         l2_cache_track_miss_types,
         core_frequency);

   LOG_PRINT("Instantiated L2 Cache Cntlr");

   _l1_cache_cntlr->setL2CacheCntlr(_l2_cache_cntlr);

   // Create Cache Performance Models
   _l1_icache_perf_model = CachePerfModel::create(l1_icache_perf_model_type,
         l1_icache_data_access_time, l1_icache_tags_access_time, core_frequency);
   _l1_dcache_perf_model = CachePerfModel::create(l1_dcache_perf_model_type,
         l1_dcache_data_access_time, l1_dcache_tags_access_time, core_frequency);
   _l2_cache_perf_model = CachePerfModel::create(l2_cache_perf_model_type,
         l2_cache_data_access_time, l2_cache_tags_access_time, core_frequency);

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
   delete _l1_icache_perf_model;
   delete _l1_dcache_perf_model;
   delete _l2_cache_perf_model;

   delete _user_thread_sem;
   delete _network_thread_sem;
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
MemoryManager::coreInitiateMemoryAccess(
      MemComponent::Type mem_component,
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

   MemComponent::Type receiver_mem_component = shmem_msg->getReceiverMemComponent();
   MemComponent::Type sender_mem_component = shmem_msg->getSenderMemComponent();

   if (_enabled)
   {
      LOG_PRINT("Got Shmem Msg: type(%i), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), sender(%i,%i), receiver(%i,%i)", 
            shmem_msg->getType(), shmem_msg->getAddress(), sender_mem_component, receiver_mem_component, sender.tile_id, sender.core_type, packet.receiver.tile_id, packet.receiver.core_type);    
   }

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

void
MemoryManager::sendMsg(tile_id_t receiver, ShmemMsg& shmem_msg)
{
   assert((shmem_msg.getDataBuf() == NULL) == (shmem_msg.getDataLength() == 0));

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   if (_enabled)
   {
      LOG_PRINT("Sending Msg: type(%u), address(%#llx), sender_mem_component(%u), receiver_mem_component(%u), requester(%i), sender(%i), receiver(%i)",
                shmem_msg.getType(), shmem_msg.getAddress(), shmem_msg.getSenderMemComponent(), shmem_msg.getReceiverMemComponent(),
                shmem_msg.getRequester(), getTile()->getId(), receiver);
   }

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

   if (_enabled)
   {
      LOG_PRINT("Broadcasting Msg: type(%u), address(%#llx), sender_mem_component(%u), receiver_mem_component(%u), requester(%i), sender(%i)",
                shmem_msg.getType(), shmem_msg.getAddress(), shmem_msg.getSenderMemComponent(), shmem_msg.getReceiverMemComponent(),
                shmem_msg.getRequester(), getTile()->getId());
   }

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
}

void
MemoryManager::disableModels()
{
   _enabled = false;

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
}

void
MemoryManager::openCacheLineReplicationTraceFiles()
{
   string output_dir;
   try
   {
      output_dir = Sim()->getCfg()->getString("general/output_dir");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read general/output_dir from the cfg file");
   }

   string filename = output_dir + "/cache_line_replication.dat";
   _cache_line_replication_file.open(filename.c_str());
}

void
MemoryManager::closeCacheLineReplicationTraceFiles()
{
   _cache_line_replication_file.close();
}

void
MemoryManager::outputCacheLineReplicationSummary()
{
   // Static Function to Compute the Time Varying Replication Index of a Cache Line
   // Go through the set of all caches and directories and get the
   // number of times each cache line is replicated

   SInt32 total_tiles = (SInt32) Config::getSingleton()->getTotalTiles();
   
   UInt64 total_exclusive_lines_l1_cache = 0;
   UInt64 total_shared_lines_l1_cache = 0;
   UInt64 total_exclusive_lines_l2_cache = 0;
   UInt64 total_shared_lines_l2_cache = 0;
   UInt64 total_cache_lines_l2_cache = 0;
   vector<UInt64> total_cache_line_sharer_count(total_tiles+1, 0);
   
   for (SInt32 tile_id = 0; tile_id < total_tiles; tile_id ++)
   {
      // Tile Ptr
      Tile* tile = Sim()->getTileManager()->getTileFromID(tile_id);
      assert(tile);
      MemoryManager* memory_manager = (MemoryManager*) tile->getMemoryManager();
      Cache* l1_icache = memory_manager->getL1ICache();
      Cache* l1_dcache = memory_manager->getL1DCache();
      Cache* l2_cache = memory_manager->getL2Cache();
      Directory* directory = (Directory*) NULL;
      if (memory_manager->isDramCntlrPresent())
         directory = memory_manager->getDramDirectoryCache()->getDirectory();
   
      // Get total lines in L1 caches & L2 cache
      vector<UInt64> _l1_icache_line_state_counters;
      vector<UInt64> _l1_dcache_line_state_counters;
      vector<UInt64> _l2_cache_line_state_counters;
      l1_icache->getCacheLineStateCounters(_l1_icache_line_state_counters);
      l1_dcache->getCacheLineStateCounters(_l1_dcache_line_state_counters);
      l2_cache->getCacheLineStateCounters(_l2_cache_line_state_counters);

      UInt64 num_exclusive_lines_l1_icache = _l1_icache_line_state_counters[CacheState::MODIFIED];
      UInt64 num_shared_lines_l1_icache = _l1_icache_line_state_counters[CacheState::SHARED];
      UInt64 num_exclusive_lines_l1_dcache = _l1_dcache_line_state_counters[CacheState::MODIFIED];
      UInt64 num_shared_lines_l1_dcache = _l1_dcache_line_state_counters[CacheState::SHARED];
      UInt64 num_exclusive_lines_l2_cache = _l2_cache_line_state_counters[CacheState::MODIFIED];
      UInt64 num_shared_lines_l2_cache = _l2_cache_line_state_counters[CacheState::SHARED];

      // Get total
      total_exclusive_lines_l1_cache += (num_exclusive_lines_l1_icache + num_exclusive_lines_l1_dcache);
      total_shared_lines_l1_cache += (num_shared_lines_l1_icache + num_shared_lines_l1_dcache);
      total_exclusive_lines_l2_cache += num_exclusive_lines_l2_cache;
      total_shared_lines_l2_cache += num_shared_lines_l2_cache;

      if (directory)
      {
         vector<UInt64> cache_line_sharer_count;
         directory->getSharerStats(cache_line_sharer_count);
         for (SInt32 num_sharers = 1; num_sharers <= total_tiles; num_sharers ++)
         {
            total_cache_line_sharer_count[num_sharers] += cache_line_sharer_count[num_sharers];
            total_cache_lines_l2_cache += cache_line_sharer_count[num_sharers];
         }
      }
   }

   // Write to file
   // L1 cache, L2 cache
   _cache_line_replication_file << total_exclusive_lines_l1_cache << ", " << total_shared_lines_l1_cache << ", " << endl;
   _cache_line_replication_file << total_exclusive_lines_l2_cache << ", " << total_shared_lines_l2_cache << ", " << endl;
   for (SInt32 i = 1; i <= total_tiles; i++)
      _cache_line_replication_file << total_cache_line_sharer_count[i] << ", ";
   _cache_line_replication_file << endl;
   
   // Replication count
   // For Pr L1, Pr L2 configuration
   vector<UInt64> total_cache_line_replication_count(2*total_tiles+1, 0);
   // Approximate for Pr L1, Sh L2 configuration
   vector<UInt64> total_cache_line_replication_count_pr_l1_sh_l2(total_tiles+2, 0); 
   
   // Account for exclusive lines first
   // For Pr L1, Pr L2 configuration
   total_cache_line_replication_count[1] += (total_exclusive_lines_l2_cache - total_exclusive_lines_l1_cache);
   total_cache_line_replication_count[2] += total_exclusive_lines_l1_cache;
   // Approximate for Pr L1, Sh L2 configuration
   total_cache_line_replication_count_pr_l1_sh_l2[1] += (total_exclusive_lines_l2_cache - total_exclusive_lines_l1_cache);
   total_cache_line_replication_count_pr_l1_sh_l2[2] += total_exclusive_lines_l1_cache;
   
   // Subtract out the exclusive lines 
   total_cache_line_sharer_count[1] -= total_exclusive_lines_l2_cache;

   double shared_lines_ratio = 1.0 * total_shared_lines_l1_cache / total_shared_lines_l2_cache;
   
   // Accout for shared lines next
   for (SInt32 num_sharers = 1; num_sharers <= total_tiles; num_sharers ++)
   {
      UInt64 num_cache_lines = total_cache_line_sharer_count[num_sharers];
      SInt32 degree_l2 = num_sharers;
      double degree_l1 = shared_lines_ratio * degree_l2;
      
      // Some lines are replicated in floor(degree_l1) L1 cache slices
      // while some lines are replicated in ceil(degree_l1) L1 cache caches
      UInt64 num_low = (UInt64) (num_cache_lines * (ceil(degree_l1) - degree_l1));
      SInt32 degree_low = (SInt32) floor(degree_l1);
      UInt64 num_high = num_cache_lines - num_low;
      SInt32 degree_high = (SInt32) ceil(degree_l1);

      // Approximate For Pr L1, Pr L2 configuration
      total_cache_line_replication_count[num_sharers + degree_low] += num_low;
      total_cache_line_replication_count[num_sharers + degree_high] += num_high;
      // Approximate for Pr L1, Sh L2 configuration
      total_cache_line_replication_count_pr_l1_sh_l2[1 + degree_low] += num_low;
      total_cache_line_replication_count_pr_l1_sh_l2[1 + degree_high] += num_high;
   }

   // For Pr L1, Pr L2 configuration
   for (SInt32 i = 1; i <= (2*total_tiles); i++)
      _cache_line_replication_file << total_cache_line_replication_count[i] << ", ";
   _cache_line_replication_file << endl;
   // Approximate for Pr L1, Sh L2 configuration
   for (SInt32 i = 1; i <= (total_tiles+1); i++)
      _cache_line_replication_file << total_cache_line_replication_count_pr_l1_sh_l2[i] << ", ";
   _cache_line_replication_file << endl;

   _cache_line_replication_file << endl;
}

}
