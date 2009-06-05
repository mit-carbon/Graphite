#ifndef DYNAMIC_INSTRUCTION_INFO_H
#define DYNAMIC_INSTRUCTION_INFO_H

struct DynamicInstructionInfo
{
   enum Type
   {
      INFO_MEMORY,
      INFO_SYNC,
   } type;

   union
   {
      struct
      {
         UInt64 latency;
         IntPtr addr;
      } memory_info;

      struct
      {
         UInt64 time;
      } sync_info;
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

   static DynamicInstructionInfo createMemoryInfo(UInt64 l, IntPtr a)
   {
      DynamicInstructionInfo i;
      i.type = INFO_MEMORY;
      i.memory_info.latency = l;
      i.memory_info.addr = a;
      return i;
   }

   static DynamicInstructionInfo createSyncInfo(UInt64 t)
   {
      DynamicInstructionInfo i;
      i.type = INFO_SYNC;
      i.sync_info.time = t;
      return i;
   }
};

#endif
