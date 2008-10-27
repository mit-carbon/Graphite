#include "sync_api.h"
#include <iostream>
#include <cassert>

using namespace std;

void mutexInit(carbon_mutex_t *mux)
{
   cerr << "Made it to the dummy mutexInit() function." << endl;
   assert(false);
}

void mutexLock(carbon_mutex_t *mux)
{
   cerr << "Made it to the dummy mutexLock() function." << endl;
   assert(false);
}

void mutexUnlock(carbon_mutex_t *mux)
{
   cerr << "Made it to the dummy mutexUnlock() function." << endl;
   assert(false);
}


bool isMutexValid(carbon_mutex_t *mux)
{
    return(!(*mux == -1));
}
