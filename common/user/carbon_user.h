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
#include "dvfs.h"

// Start the simulation (allocate the simulation models and create the simulation threads)
SInt32 CarbonStartSim(int argc, char **argv);
// Stop the simulation (join the simulation threads and release the simulation models)
void CarbonStopSim();
// Gets the tile-ID on which the thread is running
tile_id_t CarbonGetTileId();
// Get the current time (in nanoseconds)
UInt64 CarbonGetTime();

#ifdef __cplusplus
}
#endif

#endif // CARBON_USER_H
