#ifndef MAIN_CORE_H
#define MAIN_CORE_H

#include "core.h"

class MainCore : public Core
{
public:
   MainCore(Tile* tile);
   ~MainCore();

   UInt64 readInstructionMemory(IntPtr address, UInt32 instruction_size);
   pair<UInt32, UInt64> accessMemory(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr address,
                                     char* data_buffer, UInt32 data_size, bool push_info = false);

   pair<UInt32, UInt64> initiateMemoryAccess(MemComponent::Type mem_component,
                                             lock_signal_t lock_signal,
                                             mem_op_t mem_op_type,
                                             IntPtr address,
                                             Byte* data_buf, UInt32 data_size,
                                             bool push_info = false,
                                             UInt64 time = 0);
};

#endif
