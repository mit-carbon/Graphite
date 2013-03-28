#include "dvfs.h"
#include "fxsupport.h"
#include "log.h"
#include "core.h"
#include "tile_manager.h"
#include "tile.h"
#include "simulator.h"

// Get DVFS
int CarbonGetDVFS(tile_id_t tile_id, module_t module_type, double* frequency, double* voltage)
{
   return 0;
}

int CarbonGetFrequency(tile_id_t tile_id, module_t module_type, double* frequency)
{
   return 0;
}

int CarbonGetVoltage(tile_id_t tile_id, module_t module_type, double* voltage)
{
   return 0;
}

// Set DVFS
int CarbonSetDVFS(tile_id_t tile_id, int module_mask, volatile double* frequency, dvfs_option_t frequency_flag, dvfs_option_t voltage_flag)
{

   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;
   
   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   tile->getDVFSManager()->setDVFS(tile_id, module_mask, *frequency, frequency_flag, voltage_flag);

   return 0;
}
