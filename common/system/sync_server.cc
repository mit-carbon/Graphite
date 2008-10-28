#include "sync_server.h"

using namespace std;

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
      UInt32 dummy=0xDEADBEEF;
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
      UInt32 dummy=0xDEADBEEF;
      _send_buffer.clear();
      _send_buffer << dummy;
      _pt_endpt.ptMCPSend(new_owner, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
    }
  else
    {
      // nothing...
    }

  UInt32 dummy=0xBABECAFE;
  _send_buffer.clear();
  _send_buffer << dummy;
  _pt_endpt.ptMCPSend(commid, (UInt8*)_send_buffer.getBuffer(), _send_buffer.size());
}
