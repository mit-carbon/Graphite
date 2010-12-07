#include "sync_server.h"
#include "sync_client.h"
#include "simulator.h"
#include "thread_manager.h"

using namespace std;

struct Reply
{
   UInt32 dummy;
   UInt64 time __attribute__((packed));
};

// -- SimMutex -- //

SimMutex::SimMutex()
      : m_owner(NO_OWNER)
{ }

SimMutex::~SimMutex()
{
   assert(m_waiting.empty());
}

bool SimMutex::lock(tile_id_t tile_id)
{
   if (m_owner == NO_OWNER)
   {
      m_owner = tile_id;
      return true;
   }
   else
   {
      Sim()->getThreadManager()->stallThread(tile_id);
      m_waiting.push(tile_id);
      return false;
   }
}

tile_id_t SimMutex::unlock(tile_id_t tile_id)
{
   assert(m_owner == tile_id);
   if (m_waiting.empty())
   {
      m_owner = NO_OWNER;
   }
   else
   {
      m_owner =  m_waiting.front();
      m_waiting.pop();
      Sim()->getThreadManager()->resumeThread(m_owner);
   }
   return m_owner;
}

// -- SimCond -- //
// FIXME: Currently, 'simulated times' are ignored in the synchronization constructs
SimCond::SimCond() {}
SimCond::~SimCond()
{
   assert(m_waiting.empty());
}

tile_id_t SimCond::wait(tile_id_t tile_id, UInt64 time, StableIterator<SimMutex> & simMux)
{
   Sim()->getThreadManager()->stallThread(tile_id);

   // If we don't have any later signals, then put this request in the queue
   m_waiting.push_back(CondWaiter(tile_id, simMux, time));
   return simMux->unlock(tile_id);
}

tile_id_t SimCond::signal(tile_id_t tile_id, UInt64 time)
{
   // If there is a list of threads waiting, wake up one of them
   if (!m_waiting.empty())
   {
      CondWaiter woken = *(m_waiting.begin());
      m_waiting.erase(m_waiting.begin());

      Sim()->getThreadManager()->resumeThread(woken.m_tile_id);

      if (woken.m_mutex->lock(woken.m_tile_id))
      {
         // Woken up thread is able to grab lock immediately
         return woken.m_tile_id;
      }
      else
      {
         // Woken up thread is *NOT* able to grab lock immediately
         return INVALID_TILE_ID;
      }
   }

   // There are *NO* threads waiting on the condition variable
   return INVALID_TILE_ID;
}

void SimCond::broadcast(tile_id_t tile_id, UInt64 time, WakeupList &woken_list)
{
   for (ThreadQueue::iterator i = m_waiting.begin(); i != m_waiting.end(); i++)
   {
      CondWaiter woken = *(i);

      Sim()->getThreadManager()->resumeThread(woken.m_tile_id);

      if (woken.m_mutex->lock(woken.m_tile_id))
      {
         // Woken up thread is able to grab lock immediately
         woken_list.push_back(woken.m_tile_id);
      }
   }

   // All waiting threads have been woken up from the CondVar queue
   m_waiting.clear();
}

// -- SimBarrier -- //
SimBarrier::SimBarrier(UInt32 count)
      : m_count(count)
      , m_max_time(0)
{
}

SimBarrier::~SimBarrier()
{
   assert(m_waiting.empty());
}

void SimBarrier::wait(tile_id_t tile_id, UInt64 time, WakeupList &woken_list)
{
   m_waiting.push_back(tile_id);

   Sim()->getThreadManager()->stallThread(tile_id);

   assert(m_waiting.size() <= m_count);

   if (m_waiting.size() == 1)
      m_max_time = time;
   else if (time > m_max_time)
      m_max_time = time;

   // All threads have reached the barrier
   if (m_waiting.size() == m_count)
   {
      woken_list = m_waiting;

      for (WakeupList::iterator i = woken_list.begin(); i != woken_list.end(); i++)
      {
         // Resuming all the threads stalled at the barrier
         Sim()->getThreadManager()->resumeThread(*i);
      }
      m_waiting.clear();
   }
}

// -- SyncServer -- //

SyncServer::SyncServer(Network &network, UnstructuredBuffer &recv_buffer)
      : m_network(network),
      m_recv_buffer(recv_buffer)
{ }

SyncServer::~SyncServer()
{ }

void SyncServer::mutexInit(tile_id_t tile_id)
{
   m_mutexes.push_back(SimMutex());
   UInt32 mux = (UInt32)m_mutexes.size()-1;

   m_network.netSend(tile_id, MCP_RESPONSE_TYPE, (char*)&mux, sizeof(mux));
}

void SyncServer::mutexLock(tile_id_t tile_id)
{
   carbon_mutex_t mux;
   m_recv_buffer >> mux;

   UInt64 time;
   m_recv_buffer >> time;

   assert((size_t)mux < m_mutexes.size());

   SimMutex *psimmux = &m_mutexes[mux];

   if (psimmux->lock(tile_id))
   {
      // notify the owner
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      m_network.netSend(tile_id, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
   }
   else
   {
      // nothing...thread goes to sleep
   }
}

void SyncServer::mutexUnlock(tile_id_t tile_id)
{
   carbon_mutex_t mux;
   m_recv_buffer >> mux;

   UInt64 time;
   m_recv_buffer >> time;

   assert((size_t)mux < m_mutexes.size());

   SimMutex *psimmux = &m_mutexes[mux];

   tile_id_t new_owner = psimmux->unlock(tile_id);

   if (new_owner != SimMutex::NO_OWNER)
   {
      // wake up the new owner
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      m_network.netSend(new_owner, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
   }
   else
   {
      // nothing...
   }

   UInt32 dummy = SyncClient::MUTEX_UNLOCK_RESPONSE;
   m_network.netSend(tile_id, MCP_RESPONSE_TYPE, (char*)&dummy, sizeof(dummy));
}

// -- Condition Variable Stuffs -- //
void SyncServer::condInit(tile_id_t tile_id)
{
   m_conds.push_back(SimCond());
   UInt32 cond = (UInt32)m_conds.size()-1;

   m_network.netSend(tile_id, MCP_RESPONSE_TYPE, (char*)&cond, sizeof(cond));
}

void SyncServer::condWait(tile_id_t tile_id)
{
   carbon_cond_t cond;
   carbon_mutex_t mux;
   m_recv_buffer >> cond;
   m_recv_buffer >> mux;

   UInt64 time;
   m_recv_buffer >> time;

   assert((size_t)mux < m_mutexes.size());
   assert((size_t)cond < m_conds.size());

   SimCond *psimcond = &m_conds[cond];

   StableIterator<SimMutex> it(m_mutexes, mux);
   tile_id_t new_mutex_owner = psimcond->wait(tile_id, time, it);

   if (new_mutex_owner != SimMutex::NO_OWNER)
   {
      // wake up the new owner
      Reply r;

      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      m_network.netSend(new_mutex_owner, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
   }
}


void SyncServer::condSignal(tile_id_t tile_id)
{
   carbon_cond_t cond;
   m_recv_buffer >> cond;

   UInt64 time;
   m_recv_buffer >> time;

   assert((size_t)cond < m_conds.size());

   SimCond *psimcond = &m_conds[cond];

   tile_id_t woken = psimcond->signal(tile_id, time);

   if (woken != INVALID_TILE_ID)
   {
      // wake up the new owner
      // (note: COND_WAIT_RESPONSE == MUTEX_LOCK_RESPONSE, see header)
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      m_network.netSend(woken, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
   }
   else
   {
      // nothing...
   }

   // Alert the signaler
   UInt32 dummy = SyncClient::COND_SIGNAL_RESPONSE;
   m_network.netSend(tile_id, MCP_RESPONSE_TYPE, (char*)&dummy, sizeof(dummy));
}

void SyncServer::condBroadcast(tile_id_t tile_id)
{
   carbon_cond_t cond;
   m_recv_buffer >> cond;

   UInt64 time;
   m_recv_buffer >> time;

   assert((size_t)cond < m_conds.size());

   SimCond *psimcond = &m_conds[cond];

   SimCond::WakeupList woken_list;
   psimcond->broadcast(tile_id, time, woken_list);

   for (SimCond::WakeupList::iterator it = woken_list.begin(); it != woken_list.end(); it++)
   {
      assert(*it != INVALID_TILE_ID);

      // wake up the new owner
      // (note: COND_WAIT_RESPONSE == MUTEX_LOCK_RESPONSE, see header)
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      m_network.netSend(*it, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
   }

   // Alert the signaler
   UInt32 dummy = SyncClient::COND_BROADCAST_RESPONSE;
   m_network.netSend(tile_id, MCP_RESPONSE_TYPE, (char*)&dummy, sizeof(dummy));
}

void SyncServer::barrierInit(tile_id_t tile_id)
{
   UInt32 count;
   m_recv_buffer >> count;

   m_barriers.push_back(SimBarrier(count));
   UInt32 barrier = (UInt32)m_barriers.size()-1;

   m_network.netSend(tile_id, MCP_RESPONSE_TYPE, (char*)&barrier, sizeof(barrier));
}

void SyncServer::barrierWait(tile_id_t tile_id)
{
   carbon_barrier_t barrier;
   m_recv_buffer >> barrier;

   UInt64 time;
   m_recv_buffer >> time;

   LOG_ASSERT_ERROR(barrier < (tile_id_t) m_barriers.size(), "barrier = %i, m_barriers.size()= %u", barrier, m_barriers.size());

   SimBarrier *psimbarrier = &m_barriers[barrier];

   SimBarrier::WakeupList woken_list;
   psimbarrier->wait(tile_id, time, woken_list);

   UInt64 max_time = psimbarrier->getMaxTime();

   for (SimBarrier::WakeupList::iterator it = woken_list.begin(); it != woken_list.end(); it++)
   {
      assert(*it != INVALID_TILE_ID);
      Reply r;
      r.dummy = SyncClient::BARRIER_WAIT_RESPONSE;
      r.time = max_time;
      m_network.netSend(*it, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
   }
}
