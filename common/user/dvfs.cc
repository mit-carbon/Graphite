#include "dvfs.h"
#include "fxsupport.h"
#include "log.h"

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

   //Core* core = Sim()->getTileManager()->getCurrentTile()->getCore();
   //core->coreSendW(core->getId().tile_id, tile_id, (char*) frequency, sizeof(float), (carbon_network_t) CARBON_FREQ_CONTROL);
   return 0;
}
