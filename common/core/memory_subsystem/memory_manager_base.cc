using namespace std;

#include "simulator.h"
#include "config.h"
#include "memory_manager_base.h"
#include "pr_l1_pr_l2_dram_directory_msi/memory_manager.h"
#include "pr_l1_pr_l2_dram_directory_mosi/memory_manager.h"
#include "pr_l1_pr_l1_pr_l2_dram_directory_msi/memory_manager.h"
#include "log.h"

MemoryManagerBase* 
MemoryManagerBase::createMMU(std::string protocol_type,
      Tile* tile, Network* network, ShmemPerfModel* shmem_perf_model)
{
   CachingProtocol_t caching_protocol = parseProtocolType(protocol_type);

   switch (caching_protocol)
   {
      case PR_L1_PR_L2_DRAM_DIRECTORY_MSI:
         return new PrL1PrL2DramDirectoryMSI::MemoryManager(tile, network, shmem_perf_model);

      case PR_L1_PR_L2_DRAM_DIRECTORY_MOSI:
         return new PrL1PrL2DramDirectoryMOSI::MemoryManager(tile, network, shmem_perf_model);

      case PR_L1_PR_L1_PR_L2_DRAM_DIRECTORY_MSI:
         return new PrL1PrL1PrL2DramDirectoryMSI::MemoryManager(tile, network, shmem_perf_model);

      default:
         LOG_PRINT_ERROR("Unsupported Caching Protocol (%u)", caching_protocol);
         return NULL;
   }
}

MemoryManagerBase::CachingProtocol_t
MemoryManagerBase::parseProtocolType(std::string& protocol_type)
{
   if (protocol_type == "pr_l1_pr_l2_dram_directory_msi")
      return PR_L1_PR_L2_DRAM_DIRECTORY_MSI;
   else if (protocol_type == "pr_l1_pr_l2_dram_directory_mosi")
      return PR_L1_PR_L2_DRAM_DIRECTORY_MOSI;
   else if (protocol_type == "pr_l1_pr_l1_pr_l2_dram_directory_msi")
      return PR_L1_PR_L1_PR_L2_DRAM_DIRECTORY_MSI;
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

vector<tile_id_t>
MemoryManagerBase::getTileListWithMemoryControllers()
{
   SInt32 num_memory_controllers = -1;
   string memory_controller_positions_from_cfg_file = "";

   SInt32 tile_count;
   
   tile_count = Config::getSingleton()->getTotalTiles();
   try
   {
      num_memory_controllers = Sim()->getCfg()->getInt("perf_model/dram/num_controllers");
      memory_controller_positions_from_cfg_file = Sim()->getCfg()->getString("perf_model/dram/controller_positions");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Error reading number of memory controllers or controller positions");
   }

   LOG_ASSERT_ERROR(num_memory_controllers <= tile_count, "Num Memory Controllers(%i), Num Tiles(%i)",
         num_memory_controllers, tile_count);

   if (num_memory_controllers != -1)
   {
      vector<string> tile_list_from_cfg_file_str_form;
      vector<tile_id_t> tile_list_from_cfg_file;
      parseList(memory_controller_positions_from_cfg_file, tile_list_from_cfg_file_str_form, ",");

      // Do some type-cpnversions here
      for (vector<string>::iterator it = tile_list_from_cfg_file_str_form.begin(); \
            it != tile_list_from_cfg_file_str_form.end(); it ++)
      {
         tile_id_t tile_id;
         convertFromString<tile_id_t>(tile_id, *it);
         tile_list_from_cfg_file.push_back(tile_id);
      }

      LOG_ASSERT_ERROR((tile_list_from_cfg_file.size() == 0) || \
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

         pair<bool, vector<tile_id_t> > tile_list_with_memory_controllers_1 = NetworkModel::computeMemoryControllerPositions(l_models_memory_1, num_memory_controllers, tile_count);
         pair<bool, vector<tile_id_t> > tile_list_with_memory_controllers_2 = NetworkModel::computeMemoryControllerPositions(l_models_memory_2, num_memory_controllers, tile_count);

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
      for (tile_id_t i = 0; i < tile_count; i++)
         tile_list_with_memory_controllers.push_back(i);

      return tile_list_with_memory_controllers;
   }
}

void
MemoryManagerBase::printTileListWithMemoryControllers(vector<tile_id_t>& tile_list_with_memory_controllers)
{
   ostringstream tile_list;
   for (vector<tile_id_t>::iterator it = tile_list_with_memory_controllers.begin(); it != tile_list_with_memory_controllers.end(); it++)
   {
      tile_list << *it << " ";
   }
   fprintf(stderr, "Tile IDs' with memory controllers = (%s)\n", (tile_list.str()).c_str());
}
