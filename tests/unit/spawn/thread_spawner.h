#ifndef THREAD_SPAWNER_H
#define THREAD_SPAWNER_H

#include "thread_support.h"
#include "capi.h"

void CarbonGetThreadToSpawn(ThreadSpawnRequest **req);
void CarbonThreadStart(ThreadSpawnRequest *req);
void CarbonThreadExit();
void *CarbonSpawnManagedThread(void *p);
void *CarbonThreadSpawner(void *p);
int CarbonSpawnThreadSpawner();

int CarbonSpawnThread(thread_func_t func, void *arg);
void CarbonJoinThread(int tid);

#endif
