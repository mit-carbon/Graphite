#pragma once

#include <list>
using std::list;
using std::pair;

#include "fixed_types.h"
#include "dvfs.h"

class DVFSManager
{
public:
   // Parse the cfg file to get DVFS voltage levels and frequency of operation
   DVFSManager(UInt32 technology_node);
   ~DVFSManager();
 
   // Called from common/user/dvfs
   int getDVFS(tile_id_t tile_id, module_t module_type, double* frequency, double* voltage);
   int setDVFS(tile_id_t tile_id, int module_mask, double frequency, dvfs_option_t frequency_flag, dvfs_option_t voltage_flag);
 
   // Called over the network (callbacks)
   void getDVFSCallback(tile_id_t requester, module_t module_type);
   void setDVFSCallback(tile_id_t requester, int module_mask, double frequency, double voltage);   
 
private:
   // Voltage, Frequency Multiplier
   typedef list<pair<double,double> > DVFSLevels;
   DVFSLevels _dvfs_levels;
 
   // Internal function called after figuring out voltage/frequency
   int setDVFS(tile_id_t tile_id, int module_mask, double frequency, double voltage);
};
