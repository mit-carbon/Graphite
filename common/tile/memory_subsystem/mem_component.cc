#include "mem_component.h"
#include "log.h"

string
MemComponent::getName(Type type)
{
   switch (type)
   {
   case INVALID:
      return "INVALID";
   case CORE:
      return "CORE";
   case L1_ICACHE:
      return "L1_ICACHE";
   case L1_DCACHE:
      return "L1_DCACHE";
   case L2_CACHE:
      return "L2_CACHE";
   case DRAM_DIRECTORY:
      return "DRAM_DIRECTORY";
   case DRAM_CNTLR:
      return "DRAM_CNTLR";
   default:
      LOG_PRINT_ERROR("Unrecognized mem component type(%u)", type);
      return "";
   }
}
