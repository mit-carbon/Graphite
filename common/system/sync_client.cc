#include "sync_client.h"
#include "network.h"
#include "packetize.h"
#include "mcp.h"

#include <iostream>

using namespace std;

SyncClient::SyncClient(Network *network)
  : _network(network)
{
}

SyncClient::~SyncClient()
{
}

void SyncClient::mutexInit(int commid, carbon_mutex_t *mux)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_MUTEX_INIT;

  _send_buff << msg_type << commid;

  _network->getTransport()->ptSendToMCP((UInt8 *) _send_buff.getBuffer(), _send_buff.size());

  UInt32 length = 0;
  UInt8 *res_buff = _network->getTransport()->ptRecvFromMCP(&length);
  assert( length == sizeof(carbon_mutex_t) );

  *mux = *((carbon_mutex_t*)res_buff);
  //  _recv_buff << make_pair(res_buff, length);
  //  _recv_buff >> *mux;
}

void SyncClient::mutexLock(int commid, carbon_mutex_t *mux)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_MUTEX_LOCK;

  _send_buff << msg_type << commid << *mux;

  _network->getTransport()->ptSendToMCP((UInt8 *) _send_buff.getBuffer(), _send_buff.size());

  UInt32 length = 0;
  UInt8 *res_buff = _network->getTransport()->ptRecvFromMCP(&length);
  assert( length == sizeof(unsigned int) );

  unsigned int dummy;
  _recv_buff << make_pair(res_buff, length);
  _recv_buff >> dummy;
  assert( dummy == 0xDEADBEEF );
}

void SyncClient::mutexUnlock(int commid, carbon_mutex_t *mux)
{
  // Reset the buffers for the new transmission
  _recv_buff.clear(); 
  _send_buff.clear(); 
   
  int msg_type = MCP_MESSAGE_MUTEX_UNLOCK;

  _send_buff << msg_type << commid << *mux;

  _network->getTransport()->ptSendToMCP((UInt8 *) _send_buff.getBuffer(), _send_buff.size());

  UInt32 length = 0;
  UInt8 *res_buff = _network->getTransport()->ptRecvFromMCP(&length);
  assert( length == sizeof(unsigned int) );

  unsigned int dummy;
  _recv_buff << make_pair(res_buff, length);
  _recv_buff >> dummy;
  assert( dummy == 0xBABECAFE );
}
