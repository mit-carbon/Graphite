#pragma once

#include "instruction.h"
#include "time_types.h"

class DynamicMemoryInfo
{
public:
   DynamicMemoryInfo(IntPtr address, bool read, Time latency, UInt32 num_misses)
      : _address(address)
      , _read(read)
      , _latency(latency)
      , _num_misses(num_misses)
   {}
   
   IntPtr _address;
   bool _read;
   Time _latency;
   UInt32 _num_misses;
};

class DynamicBranchInfo
{
public:
   DynamicBranchInfo(bool taken, IntPtr target)
      : _taken(taken)
      , _target(target)
   {}
   
   bool _taken;
   IntPtr _target;
};
