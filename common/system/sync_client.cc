#include "sync_client.h"
#include <iostream>

using namespace std;

void SimMutexInit(carbon_mutex_t *mux)
{
   cerr << "Initializing mutex..." << mux << endl;
}

void SimMutexLock(carbon_mutex_t *mux)
{
   cerr << "Locking mutex..." << mux << endl;
}

void SimMutexUnlock(carbon_mutex_t *mux)
{
   cerr << "Unlocking mutex..." << mux << endl;
}

