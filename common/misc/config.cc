#include "config.h"

#include "network_model_analytical_params.h"
#include "network_types.h"
#include "packet_type.h"
#include "simulator.h"

#include <sstream>
#include "log.h"
#define LOG_DEFAULT_RANK   -1
#define LOG_DEFAULT_MODULE CONFIG
extern Log *g_log;

#define DEBUG

UInt32 Config::m_knob_total_cores;
UInt32 Config::m_knob_num_process;
bool Config::m_knob_simarch_has_shared_mem;
std::string Config::m_knob_output_file;
bool Config::m_knob_enable_performance_modeling;
bool Config::m_knob_enable_dcache_modeling;
bool Config::m_knob_enable_icache_modeling;

using namespace std;

Config *Config::m_singleton;

Config *Config::getSingleton()
{
   assert(m_singleton != NULL);
   return m_singleton;
}

Config::Config()
      :
        m_current_process_num((UInt32)-1)
{

   m_knob_total_cores = Sim()->getCfg()->GetInt("general/total_cores");
   m_knob_num_process = Sim()->getCfg()->GetInt("general/num_processes");
   m_knob_simarch_has_shared_mem = Sim()->getCfg()->GetBool("general/enable_shared_mem");
   m_knob_output_file = Sim()->getCfg()->GetString("general/output_file");
   m_knob_enable_performance_modeling = Sim()->getCfg()->GetBool("general/enable_performance_modeling");
   m_knob_enable_dcache_modeling = Sim()->getCfg()->GetBool("general/enable_dcache_modeling");
   m_knob_enable_icache_modeling = Sim()->getCfg()->GetBool("general/enable_icache_modeling");
   m_num_processes = m_knob_num_process;
   m_total_cores = m_knob_total_cores;

   m_singleton = this;

   assert(m_num_processes > 0);
   assert(m_total_cores > 0);

   // Add one for the MCP
   m_total_cores += 1;

   GenerateCoreMap();

   // Create network parameters
   m_analytic_network_parms = new NetworkModelAnalyticalParameters();
   m_analytic_network_parms->Tw2 = 1; // single cycle between nodes in 2d mesh
   m_analytic_network_parms->s = 1; // single cycle switching time
   m_analytic_network_parms->n = 1; // 2-d mesh network
   m_analytic_network_parms->W = 32; // 32-bit wide channels
   m_analytic_network_parms->update_interval = 100000;
   m_analytic_network_parms->proc_cost = 100;

}

Config::~Config()
{
   // Clean up the dynamic memory we allocated
   delete m_analytic_network_parms;
   delete [] m_proc_to_core_list_map;
}

void Config::GenerateCoreMap()
{
   m_proc_to_core_list_map = new CoreList[m_num_processes];
   m_core_to_proc_map.resize(m_total_cores);

   // Stripe the cores across the processes
   UInt32 current_proc = 0;
   for (UInt32 i=0; i < m_total_cores - 1; i++)
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
      stringstream ss;
      ss << "Process " << i << ": (" << m_proc_to_core_list_map[i].size() << ") ";
      for (CLCI m = m_proc_to_core_list_map[i].begin(); m != m_proc_to_core_list_map[i].end(); m++)
         ss << "[" << *m << "]";
      LOG_PRINT(ss.str().c_str());
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

void Config::getNetworkModels(UInt32 *models) const
{
   models[STATIC_NETWORK_USER]   = NETWORK_ANALYTICAL_MESH;
   models[STATIC_NETWORK_MEMORY] = NETWORK_ANALYTICAL_MESH;
   models[STATIC_NETWORK_SYSTEM] = NETWORK_MAGIC;
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

void Config::getDisabledLogModules(set<string> &mods) const
{
//   mods.insert("TRANSPORT");
//   mods.insert("NETWORK");
//   mods.insert("CORE");
//   mods.insert("DRAMDIR");
//   mods.insert("MMU");
//   mods.insert("CORE_MANAGER");
//   mods.insert("PINSIM");
//   mods.insert("SHAREDMEM");
//   mods.insert("CONFIG");
//   mods.insert("SYSCALL");
}

const char *Config::getOutputFileName() const
{
   return m_knob_output_file.c_str();
}

void Config::updateCommToCoreMap(UInt32 comm_id, UInt32 core_id)
{
   m_comm_to_core_map[comm_id] = core_id;
   // for(CommToCoreMap::iterator iter = m_comm_to_core_map.begin(); 
   //       iter != m_comm_to_core_map.end(); iter++)
   // {
   //    LOG_PRINT("CoreMap: %d is now mapped to: %d",  iter->first, iter->second);
   // }
}

UInt32 Config::getCoreFromCommId(UInt32 comm_id)
{
   CommToCoreMap::iterator it = m_comm_to_core_map.find(comm_id);
   // if(it == m_comm_to_core_map.end())
   // {
   //    for(CommToCoreMap::iterator iter = m_comm_to_core_map.begin(); 
   //          iter != m_comm_to_core_map.end(); iter++)
   //    {
   //       LOG_PRINT("CoreMap: %d mapped to: %d",  iter->first, iter->second);
   //    }
   // }

   LOG_ASSERT_ERROR(it != m_comm_to_core_map.end(), "*ERROR* Lookup on comm_id: %d not found.", comm_id);
   return it->second;
}
