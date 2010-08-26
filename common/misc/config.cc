#include "config.h"

#include "network_model.h"
#include "network_types.h"
#include "packet_type.h"
#include "simulator.h"
#include "utils.h"

#include <sstream>
#include "log.h"

#define DEBUG

UInt32 Config::m_knob_total_cores;
UInt32 Config::m_knob_num_process;
bool Config::m_knob_simarch_has_shared_mem;
std::string Config::m_knob_output_file;
bool Config::m_knob_enable_performance_modeling;
bool Config::m_knob_enable_dcache_modeling;
bool Config::m_knob_enable_icache_modeling;
bool Config::m_knob_enable_power_modeling;

using namespace std;

Config *Config::m_singleton;

Config *Config::getSingleton()
{
   assert(m_singleton != NULL);
   return m_singleton;
}

Config::Config()
      : m_current_process_num((UInt32)-1)
{
   // NOTE: We can NOT use logging in the config constructor! The log
   // has not been instantiated at this point!
   try
   {
      m_knob_total_cores = Sim()->getCfg()->getInt("general/total_cores");
      m_knob_num_process = Sim()->getCfg()->getInt("general/num_processes");
      m_knob_simarch_has_shared_mem = Sim()->getCfg()->getBool("general/enable_shared_mem");
      m_knob_output_file = Sim()->getCfg()->getString("general/output_file");
      m_knob_enable_performance_modeling = Sim()->getCfg()->getBool("general/enable_performance_modeling");
      // TODO: these should be removed and queried directly from the cache
      m_knob_enable_dcache_modeling = Sim()->getCfg()->getBool("general/enable_dcache_modeling");
      m_knob_enable_icache_modeling = Sim()->getCfg()->getBool("general/enable_icache_modeling");
      m_knob_enable_power_modeling = Sim()->getCfg()->getBool("general/enable_power_modeling");

      // Simulation Mode
      m_simulation_mode = parseSimulationMode(Sim()->getCfg()->getString("general/mode"));
   }
   catch(...)
   {
      fprintf(stderr, "ERROR: Config obtained a bad value from config.\n");
      exit(EXIT_FAILURE);
   }

   m_num_processes = m_knob_num_process;
   m_total_cores = m_knob_total_cores;
   m_application_cores = m_total_cores;

   if ((m_simulation_mode == LITE) && (m_num_processes > 1))
   {
      fprintf(stderr, "ERROR: Use only 1 process in lite mode\n");
      exit(EXIT_FAILURE);
   }

   m_singleton = this;

   assert(m_num_processes > 0);
   assert(m_total_cores > 0);

   // Add one for the MCP
   m_total_cores += 1;

   // Add the thread-spawners (one for each process)
   if (m_simulation_mode == FULL)
      m_total_cores += m_num_processes;

   // Parse Network Models - Need to be done here to initialize the network models 
   parseNetworkParameters();

   // Adjust the number of cores corresponding to the network model we use
   m_total_cores = getNearestAcceptableCoreCount(m_total_cores);

   // Parse Core Models
   parseCoreParameters();

   // Length of a core identifier
   m_core_id_length = computeCoreIDLength(m_total_cores);

   GenerateCoreMap();
}

Config::~Config()
{
   // Clean up the dynamic memory we allocated
   delete [] m_proc_to_core_list_map;
}

UInt32 Config::getTotalCores()
{
   return m_total_cores;
}

UInt32 Config::getApplicationCores()
{
   return m_application_cores;
}

core_id_t Config::getThreadSpawnerCoreNum(UInt32 proc_num)
{
   if (m_simulation_mode == FULL)
      return (getTotalCores() - (1 + getProcessCount() - proc_num));
   else
      return INVALID_CORE_ID;
}

core_id_t Config::getCurrentThreadSpawnerCoreNum()
{
   if (m_simulation_mode == FULL)
      return (getTotalCores() - (1 + getProcessCount() - getCurrentProcessNum()));
   else
      return INVALID_CORE_ID;
}

UInt32 Config::computeCoreIDLength(UInt32 core_count)
{
   UInt32 num_bits = ceilLog2(core_count);
   if ((num_bits % 8) == 0)
      return (num_bits / 8);
   else
      return (num_bits / 8) + 1;
}

void Config::GenerateCoreMap()
{
   vector<CoreList> process_to_core_mapping = computeProcessToCoreMapping();
   
   m_proc_to_core_list_map = new CoreList[m_num_processes];
   m_core_to_proc_map.resize(m_total_cores);

   // Populate the data structures for non-thread-spawner and non-MCP cores
   assert(process_to_core_mapping.size() == m_num_processes);
   for (UInt32 i = 0; i < m_num_processes; i++)
   {
      CoreList::iterator core_it;
      for (core_it = process_to_core_mapping[i].begin(); core_it != process_to_core_mapping[i].end(); core_it++)
      {
         if ((*core_it) < (SInt32) (m_total_cores - m_num_processes - 1))
         {
            m_core_to_proc_map[*core_it] = i;
            m_proc_to_core_list_map[i].push_back(*core_it);
         }
      }
   }
    
   // Assign the thread-spawners to cores
   // Thread-spawners occupy core-id's (m_total_cores - m_num_processes - 1) to (m_total_cores - 2)
   UInt32 current_proc = 0;
   for (UInt32 i = (m_total_cores - m_num_processes - 1); i < (m_total_cores - 1); i++)
   {
      assert((current_proc >= 0) && (current_proc < m_num_processes));
      m_core_to_proc_map[i] = current_proc;
      m_proc_to_core_list_map[current_proc].push_back(i);
      current_proc++;
   }
   
   // Add one for the MCP
   m_core_to_proc_map[m_total_cores - 1] = 0;
   m_proc_to_core_list_map[0].push_back(m_total_cores - 1);

   // printProcessToCoreMapping();
}

vector<Config::CoreList>
Config::computeProcessToCoreMapping()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      UInt32 network_model = NetworkModel::parseNetworkType(Config::getSingleton()->getNetworkType(i));
      pair<bool,vector<CoreList> > process_to_core_mapping_struct = NetworkModel::computeProcessToCoreMapping(network_model);
      if (process_to_core_mapping_struct.first)
      {
         switch(network_model)
         {
            case NETWORK_ATAC_CLUSTER:
               return process_to_core_mapping_struct.second;

            case NETWORK_EMESH_HOP_BY_HOP_BASIC:
            case NETWORK_EMESH_HOP_BY_HOP_BROADCAST_TREE:
               return process_to_core_mapping_struct.second;
               break;

            default:
               LOG_PRINT_ERROR("Unrecognized Network Type(%u)", network_model);
               break;
         }
      }
   }
   
   vector<CoreList> process_to_core_mapping(m_num_processes);
   if (process_to_core_mapping.size() == 0)
   {
      UInt32 current_proc = 0;
      for (UInt32 i = 0; i < m_total_cores; i++)
      {
         process_to_core_mapping[current_proc].push_back(i);
         current_proc = (current_proc + 1) % m_num_processes;
      }
   }
   return process_to_core_mapping;
}

void Config::printProcessToCoreMapping()
{
   UInt32 curr_process_num = atoi(getenv("CARBON_PROCESS_INDEX"));
   if (curr_process_num == 0)
   {
      for (UInt32 i = 0; i < m_num_processes; i++)
      {
         fprintf(stderr, "\nProcess(%u): %u\n", i, (UInt32) m_proc_to_core_list_map[i].size());
         for (CoreList::iterator core_it = m_proc_to_core_list_map[i].begin(); \
               core_it != m_proc_to_core_list_map[i].end(); core_it++)
         {
            fprintf(stderr, "%i, ", *core_it);
         }
         fprintf(stderr, "\n\n");
      }
   }
   exit(-1);
}

void Config::logCoreMap()
{
   // Log the map we just created
   LOG_PRINT("Process num: %d\n", m_num_processes);
   for (UInt32 i=0; i < m_num_processes; i++)
   {
      LOG_ASSERT_ERROR(!m_proc_to_core_list_map[i].empty(),
                       "Process %u assigned zero cores.", i);

      stringstream ss;
      ss << "Process " << i << ": (" << m_proc_to_core_list_map[i].size() << ") ";
      for (CLCI m = m_proc_to_core_list_map[i].begin(); m != m_proc_to_core_list_map[i].end(); m++)
         ss << "[" << *m << "]";
      LOG_PRINT(ss.str().c_str());
   }
}

SInt32 Config::getIndexFromCoreID(UInt32 proc_num, core_id_t core_id)
{ 
   CoreList core_list = getCoreListForProcess(proc_num);
   for (UInt32 i = 0; i < core_list.size(); i++)
   {
      if (core_list[i] == core_id)
         return (SInt32) i;
   }
   return -1;
}

core_id_t Config::getCoreIDFromIndex(UInt32 proc_num, SInt32 index)
{
   CoreList core_list = getCoreListForProcess(proc_num);
   if (index < ((SInt32) core_list.size()))
   {
      return core_list[index];
   }
   else
   {
      return -1;
   }
}

// Parse XML config file and use it to fill in config state.  Only modifies
// fields specified in the config file.  Therefore, this method can be used
// to override only the specific options given in the file.
void Config::loadFromFile(char* filename)
{
   return;
}

// Fill in config state from command-line arguments.  Only modifies fields
// specified on the command line.  Therefore, this method can be used to
// override only the specific options given.
void Config::loadFromCmdLine()
{
   return;
}

bool Config::isSimulatingSharedMemory() const
{
   return (bool)m_knob_simarch_has_shared_mem;
}

bool Config::getEnablePerformanceModeling() const
{
   return (bool)m_knob_enable_performance_modeling;
}

bool Config::getEnableDCacheModeling() const
{
   return (bool)m_knob_enable_dcache_modeling;
}

bool Config::getEnableICacheModeling() const
{
   return (bool)m_knob_enable_icache_modeling;
}

bool Config::getEnablePowerModeling() const
{
   return (bool)m_knob_enable_power_modeling;
}

std::string Config::getOutputFileName() const
{
   return formatOutputFileName(m_knob_output_file);
}

std::string Config::formatOutputFileName(string filename) const
{
   return Sim()->getCfg()->getString("general/output_dir",".") + "/" + filename;
}

void Config::updateCommToCoreMap(UInt32 comm_id, core_id_t core_id)
{
   m_comm_to_core_map[comm_id] = core_id;
}

UInt32 Config::getCoreFromCommId(UInt32 comm_id)
{
   CommToCoreMap::iterator it = m_comm_to_core_map.find(comm_id);
   return it == m_comm_to_core_map.end() ? INVALID_CORE_ID : it->second;
}

Config::SimulationMode Config::parseSimulationMode(string mode)
{
   if (mode == "full")
      return FULL;
   else if (mode == "lite")
      return LITE;
   else
      LOG_PRINT_ERROR("Unrecognized Simulation Mode(%s)", mode.c_str());

   return NUM_SIMULATION_MODES;
}

void Config::parseCoreParameters()
{
   // Default values are as follows:
   // 1) Number of cores -> Number of application cores
   // 2) Frequency -> 1 GHz
   // 3) Core Type -> simple

   const UInt32 DEFAULT_NUM_CORES = getApplicationCores();
   const float DEFAULT_FREQUENCY = 1;
   const string DEFAULT_CORE_TYPE = "magic";
   const string DEFAULT_CACHE_TYPE = "T1";

   string core_parameter_tuple_str;
   vector<string> core_parameter_tuple_vec;
   try
   {
      core_parameter_tuple_str = Sim()->getCfg()->getString("perf_model/core/model_list");
   }
   catch(...)
   {
      fprintf(stderr, "Could not read perf_model/core/model_list from the cfg file\n");
      exit(EXIT_FAILURE);
   }

   UInt32 num_initialized_cores = 0;

   parseList(core_parameter_tuple_str, core_parameter_tuple_vec, "<>");
   
   for (vector<string>::iterator tuple_it = core_parameter_tuple_vec.begin(); \
         tuple_it != core_parameter_tuple_vec.end(); tuple_it++)
   {
      // Initializing using default values
      UInt32 num_cores = DEFAULT_NUM_CORES;
      float frequency = DEFAULT_FREQUENCY;
      string core_type = DEFAULT_CORE_TYPE;
      string l1_icache_type = DEFAULT_CACHE_TYPE;
      string l1_dcache_type = DEFAULT_CACHE_TYPE;
      string l2_cache_type = DEFAULT_CACHE_TYPE;

      vector<string> core_parameter_tuple;
      parseList(*tuple_it, core_parameter_tuple, ",");
     
      SInt32 param_num = 0; 
      for (vector<string>::iterator param_it = core_parameter_tuple.begin(); \
            param_it != core_parameter_tuple.end(); param_it ++)
      {
         if (*param_it != "default")
         {
            switch (param_num)
            {
               case 0:
                  convertFromString<UInt32>(num_cores, *param_it);
                  break;

               case 1:
                  convertFromString<float>(frequency, *param_it);
                  break;

               case 2:
                  core_type = trimSpaces(*param_it);
                  break;

               case 3:
                  l1_icache_type = trimSpaces(*param_it);
                  break;

               case 4:
                  l1_dcache_type = trimSpaces(*param_it);
                  break;

               case 5:
                  l2_cache_type = trimSpaces(*param_it);
                  break;

               default:
                  fprintf(stderr, "Tuple encountered with (%i) parameters\n", param_num);
                  exit(EXIT_FAILURE);
                  break;
            }
         }
         param_num ++;
      }

      // Append these values to an internal list
      for (UInt32 i = num_initialized_cores; i < num_initialized_cores + num_cores; i++)
      {
         m_core_parameters_vec.push_back(CoreParameters(core_type, frequency, \
                  l1_icache_type, l1_dcache_type, l2_cache_type));
      }
      num_initialized_cores += num_cores;

      if (num_initialized_cores > getApplicationCores())
      {
         fprintf(stderr, "num initialized cores(%u), num application cores(%u)\n",
            num_initialized_cores, getApplicationCores());
         exit(EXIT_FAILURE);
      }
   }
   
   if (num_initialized_cores != getApplicationCores())
   {
      fprintf(stderr, "num initialized cores(%u), num application cores(%u)\n",
         num_initialized_cores, getApplicationCores());
      exit(EXIT_FAILURE);
   }

   // MCP, thread spawner and misc cores
   for (UInt32 i = getApplicationCores(); i < getTotalCores(); i++)
   {
      m_core_parameters_vec.push_back(CoreParameters(DEFAULT_CORE_TYPE, DEFAULT_FREQUENCY, \
               DEFAULT_CACHE_TYPE, DEFAULT_CACHE_TYPE, DEFAULT_CACHE_TYPE));
   }
}

void Config::parseNetworkParameters()
{
   const string DEFAULT_NETWORK_TYPE = "magic";
   const float DEFAULT_FREQUENCY = 1;           // In GHz

   string network_parameters_list[NUM_STATIC_NETWORKS];
   try
   {
      config::Config *cfg = Sim()->getCfg();
      network_parameters_list[STATIC_NETWORK_USER_1] = cfg->getString("network/user_model_1");
      network_parameters_list[STATIC_NETWORK_USER_2] = cfg->getString("network/user_model_2");
      network_parameters_list[STATIC_NETWORK_MEMORY_1] = cfg->getString("network/memory_model_1");
      network_parameters_list[STATIC_NETWORK_MEMORY_2] = cfg->getString("network/memory_model_2");
      network_parameters_list[STATIC_NETWORK_SYSTEM] = cfg->getString("network/system_model");
   }
   catch (...)
   {
      fprintf(stderr, "Unable to read network parameters from the cfg file\n");
      exit(EXIT_FAILURE);
   }

   for (SInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_parameters_vec.push_back(NetworkParameters(network_parameters_list[i], DEFAULT_FREQUENCY));
   }
}

string Config::getCoreType(core_id_t core_id)
{
   LOG_ASSERT_ERROR(core_id < ((SInt32) getTotalCores()),
         "core_id(%i), total cores(%u)", core_id, getTotalCores());
   LOG_ASSERT_ERROR(m_core_parameters_vec.size() == getTotalCores(),
         "m_core_parameters_vec.size(%u), total cores(%u)",
         m_core_parameters_vec.size(), getTotalCores());

   return m_core_parameters_vec[core_id].getType();
}

string Config::getL1ICacheType(core_id_t core_id)
{
   LOG_ASSERT_ERROR(core_id < ((SInt32) getTotalCores()),
         "core_id(%i), total cores(%u)", core_id, getTotalCores());
   LOG_ASSERT_ERROR(m_core_parameters_vec.size() == getTotalCores(),
         "m_core_parameters_vec.size(%u), total cores(%u)",
         m_core_parameters_vec.size(), getTotalCores());

   return m_core_parameters_vec[core_id].getL1ICacheType();
}

string Config::getL1DCacheType(core_id_t core_id)
{
   LOG_ASSERT_ERROR(core_id < ((SInt32) getTotalCores()),
         "core_id(%i), total cores(%u)", core_id, getTotalCores());
   LOG_ASSERT_ERROR(m_core_parameters_vec.size() == getTotalCores(),
         "m_core_parameters_vec.size(%u), total cores(%u)",
         m_core_parameters_vec.size(), getTotalCores());

   return m_core_parameters_vec[core_id].getL1DCacheType();
}

string Config::getL2CacheType(core_id_t core_id)
{
   LOG_ASSERT_ERROR(core_id < ((SInt32) getTotalCores()),
         "core_id(%i), total cores(%u)", core_id, getTotalCores());
   LOG_ASSERT_ERROR(m_core_parameters_vec.size() == getTotalCores(),
         "m_core_parameters_vec.size(%u), total cores(%u)",
         m_core_parameters_vec.size(), getTotalCores());

   return m_core_parameters_vec[core_id].getL2CacheType();
}

volatile float Config::getCoreFrequency(core_id_t core_id)
{
   LOG_ASSERT_ERROR(core_id < ((SInt32) getTotalCores()),
         "core_id(%i), total cores(%u)", core_id, getTotalCores());
   LOG_ASSERT_ERROR(m_core_parameters_vec.size() == getTotalCores(),
         "m_core_parameters_vec.size(%u), total cores(%u)",
         m_core_parameters_vec.size(), getTotalCores());

   return m_core_parameters_vec[core_id].getFrequency();
}

void Config::setCoreFrequency(core_id_t core_id, volatile float frequency)
{
   LOG_ASSERT_ERROR(core_id < ((SInt32) getTotalCores()),
         "core_id(%i), total cores(%u)", core_id, getTotalCores());
   LOG_ASSERT_ERROR(m_core_parameters_vec.size() == getTotalCores(),
         "m_core_parameters_vec.size(%u), total cores(%u)",
         m_core_parameters_vec.size(), getTotalCores());

   return m_core_parameters_vec[core_id].setFrequency(frequency);
}

string Config::getNetworkType(SInt32 network_id)
{
   LOG_ASSERT_ERROR(m_network_parameters_vec.size() == NUM_STATIC_NETWORKS,
         "m_network_parameters_vec.size(%u), NUM_STATIC_NETWORKS(%u)",
         m_network_parameters_vec.size(), NUM_STATIC_NETWORKS);

   return m_network_parameters_vec[network_id].getType();
}

UInt32 Config::getNearestAcceptableCoreCount(UInt32 core_count)
{
   UInt32 nearest_acceptable_core_count = 0;
   
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      UInt32 network_model = NetworkModel::parseNetworkType(Config::getSingleton()->getNetworkType(i));
      pair<bool,SInt32> core_count_constraints = NetworkModel::computeCoreCountConstraints(network_model, (SInt32) core_count);
      if (core_count_constraints.first)
      {
         // Network Model has core count constraints
         if ((nearest_acceptable_core_count != 0) && 
             (core_count_constraints.second != (SInt32) nearest_acceptable_core_count))
         {
            fprintf(stderr, "Problem using the network models specified in the configuration file\n");
            exit(EXIT_FAILURE);
         }
         else
         {
            nearest_acceptable_core_count = core_count_constraints.second;
         }
      }
   }

   if (nearest_acceptable_core_count == 0)
      nearest_acceptable_core_count = core_count;

   return nearest_acceptable_core_count;
}
