#ifndef NETWORK_MODEL_ANALYTICAL_SERVER_H
#define NETWORK_MODEL_ANALYTICAL_SERVER_H

#include <vector>

class Network;
class UnstructuredBuffer;

typedef int comm_id_t;

class NetworkModelAnalyticalServer
{
 private:
  std::vector<double> _local_utilizations;

  Network & _network;
  UnstructuredBuffer & _recv_buffer;

 public:
  NetworkModelAnalyticalServer(Network &network, UnstructuredBuffer &recv_buffer);
  ~NetworkModelAnalyticalServer();

  void update(comm_id_t);
};

#endif
