using namespace std;

#include "simulator.h"
#include "config.h"
#include "memory_manager_base.h"
#include "pr_l1_pr_l2_dram_directory_msi/memory_manager.h"
#include "pr_l1_pr_l2_dram_directory_mosi/memory_manager.h"
#include "log.h"

MemoryManagerBase* 
MemoryManagerBase::createMMU(std::string protocol_type,
      Core* core, Network* network, ShmemPerfModel* shmem_perf_model)
{
   CachingProtocol_t caching_protocol = parseProtocolType(protocol_type);

   switch (caching_protocol)
   {
      case PR_L1_PR_L2_DRAM_DIRECTORY_MSI:
         return new PrL1PrL2DramDirectoryMSI::MemoryManager(core, network, shmem_perf_model);

      case PR_L1_PR_L2_DRAM_DIRECTORY_MOSI:
         return new PrL1PrL2DramDirectoryMOSI::MemoryManager(core, network, shmem_perf_model);

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

vector<core_id_t>
MemoryManagerBase::getCoreListWithMemoryControllers()
{
   SInt32 num_memory_controllers = -1;
   string memory_controller_positions_from_cfg_file = "";

   SInt32 core_count;
   
   core_count = Config::getSingleton()->getTotalCores();
   try
   {
      num_memory_controllers = Sim()->getCfg()->getInt("perf_model/dram/num_controllers");
      memory_controller_positions_from_cfg_file = Sim()->getCfg()->getString("perf_model/dram/controller_positions");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Error reading number of memory controllers or controller positions");
   }

   LOG_ASSERT_ERROR(num_memory_controllers <= core_count, "Num Memory Controllers(%i), Num Cores(%i)",
         num_memory_controllers, core_count);

   if (num_memory_controllers != -1)
   {
      vector<core_id_t> core_list_from_cfg_file;
      parseMemoryControllerList(memory_controller_positions_from_cfg_file, core_list_from_cfg_file, core_count);

      LOG_ASSERT_ERROR((core_list_from_cfg_file.size() == 0) || (core_list_from_cfg_file.size() == (size_t) num_memory_controllers),
            "num_memory_controllers(%i), num_controller_positions specified(%i)",
            num_memory_controllers, core_list_from_cfg_file.size());

      if (core_list_from_cfg_file.size() > 0)
      {
         // Return what we read from the config file
         return core_list_from_cfg_file;
      }
      else
      {
         UInt32 l_models_memory_1 = 0;
         UInt32 l_models_memory_2 = 0;
         try
         {
            config::Config *cfg = Sim()->getCfg();
            l_models_memory_1 = NetworkModel::parseNetworkType(cfg->getString("network/memory_model_1"));
            l_models_memory_2 = NetworkModel::parseNetworkType(cfg->getString("network/memory_model_2"));
         }
         catch (...)
         {
            LOG_PRINT_ERROR("Exception while reading network model types.");
         }

         pair<bool, vector<core_id_t> > core_list_with_memory_controllers_1 = NetworkModel::computeMemoryControllerPositions(l_models_memory_1, num_memory_controllers, core_count);
         pair<bool, vector<core_id_t> > core_list_with_memory_controllers_2 = NetworkModel::computeMemoryControllerPositions(l_models_memory_2, num_memory_controllers, core_count);

         if (core_list_with_memory_controllers_1.first)
            return core_list_with_memory_controllers_1.second;
         else if (core_list_with_memory_controllers_2.first)
            return core_list_with_memory_controllers_2.second;
         else
         {
            // Return Any of them - Both the network models do not have specific positions
            return core_list_with_memory_controllers_1.second;
         }
      }
   }
   else
   {
      vector<core_id_t> core_list_with_memory_controllers;
      // All cores have memory controllers
      for (core_id_t i = 0; i < core_count; i++)
         core_list_with_memory_controllers.push_back(i);

      return core_list_with_memory_controllers;
   }
}

void
MemoryManagerBase::parseMemoryControllerList(string& memory_controller_positions, vector<core_id_t>& core_list_from_cfg_file, SInt32 core_count)
{
   if (memory_controller_positions == "")
      return;

   size_t i = 0;
   bool end_reached = false;

   while(!end_reached)
   {
      size_t position = memory_controller_positions.find(',', i);
      core_id_t core_num;

      if (position != string::npos)
      {
         // The end of the string has not been reached
         string core_num_str = memory_controller_positions.substr(i, position-i);
         core_num = atoi(core_num_str.c_str());
      }
      else
      {
         // The end of the string has been reached
         string core_num_str = memory_controller_positions.substr(i);
         core_num = atoi(core_num_str.c_str());
         end_reached = true;
      }
      
      LOG_ASSERT_ERROR(core_num < core_count, "core_num(%i), num_cores(%i)", core_num, core_count);
      core_list_from_cfg_file.push_back(core_num);

      i = position + 1;
   }
}

void
MemoryManagerBase::printCoreListWithMemoryControllers(vector<core_id_t>& core_list_with_memory_controllers)
{
   ostringstream core_list;
   for (vector<core_id_t>::iterator it = core_list_with_memory_controllers.begin(); it != core_list_with_memory_controllers.end(); it++)
   {
      core_list << *it << " ";
   }
   LOG_PRINT_WARNING("Core IDs' with memory controllers = (%s)", (core_list.str()).c_str());
}
