#ifndef THREAD_SPAWNER_H
#define THREAD_SPAWNER_H

#include "user/thread_support.h"
#include "user/capi.h"

void CarbonGetThreadToSpawn(ThreadSpawnRequest **req);
void CarbonThreadStart(ThreadSpawnRequest *req);
void CarbonThreadExit();
void *CarbonSpawnManagedThread(void *p);
void *CarbonThreadSpawner(void *p);
int CarbonSpawnThreadSpawner();

int CarbonSpawnThread(thread_func_t func, void *arg);
void CarbonJoinThread(int tid);

int CarbonStartSim();
void CarbonStopSim();

#endif
