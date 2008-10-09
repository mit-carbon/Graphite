#include "network_mesh_analytical.h"
#include <math.h>

using namespace std;

NetworkMeshAnalytical::NetworkMeshAnalytical(Chip *chip, int tid, int num_threads, Core* the_core)
    :
        Network(chip,tid,num_threads, the_core),
        bytes_sent(0),
        cycles_spent_proc(0),
        cycles_spent_latency(0),
        cycles_spent_contention(0)
{
}

NetworkMeshAnalytical::~NetworkMeshAnalytical()
{
    cout << "  Network summary:" << endl;
    cout << "    bytes sent: " << bytes_sent << endl;
    cout << "    cycles spent proc: " << cycles_spent_proc << endl;
    cout << "    cycles spent latency: " << cycles_spent_latency << endl;
    cout << "    cycles spent contention: " << cycles_spent_contention << endl;
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
    UINT64 cost = 10;
    cycles_spent_proc += cost;
    return cost;
}

UINT64 NetworkMeshAnalytical::netLatency(NetPacket packet)
{
    // We model a unidirectional network with end-around connections
    // using the network model in "Limits on Interconnect Performance"
    // (by Anant). Currently, this ignores communication locality and
    // sets static values for all parameters. We also assume uniform
    // packet distribution, so we do not take the packet destination
    // into account.
    // 
    // We combine the contention model (eq. 12) with the model for
    // limited bisection width (sec. 4.2).

    const double Tw2 = 1;     // wire delay between adjacent nodes on mesh
    const double s = 1;       // switch delay, relative to Tw2
    const double N = 128;     // number of nodes in mesh
    const double n = 2;       // dimension of network
    const double W2 = 64;     // channel width constraint (constraint
      // is on bisection width, W2 * N held constant) (paper
      // normalized to unit-width channels, W2 denormalizes)
    
    // This lets us derive the latency, ignoring contention

    double k;                 // length of mesh in one dimension
    double kd;                // number of hops per dimension
    double time_per_hop;      // time spent in one hop through the network
    double W;                 // channel width
    double B;                 // number of flits for packet
    double hops_in_network;   // number of nodes visited
    double Tb;                // latency, without contention

    k = pow(N, 1/n);                   // pg 5
    kd = (k-1)/2;                      // pg 5 (note this will be
      // different for different network configurations...say,
      // bidirectional networks)
    time_per_hop = s + pow(k, n/2-1);  // pg 6
    W = W2 * k / 2;                    // pg 16 (bisection constraint)
    B = packet.length / W;
    hops_in_network = n * kd + B; // pg 5
    Tb = Tw2 * time_per_hop * hops_in_network; // pg 5

    // Now to model contention... (sec 3.2)

    const double p = 0.4;     // network utilization
    double w;                 // delay due to contention

    w  = p * B / (1 - p);
    w *= (kd-1)/(kd*kd);
    w *= 1+1/n;

    double hops_with_contention;
    double Tc;                // latency, with contention
    hops_with_contention = n * kd * (1 + w) + B;
    Tc = Tw2 * time_per_hop * hops_with_contention;

    // Computation finished...

    UINT64 Tci = (UINT64)(Tc + 0.5);
    cycles_spent_latency += Tci;
    cycles_spent_contention += (UINT64)(Tc - Tb);
    return Tci;
}
