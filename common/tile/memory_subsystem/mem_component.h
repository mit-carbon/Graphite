#pragma once

#include <string>
using std::string;

class MemComponent
{
public:
   enum Type
   {
      INVALID,
      CORE,
      L1_ICACHE,
      L1_DCACHE,
      L2_CACHE,
      DRAM_DIRECTORY,
      DRAM_CNTLR
   };

   static string getName(Type type);
};

#define SPELL_MEMCOMP(x)      (MemComponent::getName(x).c_str())
