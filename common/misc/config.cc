#include "config.h"

#include "network_model_analytical_params.h"
#include "network_types.h"
#include "packet_type.h"

#include <sstream>
#include "log.h"
#define LOG_DEFAULT_RANK   -1
#define LOG_DEFAULT_MODULE CONFIG
extern Log *g_log;

#define DEBUG

#include "pin.H"

extern LEVEL_BASE::KNOB<UInt32> g_knob_total_cores;
extern LEVEL_BASE::KNOB<UInt32> g_knob_num_process;
extern LEVEL_BASE::KNOB<Boolean> g_knob_simarch_has_shared_mem;
extern LEVEL_BASE::KNOB<std::string> g_knob_output_file;

using namespace std;

Config::Config()
   : num_process(g_knob_num_process),
   total_cores(g_knob_total_cores)
{
   assert(num_process > 0);
   assert(total_cores > 0);

   // Add one for the MCP
   total_cores += 1;

   // FIXME: This is a bit of a hack to put this here, but we need it
   // for logging in the rest of Config's constructor.
   g_log = new Log(total_cores);

   GenerateCoreMap();


   // Create network parameters
   analytic_network_parms = new NetworkModelAnalyticalParameters();
   analytic_network_parms->Tw2 = 1; // single cycle between nodes in 2d mesh
   analytic_network_parms->s = 1; // single cycle switching time
   analytic_network_parms->n = 1; // 2-d mesh network
   analytic_network_parms->W = 32; // 32-bit wide channels
   analytic_network_parms->update_interval = 100000;
   analytic_network_parms->proc_cost = 100;

}

Config::~Config()
{
   // Clean up the dynamic memory we allocated
   delete analytic_network_parms;
   delete [] proc_to_core_list_map;
}

void Config::GenerateCoreMap()
{
   proc_to_core_list_map = new CoreList[num_process];
   core_to_proc_map.resize(total_cores);

   // Stripe the cores across the processes
   UInt32 current_proc = 0;
   for (UInt32 i=0; i < total_cores - 1; i++)
   {
      core_to_proc_map[i] = current_proc;
      proc_to_core_list_map[current_proc].push_back(i);
      current_proc++;
      current_proc %= num_process;
   }

   // Add one for the MCP
   proc_to_core_list_map[0].push_back(total_cores - 1);
   core_to_proc_map[total_cores - 1] = 0;

   // Log the map we just created
   fprintf(stderr, "Num Process: %d\n", num_process);
   for (UInt32 i=0; i<num_process; i++) {   
      stringstream ss;
      ss << "Process " << i << ": (" << proc_to_core_list_map[i].size() << ") ";
      for (CLCI m = proc_to_core_list_map[i].begin(); m != proc_to_core_list_map[i].end(); m++)
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

Boolean Config::isSimulatingSharedMemory() const
{
   return (Boolean)g_knob_simarch_has_shared_mem;
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
   return g_knob_output_file.Value().c_str();
}
