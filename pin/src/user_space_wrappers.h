#ifndef USER_SPACE_WRAPPERS_H
#define USER_SPACE_WRAPPERS_H

#include "fixed_types.h"
#include "capi.h"
#include "sync_api.h"

CAPI_return_t SimGetCoreID(int *core_id);
void SimInitializeThread();
void SimTerminateThread();
void SimInitializeCommId(int comm_id);
int SimGetProcessId();
int SimGetProcessCount();

// sync wrappers
void SimMutexInit(carbon_mutex_t *mux);
void SimMutexLock(carbon_mutex_t *mux);
void SimMutexUnlock(carbon_mutex_t *mux);

void SimCondInit(carbon_cond_t *cond);
void SimCondWait(carbon_cond_t *cond, carbon_mutex_t *mux);
void SimCondSignal(carbon_cond_t *cond);
void SimCondBroadcast(carbon_cond_t *cond);

void SimBarrierInit(carbon_barrier_t *barrier, UInt32 count);
void SimBarrierWait(carbon_barrier_t *barrier);

// Helper functions
void SimSharedMemQuit();
int SimGetCoreCount();

CAPI_return_t SimSendW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size);
CAPI_return_t SimRecvW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size);

#endif
