#ifndef DVFS_H
#define DVFS_H

#include "fixed_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

void CarbonGetTileFrequency(volatile float* frequency);
void CarbonSetTileFrequency(volatile float* frequency);
void CarbonSetRemoteTileFrequency(tile_id_t tile_id, volatile float* frequency);

#ifdef __cplusplus
}
#endif

#endif // DVFS_H
