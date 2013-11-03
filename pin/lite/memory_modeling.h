#pragma once

#include "fixed_types.h"
#include "pin.H"

namespace lite
{

void addMemoryModeling(INS ins);
void handleMemoryRead(THREADID thread_id, bool is_atomic_update, IntPtr read_address, UInt32 read_data_size);
void handleMemoryWrite(THREADID thread_id, bool is_atomic_update, IntPtr write_address, UInt32 write_data_size);
IntPtr captureWriteEa(IntPtr tgt_ea);

}
