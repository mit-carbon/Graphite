#ifndef USER_API_H
#define USER_API_H

#include "capi.h"
#include "sync_api.h"

void carbonInit();
void carbonFinish();

void sharedMemThreadsInit();
void sharedMemThreadsFinish();

int getCoreCount();
void sharedMemQuit();
void* shmem_thread_func(void *);

#endif
