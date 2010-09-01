#include <iostream>
#include <fstream>
#include <cassert>
#include <stdlib.h>
using namespace std;

#include "carbon_user.h"
#include "electrical_network_router_model.h"
#include "electrical_network_link_model.h"
#include "optical_network_link_model.h"
#include "fixed_types.h"

int main(int argc, char *argv[])
{
   CarbonStartSim(argc, argv);

   // Compare Electrical and Optical power models
   // First Electrical
   string network = string(argv[1]);
   ifstream activity_counters_file(argv[2]);
   FILE* energy_out = fopen(argv[3], "w");

   if (network == "emesh")
   {
      UInt32 num_cores = 1024;
      UInt32 num_electrical_router_ports = 5;
      UInt32 electrical_link_width = 256;
      
      double total_switch_allocator_traversals;
      double total_crossbar_traversals;
      double total_buffer_accesses;
      double total_link_traversals;

      double completion_time_emesh;
      activity_counters_file >> completion_time_emesh;

      activity_counters_file >> total_switch_allocator_traversals;
      activity_counters_file >> total_crossbar_traversals;
      activity_counters_file >> total_buffer_accesses;
      activity_counters_file >> total_link_traversals;

      ElectricalNetworkRouterModel* m_electrical_router_model = ElectricalNetworkRouterModel::create(num_electrical_router_ports, num_electrical_router_ports, 4, electrical_link_width);
      ElectricalNetworkLinkModel* m_electrical_link_model = ElectricalNetworkLinkModel::create("electrical_repeated", 1.0, 1.0, electrical_link_width, true);

      m_electrical_router_model->updateDynamicEnergySwitchAllocator(num_electrical_router_ports/2);
      m_electrical_router_model->updateDynamicEnergyCrossbar(electrical_link_width/2);
      m_electrical_router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::WRITE, electrical_link_width/2);
      m_electrical_router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::READ, electrical_link_width/2);
      m_electrical_router_model->updateDynamicEnergyClock();
      m_electrical_link_model->updateDynamicEnergy(electrical_link_width/2);

      double total_dynamic_energy_emesh_router = (m_electrical_router_model->getDynamicEnergySwitchAllocator() * total_switch_allocator_traversals) + \
                                          (m_electrical_router_model->getDynamicEnergyCrossbar() * total_crossbar_traversals) + \
                                          (m_electrical_router_model->getDynamicEnergyBuffer() * total_buffer_accesses) + \
                                          (m_electrical_router_model->getDynamicEnergyClock() * (total_switch_allocator_traversals + total_crossbar_traversals + 2 * total_buffer_accesses));
      double total_dynamic_energy_emesh_link = m_electrical_link_model->getDynamicEnergy() * total_link_traversals;
      double total_dynamic_energy_emesh = total_dynamic_energy_emesh_router + total_dynamic_energy_emesh_link;

      double total_static_power_emesh_router = num_cores * \
                                        ( \
                                           m_electrical_router_model->getTotalStaticPower() \
                                        );
      double total_static_power_emesh_link = num_cores * \
                                        ( \
                                           m_electrical_link_model->getStaticPower() * (num_electrical_router_ports - 1) \
                                        );
      double total_static_power_emesh = total_static_power_emesh_router + total_static_power_emesh_link;
      
      printf("Static Energy(EMesh) = %g, Dynamic Energy(EMesh) = %g\n", \
            total_static_power_emesh * completion_time_emesh / 1e9, total_dynamic_energy_emesh);
      
      fprintf(energy_out, "%g\n%g\n%g\n%g\n",
            total_static_power_emesh_router * completion_time_emesh / 1e9,
            total_dynamic_energy_emesh_router,
            total_static_power_emesh_link * completion_time_emesh / 1e9,
            total_dynamic_energy_emesh_link);
   }

   else if (network == "anet")
   {
      // Next Optical
      UInt32 num_clusters = 64;
      UInt32 cluster_size = 16;
      UInt32 num_scatter_networks_per_cluster = 2;
      UInt32 num_gather_network_router_ports = 5;

      UInt32 gather_network_link_width = 128;
      UInt32 optical_network_link_width = 128;
      UInt32 scatter_network_link_width = 128;
      
      double total_gather_network_switch_allocator_traversals;
      double total_gather_network_crossbar_traversals;
      double total_gather_network_link_traversals;
      double total_optical_network_link_traversals;
      double total_scatter_network_switch_allocator_traversals;
      double total_scatter_network_buffer_accesses;
      double total_scatter_network_crossbar_traversals;
      double total_scatter_network_link_traversals;

      double completion_time_anet;
      activity_counters_file >> completion_time_anet;
      
      activity_counters_file >> total_gather_network_switch_allocator_traversals;
      activity_counters_file >> total_gather_network_crossbar_traversals;
      activity_counters_file >> total_gather_network_link_traversals;
      activity_counters_file >> total_optical_network_link_traversals;
      activity_counters_file >> total_scatter_network_switch_allocator_traversals;
      activity_counters_file >> total_scatter_network_link_traversals;

      total_scatter_network_crossbar_traversals = total_scatter_network_link_traversals / cluster_size;
      total_scatter_network_buffer_accesses = total_scatter_network_link_traversals / cluster_size;

      ElectricalNetworkRouterModel* m_gather_network_router_model = ElectricalNetworkRouterModel::create(num_gather_network_router_ports, num_gather_network_router_ports, 4, gather_network_link_width);
      ElectricalNetworkLinkModel* m_gather_network_link_model = ElectricalNetworkLinkModel::create("electrical_repeated", 1.0, 1.0, gather_network_link_width, true);
      OpticalNetworkLinkModel* m_optical_network_link_model = new OpticalNetworkLinkModel(1.0, 100.0, optical_network_link_width);
      ElectricalNetworkRouterModel* m_scatter_network_router_model = ElectricalNetworkRouterModel::create(num_clusters/2, 1, 4, scatter_network_link_width);
      ElectricalNetworkLinkModel* m_scatter_network_link_model = ElectricalNetworkLinkModel::create("electrical_repeated", 1.0, 1.0, scatter_network_link_width, true);

      m_gather_network_router_model->updateDynamicEnergySwitchAllocator(num_gather_network_router_ports/2);
      m_gather_network_router_model->updateDynamicEnergyCrossbar(gather_network_link_width/2);
      m_gather_network_router_model->updateDynamicEnergyClock();
      m_gather_network_link_model->updateDynamicEnergy(gather_network_link_width/2);
      
      m_optical_network_link_model->updateDynamicEnergy(optical_network_link_width/2);
      
      m_scatter_network_router_model->updateDynamicEnergySwitchAllocator(num_clusters/4);
      m_scatter_network_router_model->updateDynamicEnergyCrossbar(scatter_network_link_width/2);
      m_scatter_network_router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::WRITE, scatter_network_link_width/2);
      m_scatter_network_router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::READ, scatter_network_link_width/2);
      m_scatter_network_router_model->updateDynamicEnergyClock();
      m_scatter_network_link_model->updateDynamicEnergy(scatter_network_link_width/2);

      // Dynamic Energy - ENet
      double total_dynamic_energy_enet_router = \
         m_gather_network_router_model->getDynamicEnergySwitchAllocator() * total_gather_network_switch_allocator_traversals + \
         m_gather_network_router_model->getDynamicEnergyCrossbar() * total_gather_network_crossbar_traversals + \
         m_gather_network_router_model->getDynamicEnergyClock() * (total_gather_network_switch_allocator_traversals + total_gather_network_crossbar_traversals);
      double total_dynamic_energy_enet_link = \
         m_gather_network_link_model->getDynamicEnergy() * total_gather_network_link_traversals;
      double total_dynamic_energy_enet = total_dynamic_energy_enet_router + total_dynamic_energy_enet_link;
      
      // Dynamic Energy - ONet
      double total_dynamic_energy_onet_sender = \
         m_optical_network_link_model->getDynamicEnergySender() * total_optical_network_link_traversals;
      double total_dynamic_energy_onet_receiver = \
         m_optical_network_link_model->getDynamicEnergyReceiver() * total_optical_network_link_traversals;
      double total_dynamic_energy_onet = total_dynamic_energy_onet_sender + total_dynamic_energy_onet_receiver;
      
      // Dynamic Energy - BNet
      double total_dynamic_energy_bnet_router = \
         m_scatter_network_router_model->getDynamicEnergySwitchAllocator() * total_scatter_network_switch_allocator_traversals + \
         m_scatter_network_router_model->getDynamicEnergyBuffer() * total_scatter_network_buffer_accesses + \
         m_scatter_network_router_model->getDynamicEnergyCrossbar() * total_scatter_network_crossbar_traversals + \
         m_scatter_network_router_model->getDynamicEnergyClock() * (total_scatter_network_switch_allocator_traversals + total_scatter_network_buffer_accesses * 2 + total_scatter_network_crossbar_traversals);
      double total_dynamic_energy_bnet_link = \
         m_scatter_network_link_model->getDynamicEnergy() * total_scatter_network_link_traversals;
      double total_dynamic_energy_bnet = total_dynamic_energy_bnet_router + total_dynamic_energy_bnet_link;

      double total_dynamic_energy_anet = total_dynamic_energy_enet + total_dynamic_energy_onet + total_dynamic_energy_bnet;

      // Static Power - ENet
      double total_static_power_enet_router = num_clusters * \
                                              ( \
                                                m_gather_network_router_model->getTotalStaticPower() * cluster_size \
                                              );
      double total_static_power_enet_link = num_clusters * \
                                            ( \
                                              m_gather_network_link_model->getStaticPower() * (num_gather_network_router_ports-1) * cluster_size \
                                            );
      double total_static_power_enet = total_static_power_enet_router + total_static_power_enet_link;
      
      // Static Power - ONet
      double total_static_power_onet_laser = num_clusters * \
                                             ( \
                                               m_optical_network_link_model->getLaserPower() \
                                             );
      double total_static_power_onet_ring = num_clusters * \
                                          ( \
                                             m_optical_network_link_model->getRingTuningPower() \
                                          );
      double total_static_power_onet = num_clusters * \
                                       ( \
                                         m_optical_network_link_model->getStaticPower() \
                                       );
      assert(total_static_power_onet == (total_static_power_onet_laser + total_static_power_onet_ring));
     
      // Static Power - BNet
      double total_static_power_bnet_router = num_clusters * \
                                              ( \
                                                m_scatter_network_router_model->getTotalStaticPower() * num_scatter_networks_per_cluster \
                                              );
      double total_static_power_bnet_link = num_clusters * \
                                            ( \
                                              m_scatter_network_link_model->getStaticPower() * cluster_size * num_scatter_networks_per_cluster \
                                            );
      double total_static_power_bnet = total_static_power_bnet_router + total_static_power_bnet_link;
      
      double total_static_power_anet = total_static_power_enet + total_static_power_onet + total_static_power_bnet;

      printf("Static Energy(ANet) = %g, Dynamic Energy(ANet) = %g\n", \
            total_static_power_anet * completion_time_anet / 1e9 , total_dynamic_energy_anet);

      fprintf(energy_out, "%g\n%g\n%g\n%g\n%g\n%g\n%g\n%g\n%g\n%g\n%g\n%g\n",
            total_static_power_enet_router * completion_time_anet / 1e9,
            total_dynamic_energy_enet_router,
            total_static_power_enet_link * completion_time_anet / 1e9,
            total_dynamic_energy_enet_link,
            total_static_power_onet_laser * completion_time_anet / 1e9,
            total_static_power_onet_ring * completion_time_anet / 1e9,
            total_dynamic_energy_onet_sender,
            total_dynamic_energy_onet_receiver,
            total_static_power_bnet_router * completion_time_anet / 1e9,
            total_dynamic_energy_bnet_router,
            total_static_power_bnet_link * completion_time_anet / 1e9,
            total_dynamic_energy_bnet_link);
   }

   else
   {
      printf("Unrecognized Network(%s)\n", network.c_str());
      exit(EXIT_FAILURE);
   }

   CarbonStopSim();
   
   return 0;
}
