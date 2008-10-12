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
   
 public:
   Config();
   ~Config();

   void loadFromFile(char* filename);
   void loadFromCmdLine();

   // Return the number of processes involved in this simulation
   UInt32 numProcs() { return num_process; }

   // Return the number of modules (cores) in a given process
   UInt32 numMods() { assert(my_proc_num < num_process); return num_modules[my_proc_num]; }
   UInt32 numMods(UInt32 proc_num) { assert(proc_num < num_process); return num_modules[proc_num]; }

   // Return the total number of modules in all processes
   UInt32 totalMods() { return total_cores; }

   // Return an array of core numbers for a given process
   //  The returned array will have numMods(proc_num) elements
   const CoreList getModuleList() { assert(my_proc_num < num_process); return core_map[my_proc_num]; }
   const CoreList getModuleList(UInt32 proc_num) { assert(proc_num < num_process); return core_map[proc_num]; }
   
};

#endif
