#include "config.h"

#include "network_model_analytical_params.h"
#include "network_types.h"
#include "packet_type.h"

#define DEBUG

#include "pin.H"

extern LEVEL_BASE::KNOB<UInt32> g_knob_num_cores;
extern LEVEL_BASE::KNOB<UInt32> g_knob_total_cores;
extern LEVEL_BASE::KNOB<UInt32> g_knob_num_process;
extern LEVEL_BASE::KNOB<Boolean> g_knob_simarch_has_shared_mem;

using namespace std;

Config::Config()
{
   UInt32 i, j;

   // FIXME: These are temporary defaults.  Eventually, we'll load these from
   // a file or the command line.

   if (g_knob_num_process == 0) {
      cerr << "\nWARNING: Using compatibility mode for number of processes!\n"
	   << "  Assuming number of processes = 1.\n"
	   << "  Please use the -np command-line argument in the future.\n"
	   << endl;
      num_process = 1;
   } else {      
      num_process = g_knob_num_process;
   }
   my_proc_num = 0;
   // Default location for MCP is process 0
   MCP_process = 0;

   if (g_knob_total_cores == 0) {
      // Backwards compatibility mode (in case the user does not specify
      // the -tc command line argument)
      cerr << "\nWARNING: Using compatibility mode for total number of cores!\n"
	   << "  Assuming all cores are in one process.\n"
	   << "  Please use the -tc command-line argument in the future.\n"
	   << endl;
      total_cores = g_knob_num_cores;
   } else {
      total_cores = g_knob_total_cores;
   }

   // Sanity check on number of process and cores.  FIXME: For now, assume
   //  all processes have the same number of cores.
   assert((num_process*g_knob_num_cores) == total_cores);

   //Add one to account for the MCP
   total_cores += 1;

   num_modules = new UInt32[num_process];
   // FIXME: This assumes that every process has the same number of modules.
   //  Each entry should be filled in with a different value.
   for (i=0; i<num_process; i++) { num_modules[i] = total_cores; }  
  
   // Create an empty list for each process
   core_map = new CoreList[num_process];

   // FIXME: This code is temporary and is only here until we have a final
   //  mechanism for reading the list of cores in each process.
   // Since we only have one process, fill in its list with all the core #'s
   UInt32 k = 0;
   for (i=0; i<num_process; i++) {
      for (j=0; j<num_modules[i]; j++) {
	 		core_map[i].push_back(k++);
      }
   }

   // Create network parameters
   analytic_network_parms = new NetworkModelAnalyticalParameters();
   analytic_network_parms->Tw2 = 1; // single cycle between nodes in 2d mesh
   analytic_network_parms->s = 1; // single cycle switching time
   analytic_network_parms->n = 1; // 2-d mesh network
   analytic_network_parms->W = 32; // 32-bit wide channels
   analytic_network_parms->update_interval = 100000;
   analytic_network_parms->proc_cost = 100;

#ifdef DEBUG  
   for (i=0; i<num_process; i++) {   
      cout << "Process " << i << ": ";
      for (CLCI m = core_map[i].begin(); m != core_map[i].end(); m++)
	 		cout << "[" << *m << "]";
      cout << endl;
   }
#endif
}

Config::~Config()
{
   // Clean up the dynamic memory we allocated
   delete analytic_network_parms;
   delete [] num_modules;
   delete [] core_map;
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
