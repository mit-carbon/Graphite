#pragma once

class MemComponent
{
public:
   enum Type
   {
      INVALID_MEM_COMPONENT = 0,
      MIN_MEM_COMPONENT,
      CORE = MIN_MEM_COMPONENT,
      L1_ICACHE,
      L1_DCACHE,
      L2_CACHE,
      DRAM_DIRECTORY,
      DRAM,
      MAX_MEM_COMPONENT = DRAM,
      NUM_MEM_COMPONENTS = MAX_MEM_COMPONENT - MIN_MEM_COMPONENT + 1
   };
};
