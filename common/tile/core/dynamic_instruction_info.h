#ifndef DYNAMIC_INSTRUCTION_INFO_H
#define DYNAMIC_INSTRUCTION_INFO_H

#include "instruction.h"
#include "time_types.h"

struct DynamicInstructionInfo
{
   enum Type
   {
      MEMORY_READ,
      MEMORY_WRITE,
      BRANCH,
   } type;

   union
   {
      // MEMORY
      struct
      {
         UInt64 latency;
         IntPtr addr;
         UInt32 num_misses;
      } memory_info;

      // BRANCH
      struct
      {
         bool taken;
         IntPtr target;
      } branch_info;
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

   static DynamicInstructionInfo createMemoryInfo(Time time, IntPtr a, Operand::Direction dir, UInt32 num_misses)
   {
      DynamicInstructionInfo i;
      i.type = (dir == Operand::READ) ? MEMORY_READ : MEMORY_WRITE;
      i.memory_info.latency = time.getTime(); // time stored in picoseconds
      i.memory_info.addr = a;
      i.memory_info.num_misses = num_misses;
      return i;
   }

   static DynamicInstructionInfo createBranchInfo(bool taken, IntPtr target)
   {
      DynamicInstructionInfo i;
      i.type = BRANCH;
      i.branch_info.taken = taken;
      i.branch_info.target = target;
      return i;
   }
};

#endif
