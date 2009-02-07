#include "sync_client.h"
#include "network.h"
#include "core.h"
#include "packetize.h"
#include "mcp.h"

#include <iostream>

using namespace std;

SyncClient::SyncClient(Core *core)
  : _core(core)
  , _network(core->getNetwork())
{
}

SyncClient::~SyncClient()
{
}

void SyncClient::mutexInit(carbon_mutex_t *mux)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_MUTEX_INIT;

  _send_buff << msg_type;

  _network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, _send_buff.getBuffer(), _send_buff.size());

  NetPacket recv_pkt;
  recv_pkt = _network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
  assert( recv_pkt.length == sizeof(carbon_mutex_t) );

  *mux = *((carbon_mutex_t*)recv_pkt.data);

  delete [] (Byte*) recv_pkt.data;
}

void SyncClient::mutexLock(carbon_mutex_t *mux)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_MUTEX_LOCK;

  _send_buff << msg_type << *mux << _core->getPerfModel()->getCycleCount();

  _network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, _send_buff.getBuffer(), _send_buff.size());

  NetPacket recv_pkt;
  recv_pkt = _network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
  assert( recv_pkt.length == sizeof(unsigned int) + sizeof(UInt64) );

  unsigned int dummy;
  UInt64 time;
  _recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
  _recv_buff >> dummy;
  assert( dummy == MUTEX_LOCK_RESPONSE );

  _recv_buff >> time;
  _core->getPerfModel()->updateCycleCount(time);

  delete [] (Byte*) recv_pkt.data;
}

void SyncClient::mutexUnlock(carbon_mutex_t *mux)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_MUTEX_UNLOCK;

  _send_buff << msg_type << *mux << _core->getPerfModel()->getCycleCount();

  _network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, _send_buff.getBuffer(), _send_buff.size());

  NetPacket recv_pkt;
  recv_pkt = _network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
  assert( recv_pkt.length == sizeof(unsigned int) );

  unsigned int dummy;
  _recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
  _recv_buff >> dummy;
  assert( dummy == MUTEX_UNLOCK_RESPONSE );

  delete [] (Byte*) recv_pkt.data;
}

void SyncClient::condInit(carbon_cond_t *cond)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_COND_INIT;

  _send_buff << msg_type << *cond << _core->getPerfModel()->getCycleCount();

  _network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, _send_buff.getBuffer(), _send_buff.size());

  NetPacket recv_pkt;
  recv_pkt = _network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
  assert( recv_pkt.length == sizeof(carbon_cond_t) );

  *cond = *((carbon_cond_t*)recv_pkt.data);

  delete [] (Byte*) recv_pkt.data;
}

void SyncClient::condWait(carbon_cond_t *cond, carbon_mutex_t *mux)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_COND_WAIT;

  _send_buff << msg_type << *cond << *mux << _core->getPerfModel()->getCycleCount();

  _network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, _send_buff.getBuffer(), _send_buff.size());

  NetPacket recv_pkt;
  recv_pkt = _network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
  assert( recv_pkt.length == sizeof(unsigned int) + sizeof(UInt64) );

  unsigned int dummy;
  _recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
  _recv_buff >> dummy;
  assert( dummy == COND_WAIT_RESPONSE );

  UInt64 time;
  _recv_buff >> time;
  _core->getPerfModel()->updateCycleCount(time);

  delete [] (Byte*) recv_pkt.data;
}

void SyncClient::condSignal(carbon_cond_t *cond)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_COND_SIGNAL;

  _send_buff << msg_type << *cond << _core->getPerfModel()->getCycleCount();

  _network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, _send_buff.getBuffer(), _send_buff.size());

  NetPacket recv_pkt;
  recv_pkt = _network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
  assert( recv_pkt.length == sizeof(unsigned int) );

  unsigned int dummy;
  _recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
  _recv_buff >> dummy;
  assert( dummy == COND_SIGNAL_RESPONSE );

  delete [] (Byte*) recv_pkt.data;
}

void SyncClient::condBroadcast(carbon_cond_t *cond)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_COND_BROADCAST;

  _send_buff << msg_type << *cond << _core->getPerfModel()->getCycleCount();

  _network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, _send_buff.getBuffer(), _send_buff.size());

  NetPacket recv_pkt;
  recv_pkt = _network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
  assert( recv_pkt.length == sizeof(unsigned int) );

  unsigned int dummy;
  _recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
  _recv_buff >> dummy;
  assert( dummy == COND_BROADCAST_RESPONSE );

  delete [] (Byte*) recv_pkt.data;
}

void SyncClient::barrierInit(carbon_barrier_t *barrier, UInt32 count)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_BARRIER_INIT;

  _send_buff << msg_type << count << _core->getPerfModel()->getCycleCount();

  _network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, _send_buff.getBuffer(), _send_buff.size());

  NetPacket recv_pkt;
  recv_pkt = _network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
  assert( recv_pkt.length == sizeof(carbon_barrier_t) );

  *barrier = *((carbon_barrier_t*)recv_pkt.data);

  delete [] (Byte*) recv_pkt.data;
}

void SyncClient::barrierWait(carbon_barrier_t *barrier)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_BARRIER_WAIT;

  _send_buff << msg_type << *barrier << _core->getPerfModel()->getCycleCount();

  _network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, _send_buff.getBuffer(), _send_buff.size());

  NetPacket recv_pkt;
  recv_pkt = _network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
  assert( recv_pkt.length == sizeof(unsigned int) + sizeof(UInt64) );

  unsigned int dummy;
  _recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
  _recv_buff >> dummy;
  assert( dummy == BARRIER_WAIT_RESPONSE );

  UInt64 time;
  _recv_buff >> time;
  _core->getPerfModel()->updateCycleCount(time);

  delete [] (Byte*) recv_pkt.data;
}

