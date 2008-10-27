#include "sync_api.h"
#include <iostream>
#include <cassert>

using namespace std;

bool mutexInit(carbon_mutex_t *mux)
{
   cerr << "Made it to the dummy mutexInit() function." << endl;
   assert(false);
}

bool mutexLock(carbon_mutex_t *mux)
{
   cerr << "Made it to the dummy mutexLock() function." << endl;
   assert(false);
}

bool mutexUnlock(carbon_mutex_t *mux)
{
   cerr << "Made it to the dummy mutexUnlock() function." << endl;
   assert(false);
}


