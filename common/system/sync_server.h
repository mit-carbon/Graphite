#ifndef SYNC_SERVER_H
#define SYNC_SERVER_H

#include <queue>
#include <vector>

#include "sync_api.h"
#include "transport.h"
#include "network.h"
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
  class CondWaiter
  {
    public:
      CondWaiter(comm_id_t comm_id, StableIterator<SimMutex> mutex, UInt64 time)
          : _comm_id(comm_id), _mutex(mutex), _arrival_time(time) {}
      comm_id_t _comm_id;
      StableIterator<SimMutex> _mutex;
      UInt64 _arrival_time;
  };

  typedef std::vector< CondWaiter > ThreadQueue;
  typedef std::vector< UInt64 > SignalQueue;

  ThreadQueue _waiting;
  SignalQueue _signals;

 public:
  typedef std::vector<comm_id_t> WakeupList;

  SimCond();
  ~SimCond();

  // returns the thread that gets woken up when the mux is unlocked
  comm_id_t wait(comm_id_t commid, UInt64 time, StableIterator<SimMutex> & it);
  comm_id_t signal(comm_id_t commid, UInt64 time);
  void broadcast(comm_id_t commid, UInt64 time, WakeupList &woken);
};

class SimBarrier
{
  typedef std::vector< comm_id_t > ThreadQueue;

  ThreadQueue _waiting;
  UInt32 _count;
  UInt64 _max_time;

 public:
  typedef std::vector<comm_id_t> WakeupList;

  SimBarrier(UInt32 count);
  ~SimBarrier();

  // returns a list of threads to wake up if all have reached barrier
  void wait(comm_id_t commid, UInt64 time, WakeupList &woken);
  UInt64 getMaxTime() { return _max_time; }

};

class SyncServer
{
  typedef std::vector<SimMutex> MutexVector;
  typedef std::vector<SimCond> CondVector;
  typedef std::vector<SimBarrier> BarrierVector;

  MutexVector _mutexes;
  CondVector _conds;
  BarrierVector _barriers;

  // FIXME: This should be better organized -- too much redundant crap
 private:
  Network & _network;
  UnstructuredBuffer &_recv_buffer;

 public:
  SyncServer(Network &network, UnstructuredBuffer &recv_buffer);
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

  void barrierInit(comm_id_t);
  void barrierWait(comm_id_t);
};

#endif // SYNC_SERVER_H
