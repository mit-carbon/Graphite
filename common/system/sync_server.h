#ifndef SYNC_SERVER_H
#define SYNC_SERVER_H

#include <queue>
#include <vector>
#include <limits.h>
#include <string.h>

#include "sync_api.h"
#include "transport.h"
#include "network.h"
#include "packetize.h"
#include "stable_iterator.h"

class SimMutex
{
   public:
      //static const core_id_t NO_OWNER = (core_id_t) {UINT_MAX, UINT_MAX};

      SimMutex();
      ~SimMutex();

      // returns true if this thread now owns the lock
      bool lock(core_id_t core_id);

      // returns the next owner of the lock so that it can be signaled by
      // the server
      core_id_t unlock(core_id_t core_id);

   private:
      typedef std::queue<core_id_t> ThreadQueue;

      ThreadQueue m_waiting;
      core_id_t m_owner;
};

class SimCond
{

   public:
      typedef std::vector<core_id_t> WakeupList;

      SimCond();
      ~SimCond();

      // returns the thread that gets woken up when the mux is unlocked
      core_id_t wait(core_id_t core_id, UInt64 time, StableIterator<SimMutex> & it);
      core_id_t signal(core_id_t core_id, UInt64 time);
      void broadcast(core_id_t core_id, UInt64 time, WakeupList &woken);

   private:
      class CondWaiter
      {
         public:
            CondWaiter(core_id_t core_id, StableIterator<SimMutex> mutex, UInt64 time)
                  : m_core_id(core_id), m_mutex(mutex), m_arrival_time(time) {}
            core_id_t m_core_id;
            StableIterator<SimMutex> m_mutex;
            UInt64 m_arrival_time;
      };

      typedef std::vector< CondWaiter > ThreadQueue;
      ThreadQueue m_waiting;
};

class SimBarrier
{
   public:
      typedef std::vector<core_id_t> WakeupList;

      SimBarrier(UInt32 count);
      ~SimBarrier();

      // returns a list of threads to wake up if all have reached barrier
      void wait(core_id_t core_id, UInt64 time, WakeupList &woken);
      UInt64 getMaxTime() { return m_max_time; }

   private:
      typedef std::vector< core_id_t > ThreadQueue;
      ThreadQueue m_waiting;

      UInt32 m_count;
      UInt64 m_max_time;
};

class SyncServer
{
      typedef std::vector<SimMutex> MutexVector;
      typedef std::vector<SimCond> CondVector;
      typedef std::vector<SimBarrier> BarrierVector;

      MutexVector m_mutexes;
      CondVector m_conds;
      BarrierVector m_barriers;

      // FIXME: This should be better organized -- too much redundant crap

   public:
      SyncServer(Network &network, UnstructuredBuffer &recv_buffer);
      ~SyncServer();

      // Remaining parameters to these functions are stored
      // in the recv buffer and get unpacked
      void mutexInit(core_id_t core_id);
      void mutexLock(core_id_t core_id);
      void mutexUnlock(core_id_t core_id);

      void condInit(core_id_t core_id);
      void condWait(core_id_t core_id);
      void condSignal(core_id_t core_id);
      void condBroadcast(core_id_t core_id);

      void barrierInit(core_id_t);
      void barrierWait(core_id_t);

   private:
      Network &m_network;
      UnstructuredBuffer &m_recv_buffer;
};

#endif // SYNC_SERVER_H
