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

    private:
    virtual UINT64 netProcCost(NetPacket packet);
    virtual UINT64 netLatency(NetPacket packet);

    //For statistical purposes
    unsigned int bytes_sent;
    unsigned int bytes_recv;

    unsigned int cycles_spent_proc;
    unsigned int cycles_spent_latency;

};

#endif
