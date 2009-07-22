#include "config.h"

#include "network_model.h"
#include "network_types.h"
#include "packet_type.h"
#include "simulator.h"

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
UInt32 Config::m_knob_cache_line_size;

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
      m_knob_cache_line_size = Sim()->getCfg()->getInt("cache/line_size");
   }
   catch(...)
   {
      fprintf(stderr, "Config obtained a bad value from config.\n");
      assert(false);
   }
   
   m_num_processes = m_knob_num_process;
   m_total_cores = m_knob_total_cores;

   m_singleton = this;

   assert(m_num_processes > 0);
   assert(m_total_cores > 0);

   // Add one for the MCP
   m_total_cores += 1;

   // Add the thread-spawners (one for each process)
   m_total_cores += m_num_processes;

   GenerateCoreMap();
}

Config::~Config()
{
   // Clean up the dynamic memory we allocated
   delete [] m_proc_to_core_list_map;
}


void Config::GenerateCoreMap()
{
   m_proc_to_core_list_map = new CoreList[m_num_processes];
   m_core_to_proc_map.resize(m_total_cores);

   // Stripe the cores across the processes
   UInt32 current_proc = 0;
   for (UInt32 i = 0; i < (m_total_cores - m_num_processes - 1) ; i++)
   {
      m_core_to_proc_map [i] = current_proc;
      m_proc_to_core_list_map[current_proc].push_back(i);
      current_proc++;
      current_proc %= m_num_processes;
   }

   // Assign the thread-spawners to cores
   // Thread-spawners occupy core-id's (m_total_cores - m_num_processes - 1) to (m_total_cores - 2)
   current_proc = 0;
   for (UInt32 i = (m_total_cores - m_num_processes - 1); i < (m_total_cores - 1); i++)
   {
      m_core_to_proc_map[i] = current_proc;
      m_proc_to_core_list_map[current_proc].push_back(i);
      current_proc++;
      current_proc %= m_num_processes;
   }
   
   // Add one for the MCP
   m_proc_to_core_list_map[0].push_back(m_total_cores - 1);
   m_core_to_proc_map[m_total_cores - 1] = 0;
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

UInt32 Config::getCacheLineSize() const
{
   return (UInt32) m_knob_cache_line_size;
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

void Config::getNetworkModels(UInt32 *models) const
{
   try
   {
      config::Config *cfg = Sim()->getCfg();
      models[STATIC_NETWORK_USER]   = NetworkModel::parseNetworkType(cfg->getString("network/user_model"));
      models[STATIC_NETWORK_MEMORY] = NetworkModel::parseNetworkType(cfg->getString("network/memory_model"));
      models[STATIC_NETWORK_SYSTEM] = NetworkModel::parseNetworkType(cfg->getString("network/system_model"));
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Exception while reading network model types.");
   }
}
