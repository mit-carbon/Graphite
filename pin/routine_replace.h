#ifndef ROUTINE_REPLACE_H
#define ROUTINE_REPLACE_H

#include <string>
#include "pin.H"

bool replaceUserAPIFunction(RTN& rtn, string& name);

void setupCarbonSpawnThreadSpawnerStack (CONTEXT *ctx);
void setupCarbonThreadSpawnerStack (CONTEXT *ctx);

// Thread spawning and management
void replacementMain (CONTEXT *ctxt);
void replacementGetThreadToSpawn (CONTEXT *ctxt);
void replacementThreadStartNull (CONTEXT *ctxt);
void replacementThreadExitNull (CONTEXT *ctxt);
void replacementGetTileId (CONTEXT *ctxt);
void replacementDequeueThreadSpawnRequest (CONTEXT *ctxt);

// Pin specific stack management
void replacementPthreadAttrInitOtherAttr (CONTEXT *ctxt);

// Carbon API
void replacementStartSimNull (CONTEXT *ctxt);
void replacementStopSim (CONTEXT *ctxt);
void replacementSpawnThread (CONTEXT *ctxt);
void replacementSpawnThreadOnTile (CONTEXT *ctxt);
void replacementSchedSetAffinity (CONTEXT *ctxt);
void replacementSchedGetAffinity (CONTEXT *ctxt);
void replacementJoinThread (CONTEXT *ctxt);

void replacementMutexInit(CONTEXT *ctxt);
void replacementMutexLock(CONTEXT *ctxt);
void replacementMutexUnlock(CONTEXT *ctxt);

void replacementCondInit(CONTEXT *ctxt);
void replacementCondWait(CONTEXT *ctxt);
void replacementCondSignal(CONTEXT *ctxt);
void replacementCondBroadcast(CONTEXT *ctxt);

void replacementBarrierInit(CONTEXT *ctxt);
void replacementBarrierWait(CONTEXT *ctxt);

// CAPI
void replacement_CAPI_Initialize (CONTEXT *ctxt);
void replacement_CAPI_rank (CONTEXT *ctxt);
void replacement_CAPI_message_send_w (CONTEXT *ctxt);
void replacement_CAPI_message_receive_w (CONTEXT *ctxt);
void replacement_CAPI_message_send_w_ex (CONTEXT *ctxt);
void replacement_CAPI_message_receive_w_ex (CONTEXT *ctxt);

// pthread
void replacementPthreadCreate(CONTEXT *ctxt);
void replacementPthreadJoin(CONTEXT *ctxt);
void replacementPthreadExitNull(CONTEXT *ctxt);

// Enable/Disable Models
void replacementEnableModels(CONTEXT* ctxt);
void replacementDisableModels(CONTEXT* ctxt);

// Cache Counters
void replacementResetCacheCounters(CONTEXT *ctxt);
void replacementDisableCacheCounters(CONTEXT *ctxt);

// Getting Simulated Time
void replacementCarbonGetTime(CONTEXT *ctxt);

// DVFS
void replacementCarbonGetDVFSDomain(CONTEXT *ctxt);
void replacementCarbonGetDVFS(CONTEXT *ctxt);
void replacementCarbonGetFrequency(CONTEXT *ctxt);
void replacementCarbonGetVoltage(CONTEXT *ctxt);
void replacementCarbonSetDVFS(CONTEXT *ctxt);
void replacementCarbonSetDVFSAllTiles(CONTEXT *ctxt);
void replacementCarbonGetTileEnergy(CONTEXT *ctxt);

void initialize_replacement_args (CONTEXT *ctxt, ...);
void retFromReplacedRtn (CONTEXT *ctxt, ADDRINT ret_val);
#endif
