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
    virtual UINT64 netProcCost(NetPacket packet);
    virtual UINT64 netLatency(NetPacket packet);

 private:
    void updateUtilization();
    static void receiveMCPUpdate(void *, NetPacket);

    // For statistical purposes
    unsigned int bytes_sent;
    unsigned int bytes_recv;

    UINT64 cycles_spent_proc;
    UINT64 cycles_spent_latency;
    UINT64 cycles_spent_contention;

    double global_utilization;
    UINT64 local_utilization_last_update;
    UINT64 local_utilization_flits_sent;
    UINT64 update_interval;
};

#endif
