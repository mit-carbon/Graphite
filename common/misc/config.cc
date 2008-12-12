#include "config.h"

#define DEBUG

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
