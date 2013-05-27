#pragma once

#include <list>
#include <map>
using std::list;
using std::pair;
using std::map;

#include "fixed_types.h"
#include "dvfs.h"
#include "network.h"

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

   // Called to initialize DVFS voltage-frequency levels
   static void initializeDVFSLevels();

   // Called to initialize DVFS domains 
   static void initializeDVFSDomainMap();

   // Get DVFS Levels
   static const DVFSLevels& getDVFSLevels();

   // Called from the McPAT interfaces
   static double getMaxFrequency(double voltage);

   // Returns true if the components belong to the same domain
   bool checkDVFSDomain(module_t component_1, module_t component_2);
 
private:
   // Voltage, Frequency Multiplier, Domain Map
   static DVFSLevels _dvfs_levels;
   static volatile double _max_frequency;
   static map<module_t, pair<int, double> > _dvfs_domain_map;
   Tile* _tile;
   
   static double getMinVoltage(double frequency);

};
