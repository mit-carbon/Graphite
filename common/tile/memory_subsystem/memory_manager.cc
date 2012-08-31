using namespace std;

#include "simulator.h"
#include "config.h"
#include "memory_manager.h"
#include "pr_l1_pr_l2_dram_directory_msi/memory_manager.h"
#include "pr_l1_pr_l2_dram_directory_mosi/memory_manager.h"
#include "pr_l1_sh_l2_msi/memory_manager.h"
#include "network_model.h"
#include "log.h"

// Static Members
CachingProtocolType MemoryManager::_caching_protocol_type;

MemoryManager::MemoryManager(Tile* tile, Network* network, ShmemPerfModel* shmem_perf_model)
   : _tile(tile)
   , _network(network)
   , _shmem_perf_model(shmem_perf_model)
{}

MemoryManager::~MemoryManager()
{}

MemoryManager* 
MemoryManager::createMMU(std::string protocol_type,
      Tile* tile, Network* network, ShmemPerfModel* shmem_perf_model)
{
   _caching_protocol_type = parseProtocolType(protocol_type);

   switch (_caching_protocol_type)
   {
   case PR_L1_PR_L2_DRAM_DIRECTORY_MSI:
      return new PrL1PrL2DramDirectoryMSI::MemoryManager(tile, network, shmem_perf_model);

   case PR_L1_PR_L2_DRAM_DIRECTORY_MOSI:
      return new PrL1PrL2DramDirectoryMOSI::MemoryManager(tile, network, shmem_perf_model);

   case PR_L1_SH_L2_MSI:
      return new PrL1ShL2MSI::MemoryManager(tile, network, shmem_perf_model);

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
