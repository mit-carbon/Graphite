#ifndef NETWORK_MESH_ANALYTICAL_SERVER_H
#define NETWORK_MESH_ANALYTICAL_SERVER_H

#include <vector>

class Network;
class UnstructuredBuffer;

typedef int comm_id_t;

class NetworkMeshAnalyticalServer
{
 private:
  std::vector<double> _local_utilizations;

  Network & _network;
  UnstructuredBuffer & _recv_buffer;

 public:
  NetworkMeshAnalyticalServer(Network &network, UnstructuredBuffer &recv_buffer);
  ~NetworkMeshAnalyticalServer();

  void update(comm_id_t);
};

#endif
