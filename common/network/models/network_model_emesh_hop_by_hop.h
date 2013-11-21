#pragma once

#include <vector>
#include <iostream>
using std::vector;
using std::pair;
using std::ostream;

#include "network.h"
#include "network_model.h"
#include "fixed_types.h"
#include "queue_model.h"
#include "router_model.h"
#include "electrical_link_model.h"

class NetworkModelEMeshHopByHop : public NetworkModel
{
public:
   NetworkModelEMeshHopByHop(Network* net, SInt32 network_id);
   ~NetworkModelEMeshHopByHop();

   static bool isTileCountPermissible(SInt32 tile_count);
   static pair<bool,vector<tile_id_t> > computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 tile_count);
   static pair<bool,vector<Config::TileList> > computeProcessToTileMapping();

   void outputSummary(std::ostream &out, const Time& target_completion_time);

   // Energy computation
   void computeEnergy(const Time& curr_time);
   double getDynamicEnergy();
   double getStaticEnergy();

   static SInt32 computeDistance(tile_id_t sender, tile_id_t receiver);

private:
   enum NodeType
   {
      EMESH = 2 // Always Start at 2
   };

   enum OutputDirection
   {
      SELF = 0,
      LEFT,
      RIGHT,
      DOWN,
      UP
   };

   // Fields
   static bool _initialized;
   static SInt32 _mesh_width;
   static SInt32 _mesh_height;

   // Is contention model enabled?
   static bool _contention_model_enabled;

   // Injection Router 
   RouterModel* _injection_router;

   // Router & Link Parameters
   SInt32 _num_mesh_router_ports;
   RouterModel* _mesh_router;
   vector<ElectricalLinkModel*> _mesh_link_list;

   // Routing Function
   void routePacket(const NetPacket &pkt, queue<Hop> &next_hops);
  
   // DVFS 
   void setDVFS(double frequency, double voltage, const Time& curr_time);
   
   // Toplogy Params
   static void initializeEMeshTopologyParams();
   
   // Router & Link Models
   void createRouterAndLinkModels();
   void destroyRouterAndLinkModels();

   // Utilities
   static void computePosition(tile_id_t tile, SInt32 &x, SInt32 &y);
   static tile_id_t computeTileID(SInt32 x, SInt32 y);

   void outputPowerSummary(ostream& out, const Time& target_completion_time);
   void outputEventCountSummary(ostream& out);
   void outputContentionModelsSummary(ostream& out);
};
