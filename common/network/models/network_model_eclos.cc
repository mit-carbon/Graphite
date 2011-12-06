#include <algorithm>

#include "network_model_eclos.h"
#include "simulator.h"
#include "config.h"
#include "tile.h"
#include "log.h"

NetworkModelEClos::NetworkModelEClos(Network* network, SInt32 network_id):
   NetworkModel(network, network_id)
{
   readTopologyParams(_m, _n, _r);
   
   // Router, Link Params
   SInt32 num_flits_per_output_port = 0;
   string link_type;
   volatile double link_length;

   // Contention Model Params
   bool contention_model_enabled = false;
   string contention_model_type;

   try
   {
      _frequency = Sim()->getCfg()->getFloat("network/eclos/frequency");

      // FIXME: Change this once merged
      num_flits_per_output_port = Sim()->getCfg()->getInt("network/eclos/router/num_flits_per_port_buffer");
      _router_delay = Sim()->getCfg()->getInt("network/eclos/router/delay");
      _link_delay = Sim()->getCfg()->getInt("network/eclos/link/delay");
      link_type = Sim()->getCfg()->getString("network/eclos/link/type");
      link_length = Sim()->getCfg()->getFloat("network/eclos/link/length");
      _flit_width = Sim()->getCfg()->getInt("network/eclos/link/width");

      // Contention Model parameters
      contention_model_enabled = Sim()->getCfg()->getBool("network/eclos/queue_model/enabled");
      contention_model_type = Sim()->getCfg()->getString("network/eclos/queue_model/type");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Unable to read EClos network parameters from cfg file");
   }

   tile_id_t tile_id = getNetwork()->getTile()->getId(); 

   // Create the EClos router to tile mapping
   _eclos_router_to_tile_mapping.resize(NUM_ROUTER_STAGES);
   // Create the EClos Node Models
   _eclos_node_list.resize(NUM_ROUTER_STAGES);
   for (SInt32 i = 0; i < NUM_ROUTER_STAGES; i++)
      _eclos_node_list[i] = (EClosNode*) NULL;

   // INGRESS_ROUTER
   for (SInt32 i = 0; i < _n; i++)
   {
      _eclos_router_to_tile_mapping[INGRESS_ROUTER].push_back(i * _m);
   }
  
   if (find(_eclos_router_to_tile_mapping[INGRESS_ROUTER].begin(),
            _eclos_router_to_tile_mapping[INGRESS_ROUTER].end(),
            tile_id) != _eclos_router_to_tile_mapping[INGRESS_ROUTER].end())
   {
      _eclos_node_list[INGRESS_ROUTER] = new EClosNode(tile_id / _m,
            _m, _r,
            _flit_width, _frequency,
            num_flits_per_output_port, _router_delay,
            link_type, link_length, _link_delay,
            contention_model_enabled, contention_model_type);
   }

   // MIDDLE_ROUTER
   SInt32 N = Config::getSingleton()->getApplicationTiles();
   assert(N == (_m * _n));
   for (SInt32 i = 0; i < _r; i++)
   {
      _eclos_router_to_tile_mapping[MIDDLE_ROUTER].push_back( (i * (N/_r)) + 1);
   }

   if (find(_eclos_router_to_tile_mapping[MIDDLE_ROUTER].begin(),
            _eclos_router_to_tile_mapping[MIDDLE_ROUTER].end(),
            tile_id) != _eclos_router_to_tile_mapping[MIDDLE_ROUTER].end())
   {
      _eclos_node_list[MIDDLE_ROUTER] = new EClosNode((tile_id - 1) / (N/_r),
            _n, _n,
            _flit_width, _frequency,
            num_flits_per_output_port, _router_delay,
            link_type, link_length, _link_delay,
            contention_model_enabled, contention_model_type);
   }
   
   // EGRESS_ROUTER
   for (SInt32 i = 0; i < _n; i++)
   {
      _eclos_router_to_tile_mapping[EGRESS_ROUTER].push_back( (i * _m) + _m-1);
   }

   if (find(_eclos_router_to_tile_mapping[EGRESS_ROUTER].begin(),
            _eclos_router_to_tile_mapping[EGRESS_ROUTER].end(),
            tile_id) != _eclos_router_to_tile_mapping[EGRESS_ROUTER].end())
   {
      _eclos_node_list[EGRESS_ROUTER] = new EClosNode((tile_id - (_m-1)) / _m,
            _r, _m,
            _flit_width, _frequency,
            num_flits_per_output_port, _router_delay,
            link_type, link_length, _link_delay,
            contention_model_enabled, contention_model_type);
   }

   // Seed the buffer for random number generation
   srand48_r(tile_id, &_rand_data_buffer);
}

NetworkModelEClos::~NetworkModelEClos()
{
   for (SInt32 i = 0; i < NUM_ROUTER_STAGES; i++)
   {
      if (_eclos_node_list[i])
         delete _eclos_node_list[i];
   }
}

void
NetworkModelEClos::readTopologyParams(SInt32& m, SInt32& n, SInt32& r)
{
   try
   {
      m = Sim()->getCfg()->getInt("network/eclos/m");
      n = Sim()->getCfg()->getInt("network/eclos/n");
      r = Sim()->getCfg()->getInt("network/eclos/r");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read EClos Topology Params from cfg file");
   }
}

UInt32
NetworkModelEClos::computeAction(const NetPacket& pkt)
{
   return (pkt.specific == RECEIVING_CORE) ? RoutingAction::RECEIVE : RoutingAction::FORWARD;
}

void
NetworkModelEClos::routePacket(const NetPacket& pkt, vector<Hop>& next_hops)
{
   ScopedLock sl(_lock);

   Stage curr_stage = (pkt.specific == (UInt32) -1) ? (SENDING_CORE) : ((Stage) pkt.specific);

   Stage next_stage = NUM_STAGES;
   vector<pair<SInt32,UInt64> > next_dest_info_vec;

   SInt32 pkt_length = getNetwork()->getModeledLength(pkt);
   SInt32 num_flits = computeProcessingTime(pkt_length);

   switch (curr_stage)
   {
      case SENDING_CORE:
         {
            bool further_processing = processCornerCases(pkt, next_hops);
            if (further_processing)
            { 
               SInt32 ingress_router_idx = pkt.sender.tile_id / _m;
               next_stage = INGRESS_ROUTER;
               next_dest_info_vec.push_back(make_pair<SInt32,UInt64>(ingress_router_idx, _link_delay));
            }
         }
         break;

      case INGRESS_ROUTER:
         {
            SInt32 middle_router_idx = getRandNum(0, _r);
            next_stage = MIDDLE_ROUTER;

            EClosNode* eclos_node = _eclos_node_list[INGRESS_ROUTER];
            assert(eclos_node);
            eclos_node->process(pkt.time, num_flits, middle_router_idx, next_dest_info_vec);
         }
         break;

      case MIDDLE_ROUTER:
         {
            SInt32 egress_router_idx = (pkt.receiver.tile_id != NetPacket::BROADCAST) ? (pkt.receiver.tile_id / _m) : -1;
            next_stage = EGRESS_ROUTER;

            EClosNode* eclos_node = _eclos_node_list[MIDDLE_ROUTER];
            assert(eclos_node);
            eclos_node->process(pkt.time, num_flits, egress_router_idx, next_dest_info_vec);
         }
         break;

      case EGRESS_ROUTER:
         {
            SInt32 receiving_node_idx = (pkt.receiver.tile_id != NetPacket::BROADCAST) ? (pkt.receiver.tile_id % _m) : -1;
            next_stage = RECEIVING_CORE;
            
            EClosNode* eclos_node = _eclos_node_list[EGRESS_ROUTER];
            assert(eclos_node);
            eclos_node->process(pkt.time, num_flits, receiving_node_idx, next_dest_info_vec);
         }
         break;

      default:
         LOG_PRINT_ERROR("Cannot reach here");
         break;
   }
   
   vector<pair<SInt32,UInt64> >::iterator it = next_dest_info_vec.begin();
   for ( ; it != next_dest_info_vec.end(); it ++)
   {
      pair<SInt32,UInt64> next_dest_info = *it;
      SInt32 next_router_idx = next_dest_info.first;
      UInt64 packet_delay = next_dest_info.second;

      Hop hop;
      hop.final_dest.tile_id = pkt.receiver.tile_id;
      if (next_stage == RECEIVING_CORE)
      {
         SInt32 curr_router_idx = _eclos_node_list[EGRESS_ROUTER]->getRouterIdx();
         hop.next_dest.tile_id = (curr_router_idx * _m) + next_router_idx;
      }
      else
      {
         hop.next_dest.tile_id = _eclos_router_to_tile_mapping[next_stage][next_router_idx];
      }
      hop.time = pkt.time + packet_delay;
      hop.specific = next_stage;
      next_hops.push_back(hop);
   }

   LOG_PRINT("Sender(%i), Curr Stage(%s), Curr Tile(%i), Time(%llu) -->",
         pkt.sender.tile_id, getName(curr_stage).c_str(), getNetwork()->getTile()->getId(), pkt.time);
   for (UInt32 i = 0; i < next_hops.size(); i++)
   {
      LOG_PRINT("    --> Receiver(%i), Next Stage(%s), Next Tile(%i), Time(%llu)",
            next_hops[i].final_dest.tile_id, getName((Stage) next_hops[i].specific).c_str(),
            next_hops[i].next_dest.tile_id, next_hops[i].time);
   }

}

string
NetworkModelEClos::getName(Stage stage)
{
   switch (stage)
   {
      case SENDING_CORE:
         return "SENDING_CORE";
      case INGRESS_ROUTER:
         return "INGRESS_ROUTER";
      case MIDDLE_ROUTER:
         return "MIDDLE_ROUTER";
      case EGRESS_ROUTER:
         return "EGRESS_ROUTER";
      case RECEIVING_CORE:
         return "RECEIVING_CORE";
      default:
         LOG_PRINT_ERROR("Must be some stage");
         return "";
   }
}

void
NetworkModelEClos::processReceivedPacket(NetPacket& pkt)
{
   ScopedLock sl(_lock);

   pair<bool,bool> modeled = isModeled(pkt);
   if ( (modeled.first == true) || 
        ((modeled.second == true) && isApplicationTile(getNetwork()->getTile()->getId())) )
   {
      SInt32 pkt_length = getNetwork()->getModeledLength(pkt);
      SInt32 num_flits = computeProcessingTime(pkt_length);

      UInt64 serialization_delay = num_flits - 1;
      pkt.time += serialization_delay;
      UInt64 zero_load_delay = serialization_delay + 
         NUM_ROUTER_STAGES * _router_delay + 
         (NUM_ROUTER_STAGES + 1) * _link_delay;

      // Update Receive Counters
      updateReceiveCounters(pkt, zero_load_delay);
   }
}

bool
NetworkModelEClos::processCornerCases(const NetPacket& pkt, vector<Hop>& next_hops)
{
   assert(pkt.specific == (UInt32) -1);
  
   pair<bool,bool> modeled = isModeled(pkt); 
   if (modeled.first == true)
   {
      return true;
   }
   else if (modeled.second == true)
   {
      assert(pkt.receiver.tile_id == NetPacket::BROADCAST);
      for (UInt32 i = Config::getSingleton()->getApplicationTiles();
            i < Config::getSingleton()->getTotalTiles(); i++)
      {
         Hop hop;
         hop.next_dest.tile_id = i;
         hop.final_dest.tile_id = NetPacket::BROADCAST;
         hop.specific = RECEIVING_CORE;
         hop.time = pkt.time;
         next_hops.push_back(hop);
      }
      return true;
   }
   else // both elements are false
   {
      if (pkt.receiver.tile_id == NetPacket::BROADCAST)
      {
         for (UInt32 i = 0; i < Config::getSingleton()->getTotalTiles(); i++)
         {
            Hop hop;
            hop.next_dest.tile_id = i;
            hop.final_dest.tile_id = NetPacket::BROADCAST;
            hop.specific = RECEIVING_CORE;
            hop.time = pkt.time;
            next_hops.push_back(hop);
         }
      }
      else // (pkt.receiver.tile_id != NetPacket::BROADCAST)
      {
         Hop hop;
         hop.next_dest.tile_id = pkt.receiver.tile_id;
         hop.final_dest.tile_id = pkt.receiver.tile_id;
         hop.specific = RECEIVING_CORE;
         hop.time = pkt.time;
         next_hops.push_back(hop);
      }
      return false;
   }
}

pair<bool,bool>
NetworkModelEClos::isModeled(const NetPacket& pkt)
{
   // Get the requester first
   tile_id_t requester = getRequester(pkt);

   if (TILE_ID(pkt.sender) == TILE_ID(pkt.receiver))
   {
      return make_pair<bool,bool>(false,false);
   }
   else if (isEnabled() && isApplicationTile(requester) && isApplicationTile(TILE_ID(pkt.sender)))
   {
      if (isApplicationTile(TILE_ID(pkt.receiver)))
         return make_pair<bool,bool>(true, false);
      else if (TILE_ID(pkt.receiver) == NetPacket::BROADCAST)
         return make_pair<bool,bool>(false, true);
      else
         return make_pair<bool,bool>(false,false);
   }
   else
   {
      return make_pair<bool,bool>(false,false);
   }
}

bool
NetworkModelEClos::isApplicationTile(tile_id_t tile_id)
{
   return ((tile_id >= 0) && (tile_id < (tile_id_t) Config::getSingleton()->getApplicationTiles()));
}

SInt32
NetworkModelEClos::getRandNum(SInt32 start, SInt32 end)
{
   double result;
   drand48_r(&_rand_data_buffer, &result);
   return (SInt32) (start + result * (end - start));
}

SInt32
NetworkModelEClos::computeProcessingTime(SInt32 pkt_length)
{
   SInt32 num_bits = pkt_length * 8;
   return ((num_bits % _flit_width) == 0) ? 
      (num_bits / _flit_width) : (num_bits / _flit_width) + 1;
}

pair<bool,SInt32>
NetworkModelEClos::computeTileCountConstraints(SInt32 tile_count)
{
   // Check for m,n,r correctly
   SInt32 N = Config::getSingleton()->getApplicationTiles();
   SInt32 m,n,r;
   readTopologyParams(m,n,r);
   if ((N != (m * n)) || (N < r))
   {
      fprintf(stderr, "[[ N(%i) != m(%i) * n(%i) ]] OR [[ N(%i) < r(%i) ]]\n",
            N, m, n, N, r);
      exit(-1);
   }
   return make_pair<bool,SInt32>(false,N);
}

void
NetworkModelEClos::outputSummary(ostream& out)
{
   out << "   EClos Network: " << endl;
   NetworkModel::outputSummary(out);

   out << "    Ingress Router: " << endl;
   if (_eclos_node_list[INGRESS_ROUTER])
      _eclos_node_list[INGRESS_ROUTER]->outputSummary(out);
   else
      EClosNode::dummyOutputSummary(out);

   out << "    Middle Router: " << endl;
   if (_eclos_node_list[MIDDLE_ROUTER])
      _eclos_node_list[MIDDLE_ROUTER]->outputSummary(out);
   else
      EClosNode::dummyOutputSummary(out);

   out << "    Egress Router: " << endl;
   if (_eclos_node_list[EGRESS_ROUTER])
      _eclos_node_list[EGRESS_ROUTER]->outputSummary(out);
   else
      EClosNode::dummyOutputSummary(out);
}

NetworkModelEClos::EClosNode::EClosNode(SInt32 router_idx,
      SInt32 input_ports, SInt32 output_ports,
      SInt32 flit_width, volatile float frequency,
      SInt32 num_flits_per_output_port, UInt64 router_delay,
      string link_type, volatile double link_length, UInt64 link_delay,
      bool contention_model_enabled, string contention_model_type)
   : _router_idx(router_idx)
   , _input_ports(input_ports)
   , _output_ports(output_ports)
   , _flit_width(flit_width)
   , _contention_model_enabled(contention_model_enabled)
   , _router_delay(router_delay)
   , _link_delay(link_delay)
{
   if (_contention_model_enabled)
   {
      for (SInt32 i = 0; i < output_ports; i++)
         _contention_models.push_back(QueueModel::create(contention_model_type, 1));
   }

   _router_model = ElectricalNetworkRouterModel::create(input_ports, output_ports, 
         num_flits_per_output_port, flit_width);
   _link_model = ElectricalNetworkLinkModel::create(link_type, frequency, link_length, flit_width);

   // Initialize Event Counters
   _switch_allocator_arbitrates = 0;
   _crossbar_traversals_unicast = 0;
   _crossbar_traversals_broadcast = 0;
   _buffer_reads = 0;
   _buffer_writes = 0;
   _output_link_traversals = 0;
}

NetworkModelEClos::EClosNode::~EClosNode()
{
   delete _link_model;
   delete _router_model;
   if (_contention_model_enabled)
   {
      for (SInt32 i = 0; i < _output_ports; i++)
         delete _contention_models[i];
   }
}

void
NetworkModelEClos::EClosNode::process(UInt64 pkt_time, SInt32 num_flits, SInt32 output_link_id,
      vector<pair<SInt32,UInt64> >& next_dest_info_vec)
{
   SInt32 num_output_links = (output_link_id == -1) ? _output_ports : 1;

   if (output_link_id != -1)
   {
      assert((output_link_id >= 0) && (output_link_id < _output_ports));
      // Calculate Output Link Contention
      UInt64 contention_delay = _contention_model_enabled ? 
         _contention_models[output_link_id]->computeQueueDelay(pkt_time, num_flits) : 0;
      
      next_dest_info_vec.push_back(make_pair<SInt32,UInt64>(output_link_id,
               contention_delay + _router_delay + _link_delay));
   }
   else
   {
      for (SInt32 i = 0; i < _output_ports; i++)
      {
         UInt64 contention_delay = _contention_model_enabled ? 
            _contention_models[i]->computeQueueDelay(pkt_time, num_flits) : 0;
         
         next_dest_info_vec.push_back(make_pair<SInt32,UInt64>(i,
                  contention_delay + _router_delay + _link_delay));
      }
   }
   
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      // Update Dynamic Energy Tracked by power models
      UInt32 contention = _input_ports/2;
      _router_model->updateDynamicEnergySwitchAllocator(contention);
      _router_model->updateDynamicEnergyClock();
      
      // FIXME: This is not entirely correct (Check if unicast/broadcast)
      _router_model->updateDynamicEnergyCrossbar(_flit_width/2, num_flits * num_output_links);
      _router_model->updateDynamicEnergyClock(num_flits * num_output_links);
      
      _router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::WRITE,
            _flit_width/2, num_flits * num_output_links);
      _router_model->updateDynamicEnergyClock(num_flits * num_output_links);

      _router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::READ,
            _flit_width/2, num_flits * num_output_links);
      _router_model->updateDynamicEnergyClock(num_flits * num_output_links);
      
      _link_model->updateDynamicEnergy(_flit_width/2, num_flits * num_output_links);
   }

   _switch_allocator_arbitrates ++;
   _crossbar_traversals_unicast += (num_output_links == 1) ? num_flits : 0;
   _crossbar_traversals_broadcast += (num_output_links == _output_ports) ? num_flits : 0;
   _buffer_writes += (num_output_links * num_flits);
   _buffer_reads += (num_output_links * num_flits);
   _output_link_traversals += (num_output_links * num_flits);
}

void
NetworkModelEClos::EClosNode::outputSummary(ostream& out)
{
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      out << "      Energy Counters: " << endl;
      out << "        Router Static Power (in W): " << _router_model->getTotalStaticPower() << endl;
      out << "        Router Dynamic Energy (in J): " << _router_model->getTotalDynamicEnergy() << endl;
      out << "        Link Static Power (in W): " << _link_model->getStaticPower() * _output_ports << endl;
      out << "        Link Dynamic Energy (in J): " << _link_model->getDynamicEnergy() << endl;
   }
   out << "      Event Counters: " << endl;
   out << "        Switch Allocator Requests: " << _switch_allocator_arbitrates << endl;
   out << "        Crossbar Traversals (Unicast): " << _crossbar_traversals_unicast << endl;
   out << "        Crossbar Traversals (Broadcast): " << _crossbar_traversals_broadcast << endl;
   out << "        Buffer Writes: " << _buffer_writes << endl;
   out << "        Buffer Reads: " << _buffer_reads << endl;
   out << "        Output Link Traversals: " << _output_link_traversals << endl;
}

void
NetworkModelEClos::EClosNode::dummyOutputSummary(ostream& out)
{
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      out << "    Energy Counters: " << endl;
      out << "      Router Static Power (in W): " << endl;
      out << "      Router Dynamic Energy (in J): " << endl;
      out << "      Link Static Power (in W): " <<  endl;
      out << "      Link Dynamic Energy (in J): " << endl;
   }
   out << "    Event Counters: " << endl;
   out << "      Switch Allocator Requests: " << endl;
   out << "      Crossbar Traversals (Unicast): " << endl;
   out << "      Crossbar Traversals (Broadcast): " << endl;
   out << "      Buffer Writes: " << endl;
   out << "      Buffer Reads: " << endl;
   out << "      Output Link Traversals: " << endl;
}
