#ifndef SYNC_API_H
#define SYNC_API_H

//FIXME: Make this a more proper type
typedef int carbon_mutex_t;
typedef int carbon_cond_t;
typedef int carbon_barrier_t;

// These are the dummy functions that will get replaced
// by the simulator
// Related to Mutexes
extern "C" {
   void mutexInit(carbon_mutex_t *mux);
   void mutexLock(carbon_mutex_t *mux);
   void mutexUnlock(carbon_mutex_t *mux);
}
bool isMutexValid(carbon_mutex_t *mux);

// Related to condition variables
extern "C" {
    void condInit(carbon_cond_t *cond);
    void condWait(carbon_cond_t *cond, carbon_mutex_t *mux);
    void condSignal(carbon_cond_t *cond);
    void condBroadcast(carbon_cond_t *cond);
}
bool isCondValid(carbon_cond_t *cond);

// Related to barriers
extern "C" {
    void barrierInit(carbon_barrier_t *barrier, unsigned int count);
    void barrierWait(carbon_barrier_t *barrier);
}
bool isBarrierValid(carbon_barrier_t *barrier);

#endif
