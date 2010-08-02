#pragma once

#include "fixed_types.h"

class ElectricalNetworkRouterModel
{
public:
   class BufferAccess
   {
   public:
      enum type_t
      {
         READ = 0,
         WRITE,
         NUM_ACCESS_TYPES
      };
   };

   ElectricalNetworkRouterModel() {}
   ~ElectricalNetworkRouterModel() {}

   // Dynamic Energy - Maybe change this to updateDynamicEnergy* later
   virtual void updateDynamicEnergyBuffer(BufferAccess::type_t buffer_access_type, UInt32 num_bit_flips, UInt32 num_flits = 1) = 0;
   virtual void updateDynamicEnergyCrossbar(UInt32 num_bit_flips, UInt32 num_flits = 1) = 0;
   virtual void updateDynamicEnergySwitchAllocator(UInt32 num_requests, UInt32 num_flits = 1) = 0;
   virtual void updateDynamicEnergyClock(UInt32 num_flits = 1) = 0;
   
   // Total Dynamic Energy
   virtual volatile double getDynamicEnergyBuffer() = 0; 
   virtual volatile double getDynamicEnergyCrossbar() = 0;
   virtual volatile double getDynamicEnergySwitchAllocator() = 0;
   virtual volatile double getDynamicEnergyClock() = 0;
   virtual volatile double getTotalDynamicEnergy() = 0;

   // Static Power
   virtual volatile double getStaticPowerBuffer() = 0;
   virtual volatile double getStaticPowerBufferCrossbar() = 0;
   virtual volatile double getStaticPowerSwitchAllocator() = 0;
   virtual volatile double getStaticPowerClock() = 0;
   virtual volatile double getTotalStaticPower() = 0;

   static ElectricalNetworkRouterModel* create(UInt32 num_ports, UInt32 num_flits_per_buffer, UInt32 flit_width, bool use_orion = true);
};
