#ifndef SYNC_CLIENT_H
#define SYNC_CLIENT_H

#include "sync_api.h"

void SimMutexInit(carbon_mutex_t *mux);
void SimMutexLock(carbon_mutex_t *mux);
void SimMutexUnlock(carbon_mutex_t *mux);


#endif
