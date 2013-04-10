#ifndef DVFS_H
#define DVFS_H

#include "fixed_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
   CORE = 0x1,
   L1_ICACHE = 0x2,
   L1_DCACHE = 0x4,
   L2_CACHE = 0x8,
   L2_DIRECTORY = 0x10,
   TILE = 0x1f,
   MAX_MODULE_TYPES
} module_t;
 
typedef enum
{
   AUTO,
   HOLD,
   MAX_VOLTAGE_OPTIONS
} voltage_option_t;
 
// Get DVFS
int CarbonGetDVFS(tile_id_t tile_id, module_t module_type, double* frequency, double* voltage);
int CarbonGetFrequency(tile_id_t tile_id, module_t module_type, double* frequency);
int CarbonGetVoltage(tile_id_t tile_id, module_t module_type, double* voltage);
// Set DVFS
int CarbonSetDVFS(tile_id_t tile_id, int module_mask, double* frequency, voltage_option_t voltage_flag);
 
#ifdef __cplusplus
}
#endif

#endif // DVFS_H
