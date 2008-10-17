#include "network_mesh_analytical.h"
#include <math.h>

using namespace std;

NetworkMeshAnalytical::NetworkMeshAnalytical(Chip *chip, int tid, int num_threads)
    :
        Network(chip,tid,num_threads),
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

    // TODO: Evaluate if bisection constraint is really the behavior
    // we want in general. (It changes the channel width from the
    // initial parameters and is only useful in comparing different
    // dimensions of networks with similar parameters.)

    // TODO: Fix (if necessary) distance calculation for uneven
    // topologies, i.e. 8-node 2d mesh or 10-node 3d mesh.

    // TODO: Confirm model for latency computed based on actual number
    // of network hops.

    const double Tw2 = 1;     // wire delay between adjacent nodes on mesh
    const double s = 1;       // switch delay, relative to Tw2
    const int n = 2;          // dimension of network
    const double W2 = 32;     // channel width constraint (constraint
      // is on bisection width, W2 * N held constant) (paper
      // normalized to unit-width channels, W2 denormalizes)
    const double p = 0.8;     // network utilization
    
    // This lets us derive the latency, ignoring contention

    int N;                    // number of nodes in the network
    double k;                 // length of mesh in one dimension
    double kd;                // number of hops per dimension
    double time_per_hop;      // time spent in one hop through the network
    double W;                 // channel width
    double B;                 // number of flits for packet
    double hops_in_network;   // number of nodes visited
    double Tb;                // latency, without contention

    N = g_chip->getNumModules();
    k = pow(N, 1./n);                  // pg 5
    kd = (k-1.)/2.;                    // pg 5 (note this will be
      // different for different network configurations...say,
      // bidirectional networks)
    time_per_hop = s + pow(k, n/2.-1.);  // pg 6
    W = W2 * k / 2.;                   // pg 16 (bisection constraint)
    B = ceil(packet.length * 8. / W);

    // Compute the number of hops based on src, dest
    // Based on this eqn:
    // node number = x1 + x2 k + x3 k^2 + ... + xn k^(n-1)
    int network_distance = 0;
    int src = packet.sender;
    int dest = packet.receiver;
    int ki = (int)k;
    for (int i = 0; i < n; i++)
      {
        div_t q1, q2;
        q1 = div(src, ki);
        q2 = div(dest, ki);
        // This models a unidirectional network distance
        // Should use abs() for bidirectional.
        if (q1.rem > q2.rem)
          network_distance += ki - (q1.rem - q2.rem);
        else
          network_distance += q2.rem - q1.rem;
        src = q1.quot;
        dest = q2.quot;
      }
    hops_in_network = network_distance + B; // B flits must be sent
      // (with wormhole routing, this adds B hops)
    
    Tb = Tw2 * time_per_hop * hops_in_network; // pg 5

    // Now to model contention... (sec 3.2)
    double w;                 // delay due to contention

    w  = p * B / (1. - p);
    w *= (kd-1.)/(kd*kd);
    w *= 1.+1./((double)n);

    if (w < 0) w = 0; // correct negative contention values for small
                      // networks

    double hops_with_contention;
    double Tc;                // latency, with contention
    hops_with_contention = network_distance * (1. + w) + B;
    Tc = Tw2 * time_per_hop * hops_with_contention;

    // Computation finished...

    UINT64 Tci = (UINT64)(ceil(Tc));
    cycles_spent_latency += Tci;
    cycles_spent_contention += (UINT64)(Tc - Tb);
    return Tci;
}
