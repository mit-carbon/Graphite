// config.h
//
// The Config class is used to set, store, and retrieve all the configurable
// parameters of the simulator.
//
// Initial creation: Sep 18, 2008 by jasonm

#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <set>
#include <map>
#include <string>
#include <iostream>
#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include "fixed_types.h"

class Config
{
public:
   enum SimulationMode
   {
      FULL = 0,
      LITE,
      NUM_SIMULATION_MODES
   };

   class CoreParameters
   {
      private:
         std::string m_type;
         volatile float m_frequency;
         std::string m_l1_icache_type;
         std::string m_l1_dcache_type;
         std::string m_l2_cache_type;

      public:
         CoreParameters(std::string type, volatile float frequency, std::string l1_icache_type, std::string l1_dcache_type, std::string l2_cache_type):
            m_type(type),
            m_frequency(frequency),
            m_l1_icache_type(l1_icache_type),
            m_l1_dcache_type(l1_dcache_type),
            m_l2_cache_type(l2_cache_type)
         {}
         ~CoreParameters() {}

         volatile float getFrequency() { return m_frequency; }
         void setFrequency(volatile float frequency) { m_frequency = frequency; }
         std::string getType() { return m_type; }
         std::string getL1ICacheType() { return m_l1_icache_type; }
         std::string getL1DCacheType() { return m_l1_dcache_type; }
         std::string getL2CacheType() { return m_l2_cache_type; }
   };

   class NetworkParameters
   {
      private:
         std::string m_type;
         volatile float m_frequency;

      public:
         NetworkParameters(std::string type, volatile float frequency):
            m_type(type), m_frequency(frequency)
         {}
         ~NetworkParameters() {}

         volatile float getFrequency() { return m_frequency; }
         std::string getType() { return m_type; }
   };
   
   typedef std::vector<UInt32> CoreToProcMap;
   typedef std::vector<core_id_t> CoreList;
   typedef std::vector<core_id_t>::const_iterator CLCI;
   typedef std::map<UInt32,core_id_t> CommToCoreMap;

   Config();
   ~Config();

   void loadFromFile(char* filename);
   void loadFromCmdLine();

   // Return the number of processes involved in this simulation
   UInt32 getProcessCount() { return m_num_processes; }
   void setProcessCount(UInt32 in_num_processes) { m_num_processes = in_num_processes; }

   // Retrieve and set the process number for this process (I'm expecting
   //  that the initialization routine of the Transport layer will set this)
   UInt32 getCurrentProcessNum() { return m_current_process_num; }
   void setProcessNum(UInt32 in_my_proc_num) { m_current_process_num = in_my_proc_num; }

   core_id_t getMCPCoreNum() { return getTotalCores() -1; }

   core_id_t getMainThreadCoreNum() { return 0; }

   core_id_t getThreadSpawnerCoreNum(UInt32 proc_num);
   core_id_t getCurrentThreadSpawnerCoreNum(); 

   // Return the number of modules (cores) in a given process
   UInt32 getNumCoresInProcess(UInt32 proc_num)
   { assert (proc_num < m_num_processes); return m_proc_to_core_list_map[proc_num].size(); }

   SInt32 getIndexFromCoreID(UInt32 proc_num, core_id_t core_id);
   core_id_t getCoreIDFromIndex(UInt32 proc_num, SInt32 index);
   
   UInt32 getNumLocalCores() { return getNumCoresInProcess(getCurrentProcessNum()); }

   // Return the total number of modules in all processes
   UInt32 getTotalCores();
   UInt32 getApplicationCores();

   // Return an array of tile numbers for a given process
   //  The returned array will have numMods(proc_num) elements
   const CoreList & getCoreListForProcess(UInt32 proc_num)
   { assert(proc_num < m_num_processes); return m_proc_to_core_list_map[proc_num]; }

   const CoreList & getCoreListForCurrentProcess()
   { return getCoreListForProcess(getCurrentProcessNum()); }

   UInt32 getProcessNumForCore(UInt32 tile)
   { 
     if (tile >= m_total_cores)
     {
       fprintf(stderr, "tile(%u), m_total_cores(%u)\n", tile, m_total_cores);
       exit(-1);
     }
     return m_core_to_proc_map[tile]; 
   }

   // For mapping between user-land communication id's to actual tile id's
   void updateCommToCoreMap(UInt32 comm_id, core_id_t core_id);
   UInt32 getCoreFromCommId(UInt32 comm_id);

   // Get CoreId length
   UInt32 getCoreIDLength()
   { return m_core_id_length; }

   SimulationMode getSimulationMode()
   { return m_simulation_mode; }

   // Tile & Network Parameters
   std::string getCoreType(core_id_t core_id);
   std::string getL1ICacheType(core_id_t core_id);
   std::string getL1DCacheType(core_id_t core_id);
   std::string getL2CacheType(core_id_t core_id);
   volatile float getCoreFrequency(core_id_t core_id);
   void setCoreFrequency(core_id_t core_id, volatile float frequency);

   std::string getNetworkType(SInt32 network_id);

   // Knobs
   bool isSimulatingSharedMemory() const;
   bool getEnablePerformanceModeling() const;
   bool getEnableDCacheModeling() const;
   bool getEnableICacheModeling() const;
   bool getEnablePowerModeling() const;

   // Logging
   std::string getOutputFileName() const;
   std::string formatOutputFileName(std::string filename) const;
   void logCoreMap();

   static Config *getSingleton();

private:
   void GenerateCoreMap();
   
   UInt32  m_num_processes;         // Total number of processes (incl myself)
   UInt32  m_total_cores;           // Total number of cores in all processes
   UInt32  m_application_cores;     // Total number of cores used by the application
   UInt32  m_core_id_length;        // Number of bytes needed to store a core_id

   UInt32  m_current_process_num;          // Process number for this process

   std::vector<CoreParameters> m_core_parameters_vec;         // Vector holding tile parameters
   std::vector<NetworkParameters> m_network_parameters_vec;   // Vector holding network parameters

   // This data structure keeps track of which cores are in each process.
   // It is an array of size num_processes where each element is a list of
   // tile numbers.  Each list specifies which cores are in the corresponding
   // process.
   CoreToProcMap m_core_to_proc_map;
   CoreList* m_proc_to_core_list_map;

   CommToCoreMap m_comm_to_core_map;

   // Simulation Mode
   SimulationMode m_simulation_mode;

   UInt32  m_mcp_process;          // The process where the MCP lives

   static Config *m_singleton;

   static UInt32 m_knob_total_cores;
   static UInt32 m_knob_num_process;
   static bool m_knob_simarch_has_shared_mem;
   static std::string m_knob_output_file;
   static bool m_knob_enable_performance_modeling;
   static bool m_knob_enable_dcache_modeling;
   static bool m_knob_enable_icache_modeling;
   static bool m_knob_enable_power_modeling;

   // Get Tile & Network Parameters
   void parseCoreParameters();
   void parseNetworkParameters();

   static SimulationMode parseSimulationMode(std::string mode);
   static UInt32 computeCoreIDLength(UInt32 core_count);
   static UInt32 getNearestAcceptableCoreCount(UInt32 core_count);
};

#endif
