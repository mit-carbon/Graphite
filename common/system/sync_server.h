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
      static const tile_id_t NO_OWNER = UINT_MAX;

      SimMutex();
      ~SimMutex();

      // returns true if this thread now owns the lock
      bool lock(tile_id_t tile_id);

      // returns the next owner of the lock so that it can be signaled by
      // the server
      tile_id_t unlock(tile_id_t tile_id);

   private:
      typedef std::queue<tile_id_t> ThreadQueue;

      ThreadQueue m_waiting;
      tile_id_t m_owner;
};

class SimCond
{

   public:
      typedef std::vector<tile_id_t> WakeupList;

      SimCond();
      ~SimCond();

      // returns the thread that gets woken up when the mux is unlocked
      tile_id_t wait(tile_id_t tile_id, UInt64 time, StableIterator<SimMutex> & it);
      tile_id_t signal(tile_id_t tile_id, UInt64 time);
      void broadcast(tile_id_t tile_id, UInt64 time, WakeupList &woken);

   private:
      class CondWaiter
      {
         public:
            CondWaiter(tile_id_t tile_id, StableIterator<SimMutex> mutex, UInt64 time)
                  : m_tile_id(tile_id), m_mutex(mutex), m_arrival_time(time) {}
            tile_id_t m_tile_id;
            StableIterator<SimMutex> m_mutex;
            UInt64 m_arrival_time;
      };

      typedef std::vector< CondWaiter > ThreadQueue;
      ThreadQueue m_waiting;
};

class SimBarrier
{
   public:
      typedef std::vector<tile_id_t> WakeupList;

      SimBarrier(UInt32 count);
      ~SimBarrier();

      // returns a list of threads to wake up if all have reached barrier
      void wait(tile_id_t tile_id, UInt64 time, WakeupList &woken);
      UInt64 getMaxTime() { return m_max_time; }

   private:
      typedef std::vector< tile_id_t > ThreadQueue;
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
      void mutexInit(tile_id_t);
      void mutexLock(tile_id_t);
      void mutexUnlock(tile_id_t);

      void condInit(tile_id_t);
      void condWait(tile_id_t);
      void condSignal(tile_id_t);
      void condBroadcast(tile_id_t);

      void barrierInit(tile_id_t);
      void barrierWait(tile_id_t);

   private:
      Network &m_network;
      UnstructuredBuffer &m_recv_buffer;
};

#endif // SYNC_SERVER_H
