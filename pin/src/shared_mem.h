#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "fixed_types.h"
#include "network.h"

class Lock;
class NetThreadRunner;

extern UInt32 g_shared_mem_active_threads;
extern Lock* g_shared_mem_threads_lock;

NetThreadRunner *SimSharedMemStartThreads();
void SimSharedMemQuit();
void SimSharedMemTerminateFunc(void *vp, NetPacket pkt);
void* SimSharedMemThreadFunc(void *);

#endif // SHARED_MEM_H
