#include "network_mesh_analytical.h"

using namespace std;

NetworkMeshAnalytical::NetworkMeshAnalytical(Chip *chip, int tid, int num_threads)
    :
        Network(chip,tid,num_threads),
        bytes_sent(0)
{
}

NetworkMeshAnalytical::~NetworkMeshAnalytical()
{
    cout << " *** Network summary ***" << endl;
    cout << "   bytes_sent: " << bytes_sent << endl;
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

