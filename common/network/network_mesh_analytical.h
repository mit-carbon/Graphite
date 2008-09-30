#ifndef NETWORK_MESH_ANALYTICAL_H
#define NETWORK_MESH_ANALYTICAL_H

#include "network.h"

class NetworkMeshAnalytical : public Network
{
    public:
    NetworkMeshAnalytical(Chip *chip, int tid, int num_threads);
    virtual ~NetworkMeshAnalytical();

    virtual int netSend(NetPacket packet);
    virtual NetPacket netRecv(NetMatch match);

};

#endif
