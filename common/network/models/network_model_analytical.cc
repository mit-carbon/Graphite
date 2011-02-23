#include <math.h>
#include <stdlib.h>

#include "simulator.h"
#include "network_model_analytical.h"
#include "network_model_analytical_params.h"
#include "message_types.h"
#include "config.h"
#include "tile.h"
#include "core_perf_model.h"
#include "transport.h"
#include "lock.h"
#include "clock_converter.h"
#include "log.h"

#define IS_NAN(x) (!((x < 0.0) || (x >= 0.0)))

#define GET_INT(s) ( Sim()->getCfg()->getInt("network/analytical/" s) )

using namespace std;

NetworkModelAnalytical::NetworkModelAnalytical(Network *net, SInt32 network_id)
      : NetworkModel(net, network_id)
      , _bytesSent(0)
      , _cyclesProc(0)
      , _cyclesLatency(0)
      , _cyclesContention(0)
      , _globalUtilization(0)
      , _localUtilizationLastUpdate(0)
      , _localUtilizationFlitsSent(0)
      , m_enabled(false)
{
   getNetwork()->registerCallback(MCP_UTILIZATION_UPDATE_TYPE,
                                  receiveMCPUpdate,
                                  this);

   try
   {
      // Create network parameters
      _frequency = Sim()->getCfg()->getFloat("network/analytical/frequency");
      _params.Tw2 = GET_INT("Tw2"); // single cycle between nodes in 2d mesh
      _params.s = GET_INT("s"); // single cycle switching time
      _params.n = GET_INT("n"); // 2-d mesh network
      _params.W = GET_INT("W"); // 32-bit wide channels
      _params.update_interval = GET_INT("update_interval");
      _params.proc_cost = ((network_id == STATIC_NETWORK_MEMORY_1) || (network_id == STATIC_NETWORK_MEMORY_2)) ? 0 : GET_INT("processing_cost");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Some analytical network parameters not available.");
   }
}

NetworkModelAnalytical::~NetworkModelAnalytical()
{
   getNetwork()->unregisterCallback(MCP_UTILIZATION_UPDATE_TYPE);
}

UInt32 NetworkModelAnalytical::computeAction(const NetPacket& pkt)
{
   return RoutingAction::RECEIVE;
}

void NetworkModelAnalytical::routePacket(const NetPacket &pkt,
      std::vector<Hop> &nextHops)
{
   ScopedLock sl(_lock);

   // basic magic network routing, with two additions
   // (1) compute latency of packet
   // (2) update utilization

   CorePerfModel *perf = getNetwork()->getTile()->getCurrentCore()->getPerformanceModel();

   Hop h;
   h.final_dest = pkt.receiver;
   h.next_dest = pkt.receiver;

   UInt64 network_latency = computeLatency(pkt);
   h.time = pkt.time + network_latency;

   nextHops.push_back(h);

   if (_params.proc_cost > 0)
      //perf->queueDynamicInstruction(new DynamicInstruction(_params.proc_cost));
      perf->queueDynamicInstruction(new DynamicInstruction(0));
   _cyclesProc += _params.proc_cost;

   updateUtilization();

   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
   _bytesSent += pkt_length;
}

UInt64 NetworkModelAnalytical::computeLatency(const NetPacket &packet)
{
   if (!m_enabled)
      return 0;

   // self-sends incur no cost
   if (packet.sender.tile_id == packet.receiver.tile_id)
     return 0;

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
   double Tw2 = _params.Tw2;
   double s = _params.s;
   int n = _params.n;
   double W = _params.W;
   double p = _globalUtilization;
   LOG_ASSERT_ERROR(!IS_NAN(_globalUtilization) && 0 <= _globalUtilization && _globalUtilization < 1, "Recv'd invalid global utilization value: %f", _globalUtilization);

   // This lets us derive the latency, ignoring contention

   int N;                    // number of nodes in the network
   double k;                 // length of mesh in one dimension
   double kd;                // number of hops per dimension
   double time_per_hop;      // time spent in one hop through the network
   double B;                 // number of flits for packet
   double hops_in_network;   // number of nodes visited
   double Tb;                // latency, without contention

   N = Config::getSingleton()->getTotalTiles();
   k = pow(N, 1./n);                  // pg 5
   kd = k/2.;                         // pg 5 (note this will be
   // different for different network configurations...say,
   // bidirectional networks)
   time_per_hop = s + pow(k, n/2.-1.);  // pg 6
   
   UInt32 packet_length = getNetwork()->getModeledLength(packet);
   B = ceil(packet_length * 8. / W);

   // Compute the number of hops based on src, dest
   // Based on this eqn:
   // node number = x1 + x2 k + x3 k^2 + ... + xn k^(n-1)
   int network_distance = 0;
   int src = packet.sender.tile_id;
   int dest = packet.receiver.tile_id;
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
   UInt64 Tci = (UInt64)(ceil(Tc));
   _cyclesLatency += Tci;
   _cyclesContention += (UInt64)(Tc - Tb);

   // ** update utilization counter **
   // we must account for the usage throughout the mesh
   // which means that we must include the # of hops
   _localUtilizationFlitsSent += (UInt64)(B * hops_in_network);

   return Tci;
}

void NetworkModelAnalytical::outputSummary(ostream &out)
{
   out << "    bytes sent: " << _bytesSent << endl;
   out << "    cycles spent proc: " << _cyclesProc << endl;
   
   UInt64 _cyclesLatencyInNS = convertCycleCount(_cyclesLatency, _frequency, 1.0);
   UInt64 _cyclesContentionInNS = convertCycleCount(_cyclesContention, _frequency, 1.0);
   out << "    cycles spent latency (in clock cycles): " << _cyclesLatency << endl;
   out << "    cycles spent latency (in ns): " << _cyclesLatencyInNS << endl;
   out << "    cycles spent contention (in clock cycles): " << _cyclesContention << endl;
   out << "    cycles spent contention (in ns): " << _cyclesContentionInNS << endl;
}

struct UtilizationMessage
{
   double ut;
   NetworkModelAnalytical *model;
};

struct UtilizationMCPMessage
{
   int msg_type;
   UtilizationMessage msg;
};

// we send and receive updates asynchronously for performance and
// because the MCP is unable to reply to itself.

void NetworkModelAnalytical::updateUtilization()
{
   // ** send updates

   // don't lock because this is all approximate anyway
   UInt64 tile_time = getNetwork()->getTile()->getCore()->getPerformanceModel()->getCycleCount();
   UInt64 elapsed_time = tile_time - _localUtilizationLastUpdate;

   if (elapsed_time < _params.update_interval)
      return;

   // FIXME: This assumes one cycle per flit, might not be accurate.
   double local_utilization = ((double)_localUtilizationFlitsSent) / ((double)elapsed_time);

   LOG_ASSERT_WARNING(0 <= local_utilization && local_utilization < 1, "Unusual local utilization value: %f", local_utilization);

   _localUtilizationLastUpdate = tile_time;
   _localUtilizationFlitsSent = 0;

   // build packet
   UtilizationMCPMessage m;
   m.msg_type = MCP_MESSAGE_UTILIZATION_UPDATE;
   m.msg.ut = local_utilization;
   m.msg.model = this;

   NetPacket update;
   update.sender.tile_id = getNetwork()->getTile()->getId();
   update.receiver.tile_id = Config::getSingleton()->getMCPTileNum();
   update.length = sizeof(m);
   update.type = MCP_SYSTEM_TYPE;
   update.data = &m;

//   getNetwork()->netSend(update);
}

void NetworkModelAnalytical::receiveMCPUpdate(void *obj, NetPacket response)
{
   assert(response.length == sizeof(UtilizationMessage));

   UtilizationMessage *pr = (UtilizationMessage*) response.data;

   ScopedLock sl(pr->model->_lock);

   LOG_ASSERT_ERROR(!IS_NAN(pr->ut) && 0 <= pr->ut && pr->ut < 1, "Recv'd invalid global utilization value: %f", pr->ut);
//   fprintf(stderr, "Recv'd global utilization: %f\n", pr->ut);

   pr->model->_globalUtilization = pr->ut;
}

void NetworkModelAnalytical::enable()
{
   m_enabled = true;
}

void NetworkModelAnalytical::disable()
{
   m_enabled = false;
}
