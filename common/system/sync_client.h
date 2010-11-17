#ifndef SYNC_CLIENT_H
#define SYNC_CLIENT_H

#include "sync_api.h"
#include "packetize.h"

class Core;
class Network;

class SyncClient
{
   public:
      SyncClient(Core*);
      ~SyncClient();

      void mutexInit(carbon_mutex_t *mux);
      void mutexLock(carbon_mutex_t *mux);
      void mutexUnlock(carbon_mutex_t *mux);

      void condInit(carbon_cond_t *cond);
      void condWait(carbon_cond_t *cond, carbon_mutex_t *mux);
      void condSignal(carbon_cond_t *cond);
      void condBroadcast(carbon_cond_t *cond);

      void barrierInit(carbon_barrier_t *barrier, UInt32 count);
      void barrierWait(carbon_barrier_t *barrier);

      /* Unique return codes for each function call
         - Note: It is NOT a mistake that
           > COND_WAIT_RESPONSE == MUTEX_LOCK_RESPONSE

           This is necessary because when several threads wait on a
           condition variable and condBroadcast() is called, they will be
           woken by the mutexUnlock() of the thread that holds the lock.

      */

      static const unsigned int MUTEX_LOCK_RESPONSE   = 0xDEADBEEF;
      static const unsigned int MUTEX_UNLOCK_RESPONSE = 0xBABECAFE;
      static const unsigned int COND_WAIT_RESPONSE    = MUTEX_LOCK_RESPONSE;
      static const unsigned int COND_SIGNAL_RESPONSE  = 0xBEEFCAFE;
      static const unsigned int COND_BROADCAST_RESPONSE = 0xDEADCAFE;
      static const unsigned int BARRIER_WAIT_RESPONSE  = 0xCACACAFE;

   private:
      Core *m_core;
      Network *m_network;
      UnstructuredBuffer m_send_buff;
      UnstructuredBuffer m_recv_buff;

};

#endif
