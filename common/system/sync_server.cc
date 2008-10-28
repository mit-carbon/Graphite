#include "sync_server.h"
#include "sync_client.h"

using namespace std;

static const comm_id_t INVALID_COMMID = -1;

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
  _owner = (_waiting.empty() ? NO_OWNER : _waiting.front());
  _waiting.pop();
  return _owner;
}

// -- SimCond -- //
SimCond::SimCond() {}
SimCond::~SimCond() 
{
    assert(_waiting.empty());
}


comm_id_t SimCond::wait(comm_id_t commid, StableIterator<SimMutex> simMux)
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

  _send_buffer.clear();
  _send_buffer << mux;
  _pt_endpt.ptMCPSend(commid, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
}

void SyncServer::mutexLock(comm_id_t commid)
{
  carbon_mutex_t mux;
  _recv_buffer >> mux;

  assert((size_t)mux < _mutexes.size());

  SimMutex *psimmux = &_mutexes[mux];

  if (psimmux->lock(commid))
    {
      // notify the owner
      UInt32 dummy=SyncClient::MUTEX_LOCK_RESPONSE;
      _send_buffer.clear();
      _send_buffer << dummy;
      _pt_endpt.ptMCPSend(commid, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
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

  assert((size_t)mux < _mutexes.size());

  SimMutex *psimmux = &_mutexes[mux];

  comm_id_t new_owner = psimmux->unlock(commid);

  if (new_owner != SimMutex::NO_OWNER)
    {
      // wake up the new owner
      UInt32 dummy=SyncClient::MUTEX_LOCK_RESPONSE;
      _send_buffer.clear();
      _send_buffer << dummy;
      _pt_endpt.ptMCPSend(new_owner, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
    }
  else
    {
      // nothing...
    }

  UInt32 dummy=SyncClient::MUTEX_UNLOCK_RESPONSE;
  _send_buffer.clear();
  _send_buffer << dummy;
  _pt_endpt.ptMCPSend(commid, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
}

// -- Condition Variable Stuffs -- //
void SyncServer::condInit(comm_id_t commid)
{
  _conds.push_back(SimCond());
  UInt32 cond = (UInt32)_conds.size()-1;

  _send_buffer.clear();
  _send_buffer << cond;
  _pt_endpt.ptMCPSend(commid, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
}

void SyncServer::condWait(comm_id_t commid)
{
  carbon_cond_t cond;
  carbon_mutex_t mux;
  _recv_buffer >> cond;
  _recv_buffer >> mux; 

  assert((size_t)mux < _mutexes.size());
  assert((size_t)cond < _conds.size());

  SimCond *psimcond = &_conds[cond];

  StableIterator<SimMutex> it(_mutexes, mux);
  comm_id_t new_mutex_owner = psimcond->wait(commid, it);

  if (new_mutex_owner != SimMutex::NO_OWNER)
  {
      // wake up the new owner
      UInt32 dummy=SyncClient::MUTEX_LOCK_RESPONSE;
      _send_buffer.clear();
      _send_buffer << dummy;
      _pt_endpt.ptMCPSend(new_mutex_owner, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
  }
}


void SyncServer::condSignal(comm_id_t commid)
{
  carbon_cond_t cond;
  _recv_buffer >> cond;

  assert((size_t)cond < _conds.size());

  SimCond *psimcond = &_conds[cond];

  comm_id_t woken = psimcond->signal(commid);

  if (woken != INVALID_COMMID)
  {
      // wake up the new owner
      UInt32 dummy=SyncClient::COND_WAIT_RESPONSE;
      _send_buffer.clear();
      _send_buffer << dummy;
      _pt_endpt.ptMCPSend(woken, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
  }
  else
  {
      // nothing...
  }

  // Alert the signaler
  UInt32 dummy=SyncClient::COND_SIGNAL_RESPONSE;
  _send_buffer.clear();
  _send_buffer << dummy;
  _pt_endpt.ptMCPSend(commid, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
}

void SyncServer::condBroadcast(comm_id_t commid)
{
  carbon_cond_t cond;
  _recv_buffer >> cond;

  assert((size_t)cond < _conds.size());

  SimCond *psimcond = &_conds[cond];

  WakeupList woken_list;
  psimcond->broadcast(commid, woken_list);

  for(WakeupList::iterator it = woken_list.begin(); it != woken_list.end(); it++)
  {
      assert(*it != INVALID_COMMID);

      // wake up the new owner
      UInt32 dummy=SyncClient::COND_WAIT_RESPONSE;
      _send_buffer.clear();
      _send_buffer << dummy;
      _pt_endpt.ptMCPSend(*it, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
  }


  // Alert the signaler
  UInt32 dummy=SyncClient::COND_BROADCAST_RESPONSE;
  _send_buffer.clear();
  _send_buffer << dummy;
  _pt_endpt.ptMCPSend(commid, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
}

