#include <iostream>
#include <fstream>
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
   UInt32 num_cores = 1024;
   ifstream benchmark_data(argv[1]);

   UInt64 completion_time_emesh;
   UInt64 completion_time_anet;
   benchmark_data >> completion_time_emesh;
   benchmark_data >> completion_time_anet;

   double total_switch_allocator_traversals;
   double total_crossbar_traversals;
   double total_buffer_accesses;
   double total_link_traversals;

   benchmark_data >> total_switch_allocator_traversals;
   benchmark_data >> total_crossbar_traversals;
   benchmark_data >> total_buffer_accesses;
   benchmark_data >> total_link_traversals;

   UInt32 num_electrical_router_ports = 5;
   UInt32 electrical_link_width = 256;
   ElectricalNetworkRouterModel* m_electrical_router_model = ElectricalNetworkRouterModel::create(num_electrical_router_ports, 4, electrical_link_width);
   ElectricalNetworkLinkModel* m_electrical_link_model = ElectricalNetworkLinkModel::create("electrical_repeated", 1.0, 1.0, electrical_link_width, true);

   m_electrical_router_model->updateDynamicEnergySwitchAllocator(num_electrical_router_ports/2);
   m_electrical_router_model->updateDynamicEnergyCrossbar(electrical_link_width/2);
   m_electrical_router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::WRITE, electrical_link_width/2);
   m_electrical_router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::READ, electrical_link_width/2);
   m_electrical_router_model->updateDynamicEnergyClock();
   m_electrical_link_model->updateDynamicEnergy(electrical_link_width/2);

   printf("%g\n%g\n%g\n%g\n%g\n", m_electrical_router_model->getDynamicEnergySwitchAllocator(),
         m_electrical_router_model->getDynamicEnergyCrossbar(),
         m_electrical_router_model->getDynamicEnergyBuffer(),
         m_electrical_router_model->getDynamicEnergyClock(),
         m_electrical_link_model->getDynamicEnergy());
   printf("\n%g\n%g\n%g\n%g\n",
         total_switch_allocator_traversals,
         total_crossbar_traversals,
         total_buffer_accesses,
         total_link_traversals);

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

   // Next Optical
   UInt32 num_clusters = 64;
   UInt32 cluster_size = 16;
   UInt32 num_scatter_networks_per_cluster = 2;

   double total_gather_network_switch_allocator_traversals;
   double total_gather_network_crossbar_traversals;
   double total_gather_network_link_traversals;
   double total_optical_network_link_traversals;
   double total_scatter_network_link_traversals;

   benchmark_data >> total_gather_network_switch_allocator_traversals;
   benchmark_data >> total_gather_network_crossbar_traversals;
   benchmark_data >> total_gather_network_link_traversals;
   benchmark_data >> total_optical_network_link_traversals;
   benchmark_data >> total_scatter_network_link_traversals;

   UInt32 gather_network_link_width = 128;
   UInt32 optical_network_link_width = 128;
   UInt32 scatter_network_link_width = 128;
   ElectricalNetworkRouterModel* m_gather_network_router_model = ElectricalNetworkRouterModel::create(num_electrical_router_ports, 4, gather_network_link_width);
   ElectricalNetworkLinkModel* m_gather_network_link_model = ElectricalNetworkLinkModel::create("electrical_repeated", 1.0, 1.0, gather_network_link_width, true);
   OpticalNetworkLinkModel* m_optical_network_link_model = new OpticalNetworkLinkModel(1.0, 100.0, optical_network_link_width);
   ElectricalNetworkLinkModel* m_scatter_network_link_model = ElectricalNetworkLinkModel::create("electrical_repeated", 1.0, 1.0, scatter_network_link_width, true);

   m_gather_network_router_model->updateDynamicEnergySwitchAllocator(num_electrical_router_ports/2);
   m_gather_network_router_model->updateDynamicEnergyCrossbar(gather_network_link_width/2);
   m_gather_network_router_model->updateDynamicEnergyClock();
   m_gather_network_link_model->updateDynamicEnergy(gather_network_link_width/2);
   m_optical_network_link_model->updateDynamicEnergy(optical_network_link_width/2);
   m_scatter_network_link_model->updateDynamicEnergy(scatter_network_link_width/2);

   double total_dynamic_energy_anet_enet = \
      m_gather_network_router_model->getDynamicEnergySwitchAllocator() * total_gather_network_switch_allocator_traversals + \
      m_gather_network_router_model->getDynamicEnergyCrossbar() * total_gather_network_crossbar_traversals + \
      m_gather_network_router_model->getDynamicEnergyClock() * (total_gather_network_switch_allocator_traversals + total_gather_network_crossbar_traversals) + \
      m_gather_network_link_model->getDynamicEnergy() * total_gather_network_link_traversals;
   double total_dynamic_energy_anet_onet = \
      m_optical_network_link_model->getDynamicEnergy() * total_optical_network_link_traversals;
   double total_dynamic_energy_anet_bnet = \
      m_scatter_network_link_model->getDynamicEnergy() * total_scatter_network_link_traversals;
   double total_dynamic_energy_anet = total_dynamic_energy_anet_enet + total_dynamic_energy_anet_onet + total_dynamic_energy_anet_bnet;

   double total_static_power_anet_enet = num_clusters * \
                                    ( \
                                       m_gather_network_router_model->getTotalStaticPower() * cluster_size + \
                                       m_gather_network_link_model->getStaticPower() * (num_electrical_router_ports-1) * cluster_size \
                                    );
   double total_static_power_anet_onet = num_clusters * \
                                    ( \
                                       m_optical_network_link_model->getStaticPower() \
                                    );
   double total_static_power_anet_bnet = num_clusters * \
                                    ( \
                                       m_scatter_network_link_model->getStaticPower() * cluster_size * num_scatter_networks_per_cluster \
                                    );
   double total_static_power_anet = total_static_power_anet_enet + total_static_power_anet_onet + total_static_power_anet_bnet;

   printf("Static Energy(EMesh) = %g, Dynamic Energy(EMesh) = %g\n", \
         total_static_power_emesh * completion_time_emesh / 1e9, total_dynamic_energy_emesh);
   printf("Static Energy(ANet) = %g, Dynamic Energy(ANet) = %g\n", \
         total_static_power_anet * completion_time_anet / 1e9 , total_dynamic_energy_anet);

   printf("\nEMesh:\n%g\n%g\n%g\n%g\n",
         total_static_power_emesh_router * completion_time_emesh / 1e9,
         total_dynamic_energy_emesh_router,
         total_static_power_emesh_link * completion_time_emesh / 1e9,
         total_dynamic_energy_emesh_link);

   printf("\nANet:\n%g\n%g\n%g\n%g\n%g\n%g\n\n",
         total_static_power_anet_enet * completion_time_anet / 1e9,
         total_dynamic_energy_anet_enet,
         total_static_power_anet_onet * completion_time_anet / 1e9,
         total_dynamic_energy_anet_onet,
         total_static_power_anet_bnet * completion_time_anet / 1e9,
         total_dynamic_energy_anet_bnet);

   CarbonStopSim();
   
   return 0;
}
