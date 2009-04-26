#ifndef ROUTINE_REPLACE_H
#define ROUTINE_REPLACE_H

#include <string>
#include "pin.H"

bool replaceUserAPIFunction(RTN& rtn, string& name);

void setupCarbonSpawnThreadSpawnerStack (CONTEXT *ctx);

void replacementNullRtn (CONTEXT *ctxt);

void replacementGetThreadToSpawn (CONTEXT *ctxt);
void replacementThreadStart (CONTEXT *ctxt);
void replacementThreadExit (CONTEXT *ctxt);

void replacementSpawnThread (CONTEXT *ctxt);
void replacementJoinThread (CONTEXT *ctxt);
void replacementGetProcessCount (CONTEXT *ctxt);
void replacementGetProcessId (CONTEXT *ctxt);
void replacementInitializeCommId (CONTEXT *ctxt);

void replacementMutexInit(CONTEXT *ctxt);
void replacementMutexLock(CONTEXT *ctxt);
void replacementMutexUnlock(CONTEXT *ctxt);

void replacementCondInit(CONTEXT *ctxt);
void replacementCondWait(CONTEXT *ctxt);
void replacementCondSignal(CONTEXT *ctxt);
void replacementCondBroadcast(CONTEXT *ctxt);

void replacementBarrierInit(CONTEXT *ctxt);
void replacementBarrierWait(CONTEXT *ctxt);

void replacementGetCoreID (CONTEXT *ctxt);
void replacementSendW (CONTEXT *ctxt);
void replacementRecvW (CONTEXT *ctxt);

void initialize_replacement_args (CONTEXT *ctxt, ...);
void retFromReplacedRtn (CONTEXT *ctxt, ADDRINT ret_val);

#endif
