#ifndef CARBON_USER_H
#define CARBON_USER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>
#include "fixed_types.h"
#include "capi.h"
#include "sync_api.h"
#include "thread_support.h"

SInt32 CarbonStartSim(int argc, char **argv);
void CarbonStopSim();
core_id_t CarbonGetCoreId();
int create_pthread(pthread_t * thread, pthread_attr_t * attr, void * (*start_routine)(void *), void * arg);

#ifdef __cplusplus
}
#endif

#endif // CARBON_USER_H
