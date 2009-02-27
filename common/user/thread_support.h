// THREAD_SUPPORT
//
// This file provides structures needed for the simulator
// to pass thread creation functionality on to the user
// land application.
#ifndef THREAD_SUPPORT_H
#define THREAD_SUPPORT_H

void CarbonSpawnWorkerThreads();

typedef void *(*thread_func_t)(void *);
typedef struct 
{
   thread_func_t func;
   void *arg;
   int core_id;
} carbon_thread_info_t;

#endif

