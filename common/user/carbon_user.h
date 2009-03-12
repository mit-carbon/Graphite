#ifndef CARBON_USER_H
#define CARBON_USER_H

#include "fixed_types.h"
#include "capi.h"
#include "sync_api.h"
#include "thread_support.h"

SInt32 CarbonStartSim();
void CarbonStopSim();
core_id_t CarbonGetCoreId();

#endif // CARBON_USER_H
