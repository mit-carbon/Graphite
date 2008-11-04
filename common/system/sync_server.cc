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


comm_id_t SimCond::wait(comm_id_t commid, StableIterator<SimMutex> & simMux)
{
  _waiting.push(make_pair(commid, simMux));
  return simMux->unlock(commid);
}

comm_id_t SimCond::signal(comm_id_t commid)
{
  if(_waiting.empty())
    return INVALID_COMMID;

  WaitPair woken = _waiting.front();
  _waiting.pop();

  if(woken.second->lock(woken.first))
  {
      return woken.first;
  }
  else
  {
      return INVALID_COMMID;
  }
}

void SimCond::broadcast(comm_id_t commid, WakeupList &woken_list)
{
  while(!_waiting.empty())
  {
      WaitPair woken = _waiting.front();
      _waiting.pop();

      if(woken.second->lock(woken.first))
          woken_list.push_back(woken.first);
  }
}

// -- SimBarrier -- //
SimBarrier::SimBarrier(UINT32 count) 
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
      _max_time = 0;
  }
}

// -- SyncServer -- //

SyncServer::SyncServer(Transport &pt_endpt, UnstructuredBuffer &recv_buffer)
  : _pt_endpt(pt_endpt),
    _recv_buffer(recv_buffer)
{ }

SyncServer::~SyncServer()
{ }

void SyncServer::mutexInit(comm_id_t commid)
{
  _mutexes.push_back(SimMutex());
  UInt32 mux = (UInt32)_mutexes.size()-1;

  _pt_endpt.ptMCPSend(commid, (UInt8*)&mux, sizeof(mux));
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
      _pt_endpt.ptMCPSend(commid, (UInt8*)&r, sizeof(r));
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
      _pt_endpt.ptMCPSend(new_owner, (UInt8*)&r, sizeof(r));
    }
  else
    {
      // nothing...
    }

  UInt32 dummy=SyncClient::MUTEX_UNLOCK_RESPONSE;
  _pt_endpt.ptMCPSend(commid, (UInt8*)&dummy, sizeof(dummy));
}

// -- Condition Variable Stuffs -- //
void SyncServer::condInit(comm_id_t commid)
{
  _conds.push_back(SimCond());
  UInt32 cond = (UInt32)_conds.size()-1;

  _pt_endpt.ptMCPSend(commid, (UInt8*)&cond, sizeof(cond));
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
  comm_id_t new_mutex_owner = psimcond->wait(commid, it);

  if (new_mutex_owner != SimMutex::NO_OWNER)
  {
      // wake up the new owner
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      _pt_endpt.ptMCPSend(new_mutex_owner, (UInt8*)&r, sizeof(r));
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

  comm_id_t woken = psimcond->signal(commid);

  if (woken != INVALID_COMMID)
  {
      // wake up the new owner
      // (note: COND_WAIT_RESPONSE == MUTEX_LOCK_RESPONSE, see header)
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      _pt_endpt.ptMCPSend(woken, (UInt8*)&r, sizeof(r));
  }
  else
  {
      // nothing...
  }

  // Alert the signaler
  UInt32 dummy=SyncClient::COND_SIGNAL_RESPONSE;
  _pt_endpt.ptMCPSend(commid, (UInt8*)&dummy, sizeof(dummy));
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
  psimcond->broadcast(commid, woken_list);

  for(SimCond::WakeupList::iterator it = woken_list.begin(); it != woken_list.end(); it++)
  {
      assert(*it != INVALID_COMMID);

      // wake up the new owner
      // (note: COND_WAIT_RESPONSE == MUTEX_LOCK_RESPONSE, see header)
      Reply r;
      r.dummy = SyncClient::MUTEX_LOCK_RESPONSE;
      r.time = time;
      _pt_endpt.ptMCPSend(*it, (UInt8*)&r, sizeof(r));
  }

  // Alert the signaler
  UInt32 dummy=SyncClient::COND_BROADCAST_RESPONSE;
  _pt_endpt.ptMCPSend(commid, (UInt8*)&dummy, sizeof(dummy));
}

void SyncServer::barrierInit(comm_id_t commid)
{
  UInt32 count;
  _recv_buffer >> count;

  _barriers.push_back(SimBarrier(count));
  UInt32 barrier = (UInt32)_barriers.size()-1;

  _pt_endpt.ptMCPSend(commid, (UInt8*)&barrier, sizeof(barrier));
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
      _pt_endpt.ptMCPSend(*it, (UInt8*)&r, sizeof(r));
  }
}

