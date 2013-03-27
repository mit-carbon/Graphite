#ifndef CARBON_USER_H
#define CARBON_USER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>
#include "fixed_types.h"
#include "capi.h"
#include "sync_api.h"
#include "thread_support.h"
#include "performance_counter_support.h"
#include "dynamic_frequency_scaling.h"
#include "dvfs.h"

SInt32 CarbonStartSim(int argc, char **argv);
void CarbonStopSim();
tile_id_t CarbonGetTileId();
UInt64 CarbonGetTime();

#ifdef __cplusplus
}
#endif

#endif // CARBON_USER_H
