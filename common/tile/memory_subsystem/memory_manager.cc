#include "simulator.h"
#include "config.h"
#include "memory_manager.h"
#include "pr_l1_pr_l2_dram_directory_msi/memory_manager.h"
#include "pr_l1_pr_l2_dram_directory_mosi/memory_manager.h"
#include "pr_l1_sh_l2_msi/memory_manager.h"
#include "network_model.h"
#include "clock_converter.h"
#include "log.h"

// Static Members
CachingProtocolType MemoryManager::_caching_protocol_type;

MemoryManager::MemoryManager(Tile* tile)
   : _tile(tile)
   , _enabled(false)
{
   _network = _tile->getNetwork();
   _shmem_perf_model = new ShmemPerfModel();
   
   // Register call-backs
   _network->registerCallback(SHARED_MEM_1, MemoryManagerNetworkCallback, this);
   _network->registerCallback(SHARED_MEM_2, MemoryManagerNetworkCallback, this);
}

MemoryManager::~MemoryManager()
{
   _network->unregisterCallback(SHARED_MEM_1);
   _network->unregisterCallback(SHARED_MEM_2);
}

MemoryManager* 
MemoryManager::createMMU(std::string protocol_type, Tile* tile)
{
   _caching_protocol_type = parseProtocolType(protocol_type);

   switch (_caching_protocol_type)
   {
   case PR_L1_PR_L2_DRAM_DIRECTORY_MSI:
      return new PrL1PrL2DramDirectoryMSI::MemoryManager(tile);

   case PR_L1_PR_L2_DRAM_DIRECTORY_MOSI:
      return new PrL1PrL2DramDirectoryMOSI::MemoryManager(tile);

   case PR_L1_SH_L2_MSI:
      return new PrL1ShL2MSI::MemoryManager(tile);

   default:
      LOG_PRINT_ERROR("Unsupported Caching Protocol (%u)", _caching_protocol_type);
      return NULL;
   }
}

CachingProtocolType
MemoryManager::parseProtocolType(std::string& protocol_type)
{
   if (protocol_type == "pr_l1_pr_l2_dram_directory_msi")
      return PR_L1_PR_L2_DRAM_DIRECTORY_MSI;
   else if (protocol_type == "pr_l1_pr_l2_dram_directory_mosi")
      return PR_L1_PR_L2_DRAM_DIRECTORY_MOSI;
   else if (protocol_type == "pr_l1_sh_l2_msi")
      return PR_L1_SH_L2_MSI;
   else
      return NUM_CACHING_PROTOCOL_TYPES;
}

void MemoryManagerNetworkCallback(void* obj, NetPacket packet)
{
   MemoryManager *mm = (MemoryManager*) obj;
   assert(mm != NULL);
   mm->__handleMsgFromNetwork(packet);
}

bool
MemoryManager::__coreInitiateMemoryAccess(MemComponent::Type mem_component,
                                          Core::lock_signal_t lock_signal,
                                          Core::mem_op_t mem_op_type,
                                          IntPtr address, UInt32 offset,
                                          Byte* data_buf, UInt32 data_length,
                                          UInt64& curr_time, bool modeled)
{
   if (lock_signal != Core::UNLOCK)
      _lock.acquire();
   
   _shmem_perf_model->setCycleCount(curr_time);

   bool ret = coreInitiateMemoryAccess(mem_component, lock_signal, mem_op_type,
                                       address, offset, data_buf, data_length, modeled);

   curr_time = _shmem_perf_model->getCycleCount();

   if (lock_signal != Core::LOCK)
      _lock.release();

   return ret;
}

void
MemoryManager::__handleMsgFromNetwork(NetPacket& packet)
{
   _lock.acquire();

   // Convert from the network frequency domain to tile frequency domain
   NetworkModel* network_model = _network->getNetworkModelFromPacketType(packet.type);
   packet.time = convertCycleCount(packet.time, network_model->getFrequency(), _tile->getFrequency());

   _shmem_perf_model->setCycleCount(packet.time);

   switch (packet.type)
   {
   case SHARED_MEM_1:
   case SHARED_MEM_2:
      handleMsgFromNetwork(packet);
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized packet type (%u)", packet.type);
      break;
   }

   _lock.release();
}

void
MemoryManager::enableModels()
{
   _enabled = true;
   _shmem_perf_model->enable();
}

void
MemoryManager::disableModels()
{
   _enabled = false;
   _shmem_perf_model->disable();
}

void
MemoryManager::outputSummary(ostream& out)
{
}

void
MemoryManager::waitForAppThread()
{
   _sim_thread_sem.wait();
   _lock.acquire();
}

void
MemoryManager::wakeUpAppThread()
{
   _lock.release();
   _app_thread_sem.signal();
}

void
MemoryManager::waitForSimThread()
{
   _lock.release();
   _app_thread_sem.wait();
}

void
MemoryManager::wakeUpSimThread()
{
   _lock.acquire();
   _sim_thread_sem.signal();
}

void
MemoryManager::openCacheLineReplicationTraceFiles()
{
   switch (_caching_protocol_type)
   {
   case PR_L1_PR_L2_DRAM_DIRECTORY_MSI:
      PrL1PrL2DramDirectoryMSI::MemoryManager::openCacheLineReplicationTraceFiles();
      break;

   case PR_L1_PR_L2_DRAM_DIRECTORY_MOSI:
      PrL1PrL2DramDirectoryMOSI::MemoryManager::openCacheLineReplicationTraceFiles();
      break;

   case PR_L1_SH_L2_MSI:
   default:
      LOG_PRINT_ERROR("Caching Protocol (%u) does not support this feature", _caching_protocol_type);
      break;
   }
}

void
MemoryManager::closeCacheLineReplicationTraceFiles()
{
   switch (_caching_protocol_type)
   {
   case PR_L1_PR_L2_DRAM_DIRECTORY_MSI:
      PrL1PrL2DramDirectoryMSI::MemoryManager::closeCacheLineReplicationTraceFiles();
      break;

   case PR_L1_PR_L2_DRAM_DIRECTORY_MOSI:
      PrL1PrL2DramDirectoryMOSI::MemoryManager::closeCacheLineReplicationTraceFiles();
      break;

   case PR_L1_SH_L2_MSI:
   default:
      LOG_PRINT_ERROR("Caching Protocol (%u) does not support this feature", _caching_protocol_type);
      break;
   }
}

void
MemoryManager::outputCacheLineReplicationSummary()
{
   switch (_caching_protocol_type)
   {
   case PR_L1_PR_L2_DRAM_DIRECTORY_MSI:
      PrL1PrL2DramDirectoryMSI::MemoryManager::outputCacheLineReplicationSummary();
      break;

   case PR_L1_PR_L2_DRAM_DIRECTORY_MOSI:
      PrL1PrL2DramDirectoryMOSI::MemoryManager::outputCacheLineReplicationSummary();
      break;

   case PR_L1_SH_L2_MSI:
   default:
      LOG_PRINT_ERROR("Caching Protocol (%u) does not support this feature", _caching_protocol_type);
      break;
   }
}

vector<tile_id_t>
MemoryManager::getTileListWithMemoryControllers()
{
   string num_memory_controllers_str;
   string memory_controller_positions_from_cfg_file = "";

   UInt32 application_tile_count = Config::getSingleton()->getApplicationTiles();
   try
   {
      num_memory_controllers_str = Sim()->getCfg()->getString("dram/num_controllers");
      memory_controller_positions_from_cfg_file = Sim()->getCfg()->getString("dram/controller_positions");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Error reading number of memory controllers or controller positions");
   }

   UInt32 num_memory_controllers = (num_memory_controllers_str == "ALL") ? application_tile_count : convertFromString<UInt32>(num_memory_controllers_str);
   
   LOG_ASSERT_ERROR(num_memory_controllers <= application_tile_count, "Num Memory Controllers(%i), Num Application Tiles(%i)",
                    num_memory_controllers, application_tile_count);

   if (num_memory_controllers != application_tile_count)
   {
      vector<string> tile_list_from_cfg_file_str_form;
      vector<tile_id_t> tile_list_from_cfg_file;
      parseList(memory_controller_positions_from_cfg_file, tile_list_from_cfg_file_str_form, ",");

      // Do some type-cpnversions here
      for (vector<string>::iterator it = tile_list_from_cfg_file_str_form.begin();
            it != tile_list_from_cfg_file_str_form.end(); it ++)
      {
         tile_id_t tile_id = convertFromString<tile_id_t>(*it);
         tile_list_from_cfg_file.push_back(tile_id);
      }

      LOG_ASSERT_ERROR((tile_list_from_cfg_file.size() == 0) ||
            (tile_list_from_cfg_file.size() == (size_t) num_memory_controllers),
            "num_memory_controllers(%i), num_controller_positions specified(%i)",
            num_memory_controllers, tile_list_from_cfg_file.size());

      if (tile_list_from_cfg_file.size() > 0)
      {
         // Return what we read from the config file
         return tile_list_from_cfg_file;
      }
      else
      {
         UInt32 l_models_memory_1 = NetworkModel::parseNetworkType(Config::getSingleton()->getNetworkType(STATIC_NETWORK_MEMORY_1));
         UInt32 l_models_memory_2 = NetworkModel::parseNetworkType(Config::getSingleton()->getNetworkType(STATIC_NETWORK_MEMORY_2));

         pair<bool, vector<tile_id_t> > tile_list_with_memory_controllers_1 = NetworkModel::computeMemoryControllerPositions(l_models_memory_1, num_memory_controllers, application_tile_count);
         pair<bool, vector<tile_id_t> > tile_list_with_memory_controllers_2 = NetworkModel::computeMemoryControllerPositions(l_models_memory_2, num_memory_controllers, application_tile_count);

         if (tile_list_with_memory_controllers_1.first)
            return tile_list_with_memory_controllers_1.second;
         else if (tile_list_with_memory_controllers_2.first)
            return tile_list_with_memory_controllers_2.second;
         else
         {
            // Return Any of them - Both the network models do not have specific positions
            return tile_list_with_memory_controllers_1.second;
         }
      }
   }
   else
   {
      vector<tile_id_t> tile_list_with_memory_controllers;
      // All tiles have memory controllers
      for (tile_id_t i = 0; i < (tile_id_t) application_tile_count; i++)
         tile_list_with_memory_controllers.push_back(i);

      return tile_list_with_memory_controllers;
   }
}

void
MemoryManager::printTileListWithMemoryControllers(vector<tile_id_t>& tile_list_with_memory_controllers)
{
   ostringstream tile_list;
   for (vector<tile_id_t>::iterator it = tile_list_with_memory_controllers.begin(); it != tile_list_with_memory_controllers.end(); it++)
   {
      tile_list << *it << " ";
   }
   fprintf(stderr, "\n[[Graphite]] --> [ Tile IDs' with memory controllers = (%s) ]\n", (tile_list.str()).c_str());
}
