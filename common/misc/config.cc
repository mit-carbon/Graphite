#include "config.h"

Config::Config()
{
   UInt32 i;

   // These are temporary defaults.  Eventually, we'll load these from
   // a file or the command line.

   num_process = 1;
   my_proc_num = 0;

   if (g_knob_total_cores == 0) {
      // Backwards compatibility mode (in case the user does not specify
      // the -tc command line argument)
      cerr << "WARNING: Using compatibility mode for total number of cores!\n"
	   << "  Please specify -tc command-line argument in the future."
	   << endl;
      total_cores = g_knob_num_cores;
   } else {
      total_cores = g_knob_total_cores;
   }

   num_modules = new UInt32[num_process];
   // FIXME: This is not really correct if there is more than 1 process.
   //  Each entry should be filled in with a different value.
   for (i=0; i<num_process; i++) { num_modules[i] = g_knob_num_cores; }  
  
   // Create an empty list for each process
   core_map = new CoreList[num_process];

   // FIXME: This code is temporary and is only here until we have a final
   //  mechanism for reading the list of cores in each process.
   // Since we only have one process, fill in its list with all the core #'s
   for (i=0; i<num_modules[0]; i++) { core_map[0].push_back(i); }

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
