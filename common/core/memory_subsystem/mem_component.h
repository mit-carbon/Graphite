#ifndef __MEM_COMPONENT_H__
#define __MEM_COMPONENT_H__

class MemComponent
{
   public:
      enum component_t
      {
         INVALID_MEM_COMPONENT = 0,
         MIN_MEM_COMPONENT,
         CORE = MIN_MEM_COMPONENT,
         L1_ICACHE,
         L1_DCACHE,
         L1_PEP_ICACHE,
         L1_PEP_DCACHE,
         L2_CACHE,
         DRAM_DIR,
         DRAM,
         MAX_MEM_COMPONENT = DRAM,
         NUM_MEM_COMPONENTS = MAX_MEM_COMPONENT - MIN_MEM_COMPONENT + 1
      };
};

#endif /* __MEM_COMPONENT_H__ */
