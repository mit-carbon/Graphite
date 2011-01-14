#include "network_model_analytical_server.h"

#include "transport.h"
#include "network.h"
#include "packetize.h"
#include "config.h"
#include "tile.h"

#include "log.h"

NetworkModelAnalyticalServer::NetworkModelAnalyticalServer(Network &network,
      UnstructuredBuffer &recv_buffer)
      : _network(network),
      _recv_buffer(recv_buffer)
{
   int num_tiles = Config::getSingleton()->getTotalTiles();
   _local_utilizations.resize(num_tiles);
   for (int i = 0; i < num_tiles; i++)
   {
      _local_utilizations[i] = 0.0;
   }
}

NetworkModelAnalyticalServer::~NetworkModelAnalyticalServer()
{ }

struct UtilizationMessage
{
   double ut;
   void *vpmodel;
};

void NetworkModelAnalyticalServer::update(tile_id_t tile_id)
{
   // extract update
   assert(_recv_buffer.size() == sizeof(UtilizationMessage));
   assert(0 <= tile_id && (unsigned int)tile_id < Config::getSingleton()->getTotalTiles());

   UtilizationMessage *msg;
   msg = (UtilizationMessage*)_recv_buffer.getBuffer();
   assert(msg->vpmodel != NULL);

   _local_utilizations[tile_id] = msg->ut;

   // compute global utilization
   double global_utilization = 0.0;
   for (size_t i = 0; i < _local_utilizations.size(); i++)
   {
      global_utilization += _local_utilizations[i];
   }
   global_utilization /= _local_utilizations.size();
   //  assert(0 <= global_utilization && global_utilization <= 1);
   if (global_utilization < 0 || global_utilization > 1)
   {
      LOG_PRINT_WARNING("Unusual network utilization value: %f", global_utilization);
      global_utilization = 0.99;
   }

   UtilizationMessage response_msg = { global_utilization, msg->vpmodel };

   // send response
   NetPacket response;
   response.sender.first = Config::getSingleton()->getMCPTileNum();
   response.receiver.first = tile_id;
   response.length = sizeof(response_msg);
   response.type = MCP_UTILIZATION_UPDATE_TYPE;
   response.data = &response_msg;

   _network.netSend(response);
}
