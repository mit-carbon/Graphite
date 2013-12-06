#include "memory_manager.h"
#include "cache.h"
#include "simulator.h"
#include "tile_manager.h"
#include "network.h"
#include "utils.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMOSI
{

// Static variables
ofstream MemoryManager::_cache_line_replication_file;

MemoryManager::MemoryManager(Tile* tile)
   : ::MemoryManager(tile)
   , _dram_directory_cntlr(NULL)
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

   std::string dram_directory_total_entries_str;
   UInt32 dram_directory_associativity = 0;
   UInt32 dram_directory_max_num_sharers = 0;
   UInt32 dram_directory_max_hw_sharers = 0;
   std::string dram_directory_type_str;
   UInt32 dram_directory_home_lookup_param = 0;
   std::string dram_directory_access_cycles_str;

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

      // Dram Directory Cache
      dram_directory_total_entries_str = Sim()->getCfg()->getString("dram_directory/total_entries");
      dram_directory_associativity = Sim()->getCfg()->getInt("dram_directory/associativity");
      dram_directory_max_num_sharers = Sim()->getConfig()->getTotalTiles();
      dram_directory_max_hw_sharers = Sim()->getCfg()->getInt("dram_directory/max_hw_sharers");
      dram_directory_type_str = Sim()->getCfg()->getString("dram_directory/directory_type");
      dram_directory_access_cycles_str = Sim()->getCfg()->getString("dram_directory/access_time");

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
   dram_directory_home_lookup_param = ceilLog2(_cache_line_size);

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

      _dram_directory_cntlr = new DramDirectoryCntlr(this,
            _dram_cntlr,
            dram_directory_total_entries_str,
            dram_directory_associativity,
            getCacheLineSize(),
            dram_directory_max_num_sharers,
            dram_directory_max_hw_sharers,
            dram_directory_type_str,
            num_memory_controllers,
            dram_directory_access_cycles_str);
   }

   _dram_directory_home_lookup = new AddressHomeLookup(dram_directory_home_lookup_param, tile_list_with_memory_controllers, getCacheLineSize());

   _L1_cache_cntlr = new L1CacheCntlr(this,
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
   
   _L2_cache_cntlr = new L2CacheCntlr(this,
         _L1_cache_cntlr,
         _dram_directory_home_lookup,
         getCacheLineSize(),
         L2_cache_size,
         L2_cache_associativity,
         L2_cache_num_banks,
         L2_cache_replacement_policy,
         L2_cache_data_access_cycles,
         L2_cache_tags_access_cycles,
         L2_cache_perf_model_type,
         L2_cache_track_miss_types);

   _L1_cache_cntlr->setL2CacheCntlr(_L2_cache_cntlr);
}

MemoryManager::~MemoryManager()
{
   delete _dram_directory_home_lookup;
   delete _L1_cache_cntlr;
   delete _L2_cache_cntlr;
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

   LOG_PRINT("Time(%llu), Got Shmem Msg: type(%s), address(%#lx), "
             "sender_mem_component(%s), receiver_mem_component(%s), sender(%i,%i), receiver(%i,%i)", 
             msg_time.toNanosec(), SPELL_SHMSG(shmem_msg->getType()), shmem_msg->getAddress(),
             SPELL_MEMCOMP(sender_mem_component), SPELL_MEMCOMP(receiver_mem_component),
             sender.tile_id, sender.core_type, packet.receiver.tile_id, packet.receiver.core_type);    

   switch (receiver_mem_component)
   {
   case MemComponent::L2_CACHE:
      switch(sender_mem_component)
      {
         case MemComponent::L1_ICACHE:
         case MemComponent::L1_DCACHE:
            assert(sender.tile_id == getTile()->getId());
            _L2_cache_cntlr->handleMsgFromL1Cache(shmem_msg);
            break;

         case MemComponent::DRAM_DIRECTORY:
            _L2_cache_cntlr->handleMsgFromDramDirectory(sender.tile_id, shmem_msg);
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
   Time msg_time = getShmemPerfModel()->getCurrTime();

   LOG_PRINT("Time(%llu), Sending Msg: type(%s), address(%#lx), "
             "sender_mem_component(%s), receiver_mem_component(%s), requester(%i), sender(%i), receiver(%i)",
             msg_time.toNanosec(), SPELL_SHMSG(shmem_msg.getType()), shmem_msg.getAddress(),
             SPELL_MEMCOMP(shmem_msg.getSenderMemComponent()), SPELL_MEMCOMP(shmem_msg.getReceiverMemComponent()),
             shmem_msg.getRequester(), getTile()->getId(), receiver);

   NetPacket packet(msg_time, SHARED_MEM,
         getTile()->getId(), receiver,
         shmem_msg.getMsgLen(), (const void*) msg_buf);

   if (getTile()->getId() == receiver){
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

   LOG_PRINT("Time(%llu), Broadcasting Msg: type(%s), address(%#lx), "
             "sender_mem_component(%s), receiver_mem_component(%s), requester(%i), sender(%i)",
             msg_time.toNanosec(), SPELL_SHMSG(shmem_msg.getType()), shmem_msg.getAddress(),
             SPELL_MEMCOMP(shmem_msg.getSenderMemComponent()), SPELL_MEMCOMP(shmem_msg.getReceiverMemComponent()),
             shmem_msg.getRequester(), getTile()->getId());

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
      _dram_directory_cntlr->enable();
      _dram_directory_cntlr->getDramDirectoryCache()->enable();
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
      _dram_directory_cntlr->disable();
      _dram_directory_cntlr->getDramDirectoryCache()->disable();
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
   _L2_cache_cntlr->outputSummary(os);

   if (_dram_cntlr_present)
   {
      _dram_directory_cntlr->outputSummary(os);
      os << "Dram Directory Summary:\n";
      _dram_directory_cntlr->getDramDirectoryCache()->outputSummary(os);
      _dram_cntlr->getDramPerfModel()->outputSummary(os);
   }
   else
   {
      DramDirectoryCntlr::dummyOutputSummary(os);
      os << "Dram Directory Summary:\n";
      DirectoryCache::dummyOutputSummary(os, getTile()->getId());
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

void
MemoryManager::openCacheLineReplicationTraceFiles()
{
   checkDramDirectoryType();
   
   string output_dir;
   try
   {
      output_dir = Sim()->getCfg()->getString("general/output_dir");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read [general/output_dir] from the cfg file");
   }

   string filename = output_dir + "/cache_line_replication.dat";
   _cache_line_replication_file.open(filename.c_str());
}

void
MemoryManager::closeCacheLineReplicationTraceFiles()
{
   checkDramDirectoryType();

   _cache_line_replication_file.close();
}

void
MemoryManager::outputCacheLineReplicationSummary()
{
   checkDramDirectoryType();

   // Static Function to Compute the Time Varying Replication Index of a Cache Line
   // Go through the set of all caches and directories and get the
   // number of times each cache line is replicated

   SInt32 total_tiles = (SInt32) Config::getSingleton()->getTotalTiles();
   
   UInt64 total_exclusive_lines_L1_cache = 0;
   UInt64 total_shared_lines_L1_cache = 0;
   UInt64 total_exclusive_lines_L2_cache = 0;
   UInt64 total_shared_lines_L2_cache = 0;
   UInt64 total_cache_lines_L2_cache = 0;
   vector<UInt64> total_cache_line_sharer_count(total_tiles+1, 0);
   
   for (SInt32 tile_id = 0; tile_id < total_tiles; tile_id ++)
   {
      // Tile Ptr
      Tile* tile = Sim()->getTileManager()->getTileFromID(tile_id);
      assert(tile);
      MemoryManager* memory_manager = (MemoryManager*) tile->getMemoryManager();
      Cache* L1_icache = memory_manager->getL1ICache();
      Cache* L1_dcache = memory_manager->getL1DCache();
      Cache* L2_cache = memory_manager->getL2Cache();
      Directory* directory = (Directory*) NULL;
      if (memory_manager->isDramCntlrPresent())
         directory = memory_manager->getDramDirectoryCache()->getDirectory();
   
      // Get total lines in L1 caches & L2 cache
      vector<UInt64> L1_icache_line_state_counters;
      vector<UInt64> L1_dcache_line_state_counters;
      vector<UInt64> L2_cache_line_state_counters;
      L1_icache->getCacheLineStateCounters(L1_icache_line_state_counters);
      L1_dcache->getCacheLineStateCounters(L1_dcache_line_state_counters);
      L2_cache->getCacheLineStateCounters(L2_cache_line_state_counters);

      UInt64 num_exclusive_lines_L1_icache = L1_icache_line_state_counters[CacheState::MODIFIED];
      UInt64 num_shared_lines_L1_icache = L1_icache_line_state_counters[CacheState::OWNED] + L1_icache_line_state_counters[CacheState::SHARED];
      UInt64 num_exclusive_lines_L1_dcache = L1_dcache_line_state_counters[CacheState::MODIFIED];
      UInt64 num_shared_lines_L1_dcache = L1_dcache_line_state_counters[CacheState::OWNED] + L1_dcache_line_state_counters[CacheState::SHARED];
      UInt64 num_exclusive_lines_L2_cache = L2_cache_line_state_counters[CacheState::MODIFIED];
      UInt64 num_shared_lines_L2_cache = L2_cache_line_state_counters[CacheState::OWNED] + L2_cache_line_state_counters[CacheState::SHARED];
      
      // Get total
      total_exclusive_lines_L1_cache += (num_exclusive_lines_L1_icache + num_exclusive_lines_L1_dcache);
      total_shared_lines_L1_cache += (num_shared_lines_L1_icache + num_shared_lines_L1_dcache);
      total_exclusive_lines_L2_cache += num_exclusive_lines_L2_cache;
      total_shared_lines_L2_cache += num_shared_lines_L2_cache;

      if (directory)
      {
         vector<UInt64> cache_line_sharer_count;
         directory->getSharerStats(cache_line_sharer_count);
         for (SInt32 num_sharers = 1; num_sharers <= total_tiles; num_sharers ++)
         {
            total_cache_line_sharer_count[num_sharers] += cache_line_sharer_count[num_sharers];
            total_cache_lines_L2_cache += cache_line_sharer_count[num_sharers];
         }
      }
   }

   // Write to file
   // L1 cache, L2 cache
   _cache_line_replication_file << total_exclusive_lines_L1_cache << ", " << total_shared_lines_L1_cache << ", " << endl;
   _cache_line_replication_file << total_exclusive_lines_L2_cache << ", " << total_shared_lines_L2_cache << ", " << endl;
   for (SInt32 i = 1; i <= total_tiles; i++)
      _cache_line_replication_file << total_cache_line_sharer_count[i] << ", ";
   _cache_line_replication_file << endl;
   
   // Replication count
   // For Pr L1, Pr L2 configuration
   vector<UInt64> total_cache_line_replication_count(2*total_tiles+1, 0);
   
   // Account for exclusive lines first
   // For Pr L1, Pr L2 configuration
   total_cache_line_replication_count[1] += (total_exclusive_lines_L2_cache - total_exclusive_lines_L1_cache);
   total_cache_line_replication_count[2] += total_exclusive_lines_L1_cache;
   
   // Subtract out the exclusive lines 
   total_cache_line_sharer_count[1] -= total_exclusive_lines_L2_cache;

   double shared_lines_ratio = 1.0 * total_shared_lines_L1_cache / total_shared_lines_L2_cache;
   
   // Accout for shared lines next
   for (SInt32 num_sharers = 1; num_sharers <= total_tiles; num_sharers ++)
   {
      UInt64 num_cache_lines = total_cache_line_sharer_count[num_sharers];
      SInt32 degree_L2 = num_sharers;
      double degree_L1 = shared_lines_ratio * degree_L2;
      
      // Some lines are replicated in floor(degree_L1) L1 cache slices
      // while some lines are replicated in ceil(degree_L1) L1 cache caches
      UInt64 num_low = (UInt64) (num_cache_lines * (ceil(degree_L1) - degree_L1));
      SInt32 degree_low = (SInt32) floor(degree_L1);
      UInt64 num_high = num_cache_lines - num_low;
      SInt32 degree_high = (SInt32) ceil(degree_L1);

      // Approximate For Pr L1, Pr L2 configuration
      total_cache_line_replication_count[num_sharers + degree_low] += num_low;
      total_cache_line_replication_count[num_sharers + degree_high] += num_high;
   }

   // For Pr L1, Pr L2 configuration
   for (SInt32 i = 1; i <= (2*total_tiles); i++)
      _cache_line_replication_file << total_cache_line_replication_count[i] << ", ";
   _cache_line_replication_file << endl;

   _cache_line_replication_file << endl;
}

void
MemoryManager::checkDramDirectoryType()
{
   string dram_directory_type;
   try
   {
      dram_directory_type = Sim()->getCfg()->getString("dram_directory/directory_type");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read [dram_directory/type] from the cfg file");
   }

   LOG_ASSERT_ERROR(dram_directory_type == "full_map",
         "DRAM Directory type should be FULL_MAP for cache_line_replication to be measured, now (%s)",
         dram_directory_type.c_str());
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

      case DIRECTORY:
         getDramDirectoryCache()->getDVFS(frequency,voltage);
         break;

      default:
         rc = -1;
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
         rc = getL1ICache()->setDVFS(frequency,voltage_flag, curr_time);
         break;

      case L1_DCACHE:
         rc = getL1DCache()->setDVFS(frequency,voltage_flag, curr_time);
         break;

      case L2_CACHE:
         rc = getL2Cache()->setDVFS(frequency,voltage_flag, curr_time);
         break;

      case DIRECTORY:
         rc = getDramDirectoryCache()->setDVFS(frequency,voltage_flag, curr_time);
         break;

      default:
         rc = -1;
         break;
   }
   return rc;
}

}
