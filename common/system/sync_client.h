#ifndef SYNC_CLIENT_H
#define SYNC_CLIENT_H

#include "sync_api.h"
#include "packetize.h"

class Network;

class SyncClient
{
  Network *_network;
  UnstructuredBuffer _send_buff, _recv_buff;

 public:

  SyncClient(Network*);
  ~SyncClient();

  void mutexInit(int commid, carbon_mutex_t *mux);
  void mutexLock(int commid, carbon_mutex_t *mux);
  void mutexUnlock(int commid, carbon_mutex_t *mux);

  void condInit(int commid, carbon_cond_t *cond);
  void condWait(int commid, carbon_cond_t *cond, carbon_mutex_t *mux);
  void condSignal(int commid, carbon_cond_t *cond);
  void condBroadcast(int commid, carbon_cond_t *cond);

  static const unsigned int MUTEX_LOCK_RESPONSE   = 0xDEADBEEF;
  static const unsigned int MUTEX_UNLOCK_RESPONSE = 0xBABECAFE;
  static const unsigned int COND_WAIT_RESPONSE    = 0xBABEBEEF;
  static const unsigned int COND_SIGNAL_RESPONSE  = 0xBEEFCAFE;
  static const unsigned int COND_BROADCAST_RESPONSE = 0xDEADCAFE;

};

#endif
