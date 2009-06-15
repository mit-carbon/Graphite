#ifndef DYNAMIC_INSTRUCTION_INFO_H
#define DYNAMIC_INSTRUCTION_INFO_H

#include "instruction.h"

struct DynamicInstructionInfo
{
   enum Type
   {
      MEMORY_READ,
      MEMORY_WRITE,
      STRING,
   } type;

   union
   {
      struct
      {
         UInt64 latency;
         IntPtr addr;
      } memory_info;

      UInt32 num_ops;
   };

   // ctors

   DynamicInstructionInfo()
   {
   }

   DynamicInstructionInfo(const DynamicInstructionInfo &rhs)
   {
      type = rhs.type;
      memory_info = rhs.memory_info; // "use bigger one"
   }

   static DynamicInstructionInfo createMemoryInfo(UInt64 l, IntPtr a, Operand::Direction dir)
   {
      DynamicInstructionInfo i;
      i.type = (dir == Operand::READ) ? MEMORY_READ : MEMORY_WRITE;
      i.memory_info.latency = l;
      i.memory_info.addr = a;
      return i;
   }

   static DynamicInstructionInfo createStringInfo(UInt32 count)
   {
      DynamicInstructionInfo i;
      i.type = STRING;
      i.num_ops = count;
      return i;
   }
};

#endif
