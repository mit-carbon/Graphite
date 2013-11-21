#ifndef DVFS_H
#define DVFS_H

#include "fixed_types.h"

#ifdef __cplusplus
extern "C"
{
#endif
/*
Return codes for all DVFS API functions:
0  : success
-1 : invalid tile id error
-2 : invalid module type or domain error
-3 : invalid voltage option error
-4 : invalid frequency error
-5 : above max frequency for current voltage error
*/

typedef enum
{
   INVALID_MODULE = 0,
   CORE = 0x1,
   L1_ICACHE = 0x2,
   L1_DCACHE = 0x4,
   L2_CACHE = 0x8,
   DIRECTORY = 0x10,
   TILE = 0x1f,
   NETWORK_USER = 0x20,
   NETWORK_MEMORY = 0x40,
   MAX_MODULE_TYPES = 0x7f
} module_t;
 
typedef enum
{
   AUTO,
   HOLD
} voltage_option_t;
 
// Get DVFS
module_t CarbonGetDVFSDomain(module_t module_type);
int CarbonGetDVFS(tile_id_t tile_id, module_t module_type, double* frequency, double* voltage);
int CarbonGetFrequency(tile_id_t tile_id, module_t module_type, double* frequency);
int CarbonGetVoltage(tile_id_t tile_id, module_t module_type, double* voltage);
// Set DVFS
int CarbonSetDVFS(tile_id_t tile_id, int module_mask, double* frequency, voltage_option_t voltage_flag);
int CarbonSetDVFSAllTiles(int module_mask, double* frequency, voltage_option_t voltage_flag);
int CarbonGetTileEnergy(tile_id_t tile_id, double* energy);
 
#ifdef __cplusplus
}
#endif

#endif // DVFS_H
