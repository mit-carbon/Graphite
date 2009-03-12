#ifndef CARBON_USER_H
#define CARBON_USER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "fixed_types.h"
#include "capi.h"
#include "sync_api.h"
#include "thread_support.h"

SInt32 CarbonStartSim();
void CarbonStopSim();
core_id_t CarbonGetCoreId();

#ifdef __cplusplus
}
#endif

#endif // CARBON_USER_H
