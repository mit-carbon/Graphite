#pragma once

#include "fixed_types.h"
#include "pin.H"

namespace lite
{

void addMemoryModeling(INS ins);
void handleMemoryRead(bool is_atomic_update, IntPtr read_address, UInt32 read_data_size);
void handleMemoryWrite(bool is_atomic_update, IntPtr write_address, UInt32 write_data_size);
ADDRINT MemOpSaveEa(ADDRINT ea);

}
