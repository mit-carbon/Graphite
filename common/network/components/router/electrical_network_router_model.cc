#include "electrical_network_router_model.h"
#include "electrical_network_router_model_orion.h"
#include "log.h"

ElectricalNetworkRouterModel*
ElectricalNetworkRouterModel::create(UInt32 num_input_ports, UInt32 num_output_ports, UInt32 num_flits_per_buffer, UInt32 flit_width, bool use_orion)
{
   LOG_ASSERT_ERROR(use_orion, "Only Orion has electrical router models at this point of time");
   return new ElectricalNetworkRouterModelOrion(num_input_ports, num_output_ports, num_flits_per_buffer, flit_width);
}
