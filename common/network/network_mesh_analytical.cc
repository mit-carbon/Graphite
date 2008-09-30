#include "network_mesh_analytical.h"

using namespace std;

NetworkMeshAnalytical::NetworkMeshAnalytical(Chip *chip, int tid, int num_threads)
    :
        Network(chip,tid,num_threads)
{
}

NetworkMeshAnalytical::~NetworkMeshAnalytical()
{
}

int NetworkMeshAnalytical::netSend(NetPacket packet)
{
    return Network::netSend(packet);
}

NetPacket NetworkMeshAnalytical::netRecv(NetMatch match)
{
    return Network::netRecv(match);
}

