#include "network_mesh_analytical_server.h"

#include "transport.h"
#include "network.h"
#include "packetize.h"
#include "config.h"

NetworkMeshAnalyticalServer::NetworkMeshAnalyticalServer(Network &network,
                                                         UnstructuredBuffer &recv_buffer)
  : _network(network),
    _recv_buffer(recv_buffer)
{ 
  int num_cores = g_config->numMods();
  _local_utilizations.resize(num_cores);
  for (int i = 0; i < num_cores; i++)
    {
      _local_utilizations[i] = 0.0;
    }
}

NetworkMeshAnalyticalServer::~NetworkMeshAnalyticalServer()
{ }

void NetworkMeshAnalyticalServer::update(comm_id_t commid)
{
  // extract update
  double ut;
  assert(
         _recv_buffer.get(ut)
         );
  assert(0 <= commid && (unsigned int)commid < g_config->numMods());
  //  assert(0 <= ut && ut <= 1);

  _local_utilizations[commid] = ut;

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
        fprintf(stderr, "WARNING: Network utilization exceeds 1; %f\n", global_utilization);
        global_utilization = 0.99;
     }

  // send response
  NetPacket response;
  response.sender = g_config->MCPCommID();
  response.receiver = commid;
  response.length = sizeof(global_utilization);
  response.type = MCP_UTILIZATION_UPDATE_TYPE;
  response.data = (char *) &global_utilization;

  _network.netSend(response);
}
