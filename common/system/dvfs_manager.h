#pragma once

#include <list>
using std::list;
using std::pair;

#include "fixed_types.h"
#include "dvfs.h"
#include "network.h"

// Called over the network (callbacks)
void getDVFSCallback(void* obj, NetPacket packet);
void setDVFSCallback(void* obj, NetPacket packet);   

class DVFSManager
{
public:
   // Parse the cfg file to get DVFS voltage levels and frequency of operation
   DVFSManager(UInt32 technology_node, Tile* tile);
   ~DVFSManager();
 
   // Called from common/user/dvfs
   int getDVFS(tile_id_t tile_id, module_t module_type, double* frequency, double* voltage);
   int setDVFS(tile_id_t tile_id, int module_mask, double frequency, voltage_option_t voltage_flag);
 
   // Internal functions called after figuring out voltage/frequency
   int doGetDVFS(module_t module_type, core_id_t requester);
   int doSetDVFS(int module_mask, double frequency, voltage_option_t voltage_flag, core_id_t requester);

   // Called to initialize DVFS voltage-frequency levels
   static void initializeDVFSLevels();

   // Called from the McPAT interfaces
   static double getNominalVoltage();
   static double getMaxFrequencyFactorAtVoltage(double voltage);
 
private:
   // Voltage, Frequency Multiplier
   typedef list<pair<volatile double,volatile double> > DVFSLevels;
   static DVFSLevels _dvfs_levels;
   Tile* _tile;
};
