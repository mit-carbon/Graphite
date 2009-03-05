#include "sync_server.h"
#include "sync_client.h"

using namespace std;

struct Reply
{
   UInt32 dummy;
   UInt64 time;
};

// -- SimMutex -- //

SimMutex::SimMutex()
      : m_owner(NO_OWNER)
{ }

SimMutex::~SimMutex()
{
   assert(m_waiting.empty());
}

bool SimMutex::lock(core_id_t core_id)
{
   if (m_owner == NO_OWNER)
   {
      m_owner = core_id;
      return true;
   }
   else
   {
      m_waiting.push(core_id);
      return false;
   }
}

core_id_t SimMutex::unlock(core_id_t core_id)
{
   assert(m_owner == core_id);
   if (m_waiting.empty())
   {
      m_owner = NO_OWNER;
   }
   else
   {
      m_owner =  m_waiting.front();
      m_waiting.pop();
   }
   return m_owner;
}

// -- SimCond -- //
SimCond::SimCond() {}
SimCond::~SimCond()
{
   assert(m_waiting.empty());
}


core_id_t SimCond::wait(core_id_t core_id, UInt64 time, StableIterator<SimMutex> & simMux)
{

   // First check to see if we have gotten any signals later in 'virtual time'
   for (SignalQueue::iterator i = m_signals.begin(); i != m_signals.end(); i++)
   {
      if ((*i) > time)
      {
         //Remove the pending signal
         m_signals.erase(i,i+1);

         //Let the manager know to wake up this thread
         return core_id;
      }
   }

   // If we don't have any later signals, then put this request in the queue
   m_waiting.push_back(CondWaiter(core_id, simMux, time));
   return simMux->unlock(core_id);
}

core_id_t SimCond::signal(core_id_t core_id, UInt64 time)
{
   // If no threads are waiting, store this cond incase a new
   // thread arrives with an earlier time
   if (m_waiting.empty())
   {
      m_signals.push_back(time);
      return INVALID_CORE_ID;
   }

   // If there is a list of threads waiting, wake up one of them
   // if it has a time later than this signal
   for (ThreadQueue::iterator i = m_waiting.begin(); i != m_waiting.end(); i++)
   {
      //FIXME: This should be uncommented once the proper timings are working
      //cerr << "comparing signal time: " << (int)time << " with: " << (*i).m_arrival_time << endl;
      if (time > (*i).m_arrival_time)
      {
         CondWaiter woken = (*i);
         m_waiting.erase(i);

         if (woken.m_mutex->lock(woken.m_comm_id))
            return woken.m_comm_id;
         else
            return INVALID_CORE_ID;
      }
   }

   // If none of the waiting threads have a later time, then save this
   // signal for later usage (incase a thread arrives at later physical
   // time but earlier virtual time).
   m_signals.push_back(time);
   return INVALID_CORE_ID;

}

//FIXME: cond broadcast does not properly handle out of order signals
void SimCond::broadcast(core_id_t core_id, UInt64 time, WakeupList &woken_list)
{
   while (!m_waiting.empty())
   {
      CondWaiter woken = *(m_waiting.begin());
      m_waiting.erase(m_waiting.begin(), m_waiting.begin()+1);

      if (woken.m_mutex->lock(woken.m_comm_id))
         woken_list.push_back(woken.m_comm_id);
   }
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

void SimBarrier::wait(core_id_t core_id, UInt64 time, WakeupList &woken_list)
{
   m_waiting.push_back(core_id);

   assert(m_waiting.size() <= m_count);

   if (m_waiting.size() == 1)
      m_max_time = time;
   else if (time > m_max_time)
      m_max_time = time;

   // All threads have reached the barrier
   if (m_waiting.size() == m_count)
   {
      woken_list = m_waiting;
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

void SyncServer::mutexInit(core_id_t core_id)
{
   m_mutexes.push_back(SimMutex());
   UInt32 mux = (UInt32)m_mutexes.size()-1;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, (char*)&mux, sizeof(mux));
}

void SyncServer::mutexLock(core_id_t core_id)
{
   carbon_mutex_t mux;
   m_recv_buffer >> mux;

   UInt64 time;
   m_recv_buffer >> time;

   assert((size_t)mux < m_mutexes.size());

   SimMutex *psimmux = &m_mutexes[mux];

   if (psimmux->lock(core_id))
   {
      // notify the owner
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      m_network.netSend(core_id, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
   }
   else
   {
      // nothing...owner goes to sleep
   }
}

void SyncServer::mutexUnlock(core_id_t core_id)
{
   carbon_mutex_t mux;
   m_recv_buffer >> mux;

   UInt64 time;
   m_recv_buffer >> time;

   assert((size_t)mux < m_mutexes.size());

   SimMutex *psimmux = &m_mutexes[mux];

   core_id_t new_owner = psimmux->unlock(core_id);

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

   UInt32 dummy=SyncClient::MUTEX_UNLOCK_RESPONSE;
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, (char*)&dummy, sizeof(dummy));
}

// -- Condition Variable Stuffs -- //
void SyncServer::condInit(core_id_t core_id)
{
   m_conds.push_back(SimCond());
   UInt32 cond = (UInt32)m_conds.size()-1;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, (char*)&cond, sizeof(cond));
}

void SyncServer::condWait(core_id_t core_id)
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
   core_id_t new_mutex_owner = psimcond->wait(core_id, time, it);

   if (new_mutex_owner != SimMutex::NO_OWNER)
   {
      // wake up the new owner
      Reply r;

      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      m_network.netSend(new_mutex_owner, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
   }
}


void SyncServer::condSignal(core_id_t core_id)
{
   carbon_cond_t cond;
   m_recv_buffer >> cond;

   UInt64 time;
   m_recv_buffer >> time;

   assert((size_t)cond < m_conds.size());

   SimCond *psimcond = &m_conds[cond];

   core_id_t woken = psimcond->signal(core_id, time);

   if (woken != INVALID_CORE_ID)
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
   UInt32 dummy=SyncClient::COND_SIGNAL_RESPONSE;
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, (char*)&dummy, sizeof(dummy));
}

void SyncServer::condBroadcast(core_id_t core_id)
{
   carbon_cond_t cond;
   m_recv_buffer >> cond;

   UInt64 time;
   m_recv_buffer >> time;

   assert((size_t)cond < m_conds.size());

   SimCond *psimcond = &m_conds[cond];

   SimCond::WakeupList woken_list;
   psimcond->broadcast(core_id, time, woken_list);

   for (SimCond::WakeupList::iterator it = woken_list.begin(); it != woken_list.end(); it++)
   {
      assert(*it != INVALID_CORE_ID);

      // wake up the new owner
      // (note: COND_WAIT_RESPONSE == MUTEX_LOCK_RESPONSE, see header)
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      m_network.netSend(*it, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
   }

   // Alert the signaler
   UInt32 dummy=SyncClient::COND_BROADCAST_RESPONSE;
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, (char*)&dummy, sizeof(dummy));
}

void SyncServer::barrierInit(core_id_t core_id)
{
   UInt32 count;
   m_recv_buffer >> count;

   m_barriers.push_back(SimBarrier(count));
   UInt32 barrier = (UInt32)m_barriers.size()-1;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, (char*)&barrier, sizeof(barrier));
}

void SyncServer::barrierWait(core_id_t core_id)
{
   carbon_barrier_t barrier;
   m_recv_buffer >> barrier;

   UInt64 time;
   m_recv_buffer >> time;

   assert((size_t)barrier < m_barriers.size());

   SimBarrier *psimbarrier = &m_barriers[barrier];

   SimBarrier::WakeupList woken_list;
   psimbarrier->wait(core_id, time, woken_list);

   UInt64 max_time = psimbarrier->getMaxTime();

   for (SimBarrier::WakeupList::iterator it = woken_list.begin(); it != woken_list.end(); it++)
   {
      assert(*it != INVALID_CORE_ID);
      Reply r;
      r.dummy = SyncClient::BARRIER_WAIT_RESPONSE;
      r.time = max_time;
      m_network.netSend(*it, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
   }
}

