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

void condInit(carbon_cond_t *cond)
{
   cerr << "Made it to the dummy condInit() function." << endl;
   assert(false);
}

void condWait(carbon_cond_t *cond, carbon_mutex_t *mux)
{
   cerr << "Made it to the dummy condWait() function." << endl;
   assert(false);
}

void condSignal(carbon_cond_t *cond)
{
   cerr << "Made it to the dummy condSignal() function." << endl;
   assert(false);
}

void condBroadcast(carbon_cond_t *cond)
{
   cerr << "Made it to the dummy condBroadcast() function." << endl;
   assert(false);
}


bool isMutexValid(carbon_mutex_t *mux)
{
    return(!(*mux == -1));
}

bool isCondValid(carbon_cond_t *cond)
{
    return(!(*cond == -1));
}

