#ifndef NETWORK_MESH_ANALYTICAL_H
#define NETWORK_MESH_ANALYTICAL_H

#include "network.h"
#include "core.h"

class NetworkMeshAnalytical : public Network
{
 public:
    NetworkMeshAnalytical(Core *core, int num_threads);
    virtual ~NetworkMeshAnalytical();

    virtual int netSend(NetPacket packet);
    virtual NetPacket netRecv(NetMatch match);
    virtual void outputSummary(ostream &out);

 protected:
    virtual UInt64 netProcCost(NetPacket packet);
    virtual UInt64 netLatency(NetPacket packet);

 private:
    void updateUtilization();
    static void receiveMCPUpdate(void *, NetPacket);

    // For statistical purposes
    unsigned int bytes_sent;
    unsigned int bytes_recv;

    UInt64 cycles_spent_proc;
    UInt64 cycles_spent_latency;
    UInt64 cycles_spent_contention;

    double global_utilization;
    UInt64 local_utilization_last_update;
    UInt64 local_utilization_flits_sent;
    UInt64 update_interval;
};

#endif
