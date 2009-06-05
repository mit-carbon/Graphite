#ifndef DYNAMIC_INSTRUCTION_INFO_H
#define DYNAMIC_INSTRUCTION_INFO_H

struct DynamicInstructionInfo
{
   enum Type
   {
      INFO_MEMORY,
      INFO_SYNC,
   };

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
};

#endif
