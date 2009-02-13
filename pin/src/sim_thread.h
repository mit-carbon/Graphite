#ifndef SIM_THREAD_H
#define SIM_THREAD_H

#include "fixed_types.h"
#include "network.h"

class Lock;
class SimThreadRunner;

SimThreadRunner *SimThreadStart();
void SimThreadQuit();
void SimThreadTerminateFunc(void *vp, NetPacket pkt);
void* SimThreadFunc(void *);

#endif // SIM_THREAD_H
