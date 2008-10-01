#include "network_mesh_analytical.h"

using namespace std;

NetworkMeshAnalytical::NetworkMeshAnalytical(Chip *chip, int tid, int num_threads)
    :
        Network(chip,tid,num_threads),
        bytes_sent(0),
        cycles_spent_proc(0),
        cycles_spent_latency(0)
{
}

NetworkMeshAnalytical::~NetworkMeshAnalytical()
{
    cout << "  Network summary:" << endl;
    cout << "    bytes sent: " << bytes_sent << endl;
    cout << "    cycles spent proc: " << cycles_spent_proc << endl;
    cout << "    cycles spent latency: " << cycles_spent_latency << endl;
}

int NetworkMeshAnalytical::netSend(NetPacket packet)
{
    bytes_sent += packet.length;
    return Network::netSend(packet);
}

NetPacket NetworkMeshAnalytical::netRecv(NetMatch match)
{
    return Network::netRecv(match);
}

UINT64 NetworkMeshAnalytical::netProcCost(NetPacket packet)
{
    unsigned int cost = 10;
    cycles_spent_proc += cost;
    return cost;
}

UINT64 NetworkMeshAnalytical::netLatency(NetPacket packet)
{
    unsigned int latency = 30;
    cycles_spent_latency += latency;
    return latency;
}
