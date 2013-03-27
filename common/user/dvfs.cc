#include "dvfs.h"

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
int CarbonSetDVFS(tile_id_t tile_id, int module_mask, double frequency, dvfs_option_t frequency_flag, dvfs_option_t voltage_flag)
{
   return 0;
}
