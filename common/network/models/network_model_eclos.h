#pragma once

#include <stdlib.h>
#include <vector>
using namespace std;

#include "lock.h"
#include "network.h"
#include "network_model.h"
#include "queue_model.h"
#include "electrical_network_router_model.h"
#include "electrical_network_link_model.h"

class NetworkModelEClos : public NetworkModel
{
   public:
      enum Stage 
      {
         INGRESS_ROUTER = 0,
         MIDDLE_ROUTER,
         EGRESS_ROUTER,
         NUM_ROUTER_STAGES,
         SENDING_CORE = 3,
         RECEIVING_CORE,
         NUM_STAGES,
      };

      NetworkModelEClos(Network* network, SInt32 network_id);
      ~NetworkModelEClos();

      volatile float getFrequency() { return _frequency; }

      UInt32 computeAction(const NetPacket& pkt);
      void routePacket(const NetPacket& pkt, vector<Hop>& next_hops);
      void processReceivedPacket(NetPacket& pkt);

      void outputSummary(ostream& out);

      void enable() { _enabled = true; }
      void disable() { _enabled = false; }
      void reset() {}

      static pair<bool,SInt32> computeTileCountConstraints(SInt32 tile_count);

   private:
      class EClosNode
      {
         public:
            EClosNode(SInt32 router_idx,
                  SInt32 input_ports, SInt32 output_ports,
                  SInt32 flit_width, volatile float frequency,
                  SInt32 num_flits_per_output_port, UInt64 router_delay,
                  string link_type, volatile double link_length, UInt64 link_delay,
                  bool contention_model_enabled, string contention_model_type);
            ~EClosNode();

            SInt32 getRouterIdx() { return _router_idx; }
            void process(UInt64 pkt_time, SInt32 num_flits, SInt32 output_link_id,
                  vector<pair<SInt32,UInt64> >& next_dest_info_vec);
            void outputSummary(ostream& out);
            static void dummyOutputSummary(ostream& out);

         private:
            SInt32 _router_idx;
            SInt32 _input_ports;
            SInt32 _output_ports;
            SInt32 _flit_width;
            
            // Router (Output Link) Contention Models
            bool _contention_model_enabled;
            vector<QueueModel*> _contention_models;

            // Router and Link Delays
            UInt64 _router_delay;
            UInt64 _link_delay;

            // Router and Link (Power) Models
            ElectricalNetworkRouterModel* _router_model;
            ElectricalNetworkLinkModel* _link_model;
            
            // Event Counters
            UInt64 _switch_allocator_arbitrates;
            UInt64 _crossbar_traversals_unicast;
            UInt64 _crossbar_traversals_broadcast;
            UInt64 _buffer_reads;
            UInt64 _buffer_writes;
            UInt64 _output_link_traversals;
      };

      // m x n x r Clos network
      SInt32 _m;
      SInt32 _n;
      SInt32 _r;

      vector<vector<tile_id_t> > _eclos_router_to_tile_mapping;
      vector<EClosNode*> _eclos_node_list;

      volatile float _frequency;
      SInt32 _flit_width;

      UInt64 _router_delay;
      UInt64 _link_delay;

      bool _enabled;

      Lock _lock;

      // Rand Data Buffer
      drand48_data _rand_data_buffer;

      // Private Functions
      bool processCornerCases(const NetPacket& pkt, vector<Hop>& next_hops);
      static void readTopologyParams(SInt32& m, SInt32& n, SInt32& r);
      SInt32 getRandNum(SInt32 start, SInt32 end);
      SInt32 computeProcessingTime(SInt32 pkt_length);
      bool isApplicationTile(tile_id_t tile_id);
      pair<bool,bool> isModeled(const NetPacket& pkt);
      string getName(Stage stage);
};
