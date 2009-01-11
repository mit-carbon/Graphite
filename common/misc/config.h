// config.h
//
// The Config class is used to set, store, and retrieve all the configurable
// parameters of the simulator.
//
// Initial creation: Sep 18, 2008 by jasonm

#ifndef CONFIG_H
#define CONFIG_H

#include <list>
#include <iostream>
#include <cassert>
#include "pin.H"
#include "fixed_types.h"

extern LEVEL_BASE::KNOB<UINT32> g_knob_num_cores;
extern LEVEL_BASE::KNOB<UINT32> g_knob_total_cores;
extern LEVEL_BASE::KNOB<UINT32> g_knob_num_process;

struct NetworkMeshAnalyticalParameters;

class Config {
 public:
   typedef list<UInt32> CoreList;
   typedef list<UInt32>::const_iterator CLCI;
 private:
   UInt32  num_process;          // Total number of processes (incl myself)
   UInt32* num_modules;          // Number of cores each process
   UInt32  total_cores;          // Total number of cores in all processes

   UInt32  my_proc_num;          // Process number for this process

   // This data structure keeps track of which cores are in each process.
   // It is an array of size num_process where each element is a list of
   // core numbers.  Each list specifies which cores are in the corresponding
   // process.
   CoreList* core_map;
   
   UInt32  MCP_process;          // The process where the MCP lives

   NetworkMeshAnalyticalParameters *analytic_network_parms;
   
 public:
   Config();
   ~Config();

   void loadFromFile(char* filename);
   void loadFromCmdLine();

   // Return the number of processes involved in this simulation
   UInt32 numProcs() { return num_process; }
   void setNumProcs(UInt32 in_num_process) { num_process = in_num_process; }

   // Retrieve and set the process number for this process (I'm expecting
   //  that the initialization routine of the Transport layer will set this)
   UInt32 myProcNum() { return my_proc_num; }
   void setProcNum(UInt32 in_my_proc_num) { my_proc_num = in_my_proc_num; }

   // Return the number of the process that should contain the MCP
   UInt32 MCPProcNum() { return MCP_process; }
   UInt32 MCPCommID() { return totalMods() - 1; }

   // Return the number of modules (cores) in a given process
   UInt32 numMods()
      { assert(my_proc_num < num_process); return num_modules[my_proc_num]; }
   UInt32 numMods(UInt32 proc_num)
      { assert(proc_num < num_process); return num_modules[proc_num]; }

   // Return the total number of modules in all processes
   UInt32 totalMods() { return total_cores; }

   // Return an array of core numbers for a given process
   //  The returned array will have numMods(proc_num) elements
   const CoreList getModuleList()
      { assert(my_proc_num < num_process); return core_map[my_proc_num]; }
   const CoreList getModuleList(UInt32 proc_num)
      { assert(proc_num < num_process); return core_map[proc_num]; }
   
   const NetworkMeshAnalyticalParameters *getAnalyticNetworkParms() const
      { return analytic_network_parms; }

   // Fills in an array with the models for each static network
   void getNetworkModels(UInt32 *) const;
};

#endif
