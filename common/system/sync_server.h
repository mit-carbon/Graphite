#ifndef SYNC_SERVER_H
#define SYNC_SERVER_H

#include <queue>
#include <vector>

#include "sync_api.h"
#include "transport.h"
#include "packetize.h"

// FIXME: we need to put this somewhere that makes sense
typedef int comm_id_t;

class SimMutex
{
  typedef std::queue<comm_id_t> ThreadQueue;

  ThreadQueue _waiting;
  comm_id_t _owner;

 public:

  static const comm_id_t NO_OWNER = -1;

  SimMutex();
  ~SimMutex();

  // returns true if this thread now owns the lock
  bool lock(comm_id_t commid);

  // returns the next owner of the lock so that it can be signaled by
  // the server
  comm_id_t unlock(comm_id_t commid);
};

class SyncServer
{
  typedef std::vector<SimMutex> MutexVector;

  MutexVector _mutexes;

  // FIXME: This should be better organized -- too much redundant crap
 private:
  Transport & _pt_endpt;
  UnstructuredBuffer &_recv_buffer;
  UnstructuredBuffer _send_buffer;

 public:
  SyncServer(Transport &pt_endpt, UnstructuredBuffer &recv_buffer);
  ~SyncServer();

  void mutexInit(comm_id_t);
  void mutexLock(comm_id_t);
  void mutexUnlock(comm_id_t);
};

#endif // SYNC_SERVER_H
