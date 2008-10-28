#ifndef SYNC_SERVER_H
#define SYNC_SERVER_H

#include <queue>
#include <vector>

#include "sync_api.h"
#include "transport.h"
#include "packetize.h"
#include "stable_iterator.h"

// FIXME: we need to put this somewhere that makes sense
typedef int comm_id_t;


class SimMutex
{
  typedef std::queue<comm_id_t> ThreadQueue;

  ThreadQueue _waiting;
  comm_id_t _owner;

 public:
  static const int NO_OWNER = -1;

  SimMutex();
  ~SimMutex();

  // returns true if this thread now owns the lock
  bool lock(comm_id_t commid);

  // returns the next owner of the lock so that it can be signaled by
  // the server
  comm_id_t unlock(comm_id_t commid);
};

class SimCond
{
  typedef std::pair<comm_id_t, StableIterator<SimMutex> > WaitPair;
  typedef std::queue< WaitPair > ThreadQueue;

  ThreadQueue _waiting;

 public:
  typedef std::vector<comm_id_t> WakeupList;

  SimCond();
  ~SimCond();

  // returns the thread that gets woken up when the mux is unlocked
  comm_id_t wait(comm_id_t commid, StableIterator<SimMutex> & it);
  comm_id_t signal(comm_id_t commid);
  void broadcast(comm_id_t commid, WakeupList &woken);
};

class SyncServer
{
  typedef std::vector<SimMutex> MutexVector;
  typedef std::vector<SimCond> CondVector;

  MutexVector _mutexes;
  CondVector _conds;

  // FIXME: This should be better organized -- too much redundant crap
 private:
  Transport & _pt_endpt;
  UnstructuredBuffer &_recv_buffer;
  UnstructuredBuffer _send_buffer;
  UnstructuredBuffer _send_buffer2;

 public:
  SyncServer(Transport &pt_endpt, UnstructuredBuffer &recv_buffer);
  ~SyncServer();

  // Remaining parameters to these functions are stored
  // in the recv buffer and get unpacked
  void mutexInit(comm_id_t);
  void mutexLock(comm_id_t);
  void mutexUnlock(comm_id_t);

  void condInit(comm_id_t);
  void condWait(comm_id_t);
  void condSignal(comm_id_t);
  void condBroadcast(comm_id_t);
};

#endif // SYNC_SERVER_H
