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
};

#endif
