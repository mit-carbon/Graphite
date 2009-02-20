#include "network_model_analytical_server.h"

#include "transport.h"
#include "network.h"
#include "packetize.h"
#include "config.h"
#include "core.h"

#include "log.h"
#define LOG_DEFAULT_RANK _network.getCore()->getId()
#define LOG_DEFAULT_MODULE NETWORK

NetworkModelAnalyticalServer::NetworkModelAnalyticalServer(Network &network,
      UnstructuredBuffer &recv_buffer)
      : _network(network),
      _recv_buffer(recv_buffer)
{
   int num_cores = Config::getSingleton()->getTotalCores();
   _local_utilizations.resize(num_cores);
   for (int i = 0; i < num_cores; i++)
   {
      _local_utilizations[i] = 0.0;
   }
}

NetworkModelAnalyticalServer::~NetworkModelAnalyticalServer()
{ }

void NetworkModelAnalyticalServer::update(core_id_t core_id)
{
   // extract update
   double ut;
   assert(
      _recv_buffer.get(ut)
   );
   assert(0 <= core_id && (unsigned int)core_id < Config::getSingleton()->getTotalCores());
   //  assert(0 <= ut && ut <= 1);

   _local_utilizations[core_id] = ut;

   // compute global utilization
   double global_utilization = 0.0;
   for (size_t i = 0; i < _local_utilizations.size(); i++)
   {
      global_utilization += _local_utilizations[i];
   }
   global_utilization /= _local_utilizations.size();
   //  assert(0 <= global_utilization && global_utilization <= 1);
   if (global_utilization > 1)
   {
      LOG_NOTIFY_WARNING();
      LOG_PRINT("WARNING: Network utilization exceeds 1; %f", global_utilization);
      global_utilization = 0.99;
   }

   // send response
   NetPacket response;
   response.sender = Config::getSingleton()->getMCPCoreNum();
   response.receiver = core_id;
   response.length = sizeof(global_utilization);
   response.type = MCP_UTILIZATION_UPDATE_TYPE;
   response.data = (char *) &global_utilization;

   _network.netSend(response);
}
