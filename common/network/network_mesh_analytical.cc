#include "network_mesh_analytical.h"
#include "network_mesh_analytical_params.h"
#include "message_types.h"
#include "chip.h"
#include <math.h>

using namespace std;

//NetworkMeshAnalytical::NetworkMeshAnalytical(int tid, int num_threads, Core *core)
NetworkMeshAnalytical::NetworkMeshAnalytical(Core *core, int num_threads)
    :
        Network(core, num_threads),
        bytes_sent(0),
        cycles_spent_proc(0),
        cycles_spent_latency(0),
        cycles_spent_contention(0),
        global_utilization(0.0),
        local_utilization_last_update(0),
        local_utilization_flits_sent(0),
        update_interval(0)
{
   registerCallback(MCP_UTILIZATION_UPDATE_TYPE,
                    receiveMCPUpdate,
                    this);

   update_interval = g_config->getAnalyticNetworkParms()->update_interval;
}

NetworkMeshAnalytical::~NetworkMeshAnalytical()
{
   unregisterCallback(MCP_UTILIZATION_UPDATE_TYPE);
}

int NetworkMeshAnalytical::netSend(NetPacket packet)
{
    bytes_sent += packet.length;
    updateUtilization();
    return Network::netSend(packet);
}

NetPacket NetworkMeshAnalytical::netRecv(NetMatch match)
{
    updateUtilization();
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

    // TODO: Fix (if necessary) distance calculation for uneven
    // topologies, i.e. 8-node 2d mesh or 10-node 3d mesh.

    // TODO: Confirm model for latency computed based on actual number
    // of network hops.

    // Retrieve parameters
    const NetworkMeshAnalyticalParameters *pParams = g_config->getAnalyticNetworkParms();
    double Tw2 = pParams->Tw2;
    double s = pParams->s;
    int n = pParams->n;
    double W = pParams->W;
    double p = global_utilization;

    // This lets us derive the latency, ignoring contention

    int N;                    // number of nodes in the network
    double k;                 // length of mesh in one dimension
    double kd;                // number of hops per dimension
    double time_per_hop;      // time spent in one hop through the network
    double B;                 // number of flits for packet
    double hops_in_network;   // number of nodes visited
    double Tb;                // latency, without contention

    N = g_chip->getNumModules();
    k = pow(N, 1./n);                  // pg 5
    kd = (k-1.)/2.;                    // pg 5 (note this will be
      // different for different network configurations...say,
      // bidirectional networks)
    time_per_hop = s + pow(k, n/2.-1.);  // pg 6
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

    // ** update utilization counter **
    // we must account for the usage throughout the mesh
    // which means that we must include the # of hops
    local_utilization_flits_sent += (UINT64)(B * hops_in_network);

    return Tci;
}

void NetworkMeshAnalytical::outputSummary(ostream &out)
{
   out << "  Network summary:" << endl;
   out << "    bytes sent: " << bytes_sent << endl;
   out << "    cycles spent proc: " << cycles_spent_proc << endl;
   out << "    cycles spent latency: " << cycles_spent_latency << endl;
   out << "    cycles spent contention: " << cycles_spent_contention << endl;
}

struct UtilizationMessage
{
  int msg;
  int comm_id;
  double ut;
};

// we send and receive updates asynchronously for performance and
// because the MCP is unable to reply to itself.

void NetworkMeshAnalytical::updateUtilization()
{
  // make sure the system is properly initialized
  int rank;
  chipRank(&rank);
  if (rank < 0)
    return;

  // ** send updates

  // don't lock because this is all approximate anyway
  UINT64 core_time = the_core->getPerfModel()->getCycleCount();
  UINT64 elapsed_time = core_time - local_utilization_last_update;

  if (elapsed_time < update_interval)
    return;

  // FIXME: This assumes one cycle per flit, might not be accurate.
  double local_utilization = ((double)local_utilization_flits_sent) / ((double)elapsed_time);

  local_utilization_last_update = core_time;
  local_utilization_flits_sent = 0;

  // build packet
  UtilizationMessage m;
  m.msg = MCP_MESSAGE_UTILIZATION_UPDATE;
  m.comm_id = netCommID();
  assert(0 <= m.comm_id && (unsigned int)m.comm_id < g_config->numMods());
  m.ut = local_utilization;

  NetPacket update;
  update.sender = netCommID();
  update.receiver = g_config->MCPCommID();
  update.length = sizeof(m);
  update.type = MCP_REQUEST_TYPE;
  update.data = (char*) &m;

  netSendMagic(update);
}

void NetworkMeshAnalytical::receiveMCPUpdate(void *obj, NetPacket response)
{
   NetworkMeshAnalytical *net = (NetworkMeshAnalytical*) obj;

   assert(response.length == sizeof(double));
   //   assert(0 <= net->global_utilization || net->global_utilization <= 1);

   net->global_utilization = *((double*)response.data);
}
