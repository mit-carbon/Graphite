#include "sync_server.h"
#include "sync_client.h"

using namespace std;

static const comm_id_t INVALID_COMMID = -1;

struct Reply
{
  UInt32 dummy;
  UInt64 time;
};

// -- SimMutex -- //

SimMutex::SimMutex()
  : _owner(NO_OWNER)
{ }

SimMutex::~SimMutex()
{
  assert(_waiting.empty());
}

bool SimMutex::lock(comm_id_t commid)
{
  if (_owner == NO_OWNER)
    {
      _owner = commid;
      return true;
    }
  else
    {
      _waiting.push(commid);
      return false;
    }
}

comm_id_t SimMutex::unlock(comm_id_t commid)
{
  assert(_owner == commid);
  if(_waiting.empty())
  {
      _owner = NO_OWNER;
  }
  else 
  {
      _owner =  _waiting.front();
      _waiting.pop();
  }
  return _owner;
}

// -- SimCond -- //
SimCond::SimCond() {}
SimCond::~SimCond() 
{
  assert(_waiting.empty());
}


comm_id_t SimCond::wait(comm_id_t commid, UInt64 time, StableIterator<SimMutex> & simMux)
{

  // First check to see if we have gotten any signals later in 'virtual time'
  for(SignalQueue::iterator i = _signals.begin(); i != _signals.end(); i++)
  {
      if((*i) > time)
      {
          //Remove the pending signal
          _signals.erase(i,i+1);

          //Let the manager know to wake up this thread
          return commid;
      }
  }

  // If we don't have any later signals, then put this request in the queue
  _waiting.push_back(CondWaiter(commid, simMux, time));
  return simMux->unlock(commid);
}

comm_id_t SimCond::signal(comm_id_t commid, UInt64 time)
{
  // If no threads are waiting, store this cond incase a new
  // thread arrives with an earlier time
  if(_waiting.empty())
  {
    _signals.push_back(time);
    return INVALID_COMMID;
  }

  // If there is a list of threads waiting, wake up one of them
  // if it has a time later than this signal
  for(ThreadQueue::iterator i = _waiting.begin(); i != _waiting.end(); i++)
  {
      //FIXME: This should be uncommented once the proper timings are working
      //cerr << "comparing signal time: " << (int)time << " with: " << (*i)._arrival_time << endl;
      if(time > (*i)._arrival_time)
      {
          CondWaiter woken = (*i);
          _waiting.erase(i);

          if(woken._mutex->lock(woken._comm_id))
              return woken._comm_id;
          else
              return INVALID_COMMID;
      }
  }

  // If none of the waiting threads have a later time, then save this
  // signal for later usage (incase a thread arrives at later physical
  // time but earlier virtual time).
  _signals.push_back(time);
  return INVALID_COMMID;

}

//FIXME: cond broadcast does not properly handle out of order signals
void SimCond::broadcast(comm_id_t commid, UInt64 time, WakeupList &woken_list)
{
  while(!_waiting.empty())
  {
      CondWaiter woken = *(_waiting.begin());
      _waiting.erase(_waiting.begin(), _waiting.begin()+1);

      if(woken._mutex->lock(woken._comm_id))
          woken_list.push_back(woken._comm_id);
  }
}

// -- SimBarrier -- //
SimBarrier::SimBarrier(UInt32 count) 
   : _count(count)
   , _max_time(0)
{
}

SimBarrier::~SimBarrier() 
{
  assert(_waiting.empty());
}

void SimBarrier::wait(comm_id_t commid, UInt64 time, WakeupList &woken_list)
{
  _waiting.push_back(commid);

  assert(_waiting.size() <= _count);

  if(_waiting.size() == 1)
     _max_time = time;
  else if(time > _max_time)
     _max_time = time;

  // All threads have reached the barrier
  if(_waiting.size() == _count)
  {
      woken_list = _waiting;
      _waiting.clear();
  }
}

// -- SyncServer -- //

SyncServer::SyncServer(Network &network, UnstructuredBuffer &recv_buffer)
  : _network(network),
    _recv_buffer(recv_buffer)
{ }

SyncServer::~SyncServer()
{ }

void SyncServer::mutexInit(comm_id_t commid)
{
  _mutexes.push_back(SimMutex());
  UInt32 mux = (UInt32)_mutexes.size()-1;

  _network.netSend(commid, MCP_RESPONSE_TYPE, (char*)&mux, sizeof(mux));
}

void SyncServer::mutexLock(comm_id_t commid)
{
  carbon_mutex_t mux;
  _recv_buffer >> mux;

  UInt64 time;
  _recv_buffer >> time;

  assert((size_t)mux < _mutexes.size());

  SimMutex *psimmux = &_mutexes[mux];

  if (psimmux->lock(commid))
    {
      // notify the owner
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      _network.netSend(commid, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
    }
  else
    {
      // nothing...owner goes to sleep
    }
}

void SyncServer::mutexUnlock(comm_id_t commid)
{
  carbon_mutex_t mux;
  _recv_buffer >> mux;

  UInt64 time;
  _recv_buffer >> time;

  assert((size_t)mux < _mutexes.size());

  SimMutex *psimmux = &_mutexes[mux];

  comm_id_t new_owner = psimmux->unlock(commid);

  if (new_owner != SimMutex::NO_OWNER)
    {
      // wake up the new owner
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      _network.netSend(new_owner, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
    }
  else
    {
      // nothing...
    }

  UInt32 dummy=SyncClient::MUTEX_UNLOCK_RESPONSE;
  _network.netSend(commid, MCP_RESPONSE_TYPE, (char*)&dummy, sizeof(dummy));
}

// -- Condition Variable Stuffs -- //
void SyncServer::condInit(comm_id_t commid)
{
  _conds.push_back(SimCond());
  UInt32 cond = (UInt32)_conds.size()-1;

  _network.netSend(commid, MCP_RESPONSE_TYPE, (char*)&cond, sizeof(cond));
}

void SyncServer::condWait(comm_id_t commid)
{
  carbon_cond_t cond;
  carbon_mutex_t mux;
  _recv_buffer >> cond;
  _recv_buffer >> mux; 

  UInt64 time;
  _recv_buffer >> time;

  assert((size_t)mux < _mutexes.size());
  assert((size_t)cond < _conds.size());

  SimCond *psimcond = &_conds[cond];

  StableIterator<SimMutex> it(_mutexes, mux);
  comm_id_t new_mutex_owner = psimcond->wait(commid, time, it);

  if (new_mutex_owner != SimMutex::NO_OWNER)
  {
      // wake up the new owner
      Reply r;

      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      _network.netSend(new_mutex_owner, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
  }
}


void SyncServer::condSignal(comm_id_t commid)
{
  carbon_cond_t cond;
  _recv_buffer >> cond;
  
  UInt64 time;
  _recv_buffer >> time;

  assert((size_t)cond < _conds.size());

  SimCond *psimcond = &_conds[cond];

  comm_id_t woken = psimcond->signal(commid, time);

  if (woken != INVALID_COMMID)
  {
      // wake up the new owner
      // (note: COND_WAIT_RESPONSE == MUTEX_LOCK_RESPONSE, see header)
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      _network.netSend(woken, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
  }
  else
  {
      // nothing...
  }

  // Alert the signaler
  UInt32 dummy=SyncClient::COND_SIGNAL_RESPONSE;
  _network.netSend(commid, MCP_RESPONSE_TYPE, (char*)&dummy, sizeof(dummy));
}

void SyncServer::condBroadcast(comm_id_t commid)
{
  carbon_cond_t cond;
  _recv_buffer >> cond;

  UInt64 time;
  _recv_buffer >> time;

  assert((size_t)cond < _conds.size());

  SimCond *psimcond = &_conds[cond];

  SimCond::WakeupList woken_list;
  psimcond->broadcast(commid, time, woken_list);

  for(SimCond::WakeupList::iterator it = woken_list.begin(); it != woken_list.end(); it++)
  {
      assert(*it != INVALID_COMMID);

      // wake up the new owner
      // (note: COND_WAIT_RESPONSE == MUTEX_LOCK_RESPONSE, see header)
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      _network.netSend(*it, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
  }

  // Alert the signaler
  UInt32 dummy=SyncClient::COND_BROADCAST_RESPONSE;
  _network.netSend(commid, MCP_RESPONSE_TYPE, (char*)&dummy, sizeof(dummy));
}

void SyncServer::barrierInit(comm_id_t commid)
{
  UInt32 count;
  _recv_buffer >> count;

  _barriers.push_back(SimBarrier(count));
  UInt32 barrier = (UInt32)_barriers.size()-1;

  _network.netSend(commid, MCP_RESPONSE_TYPE, (char*)&barrier, sizeof(barrier));
}

void SyncServer::barrierWait(comm_id_t commid)
{
  carbon_barrier_t barrier;
  _recv_buffer >> barrier;

  UInt64 time;
  _recv_buffer >> time;

  assert((size_t)barrier < _barriers.size());

  SimBarrier *psimbarrier = &_barriers[barrier];

  SimCond::WakeupList woken_list;
  psimbarrier->wait(commid, time, woken_list);

  UInt64 max_time = psimbarrier->getMaxTime();

  for(SimCond::WakeupList::iterator it = woken_list.begin(); it != woken_list.end(); it++)
  {
      assert(*it != INVALID_COMMID);
      Reply r;
      r.dummy = SyncClient::BARRIER_WAIT_RESPONSE;
      r.time = max_time;
      _network.netSend(*it, MCP_RESPONSE_TYPE, (char*)&r, sizeof(r));
  }
}

