#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "fixed_types.h"
#include "network.h"

class Lock;
class NetThreadRunner;

NetThreadRunner *SimSharedMemStartThreads();
void SimSharedMemQuit();
void SimSharedMemTerminateFunc(void *vp, NetPacket pkt);
void* SimSharedMemThreadFunc(void *);

#endif // SHARED_MEM_H
