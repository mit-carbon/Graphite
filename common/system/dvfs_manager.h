#pragma once

#include <list>
#include <map>
using std::list;
using std::pair;
using std::map;

#include "fixed_types.h"
#include "dvfs.h"
#include "network.h"
#include "mem_component.h"

// Called over the network (callbacks)
void getDVFSCallback(void* obj, NetPacket packet);
void setDVFSCallback(void* obj, NetPacket packet);   

class DVFSManager
{
public:
   // DVFS Levels type
   typedef list<pair<volatile double,volatile double> > DVFSLevels;
   
   // Parse the cfg file to get DVFS voltage levels and frequency of operation
   DVFSManager(UInt32 technology_node, Tile* tile);
   ~DVFSManager();
 
   // Called from common/user/dvfs
   int getDVFS(tile_id_t tile_id, module_t module_type, double* frequency, double* voltage);
   int setDVFS(tile_id_t tile_id, int module_mask, double frequency, voltage_option_t voltage_flag);
 
   // Internal functions called after figuring out voltage/frequency
   void doGetDVFS(module_t module_type, core_id_t requester);
   void doSetDVFS(int module_mask, double frequency, voltage_option_t voltage_flag, const Time& curr_time, core_id_t requester);
   static int getVoltage(volatile double &voltage, voltage_option_t voltage_flag, double frequency);
   static int getInitialFrequencyAndVoltage(module_t component, volatile double &frequency, volatile double &voltage);

   // Called to initialize DVFS
   static void initializeDVFS();

   // Called to initialize DVFS voltage-frequency levels
   static void initializeDVFSLevels();

   // Called to initialize DVFS domains 
   static void initializeDVFSDomainMap();

   // Get DVFS Levels
   static const DVFSLevels& getDVFSLevels();

   // Called from the McPAT interfaces
   static double getMaxFrequency(double voltage);

   // Returns true if the components belong to the same domain
   static bool hasSameDVFSDomain(module_t component_1, module_t component_2);

   // returns synchronization delay
   static UInt32 getSynchronizationDelay();

   // returns synchronization delay
   static module_t convertToModule(MemComponent::Type component);
 
private:
   // Voltage, Frequency Multiplier, Domain Map
   static DVFSLevels _dvfs_levels;
   static volatile double _max_frequency;
   static map<module_t, pair<module_t, double> > _dvfs_domain_map;
   Tile* _tile;
   
   static double getMinVoltage(double frequency);

   static UInt32 _synchronization_delay_cycles;
};
