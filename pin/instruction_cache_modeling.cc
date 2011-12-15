#include "instruction_cache_modeling.h"
#include "simulator.h"
#include "tile_manager.h"
#include "core.h"

void modelInstructionCache(IntPtr address, UInt32 instruction_size)
{
   Core* core = Sim()->getTileManager()->getCurrentCore();
   core->readInstructionMemory(address, instruction_size);
}

void addInstructionCacheModeling(INS ins)
{
   INS_InsertCall(ins, IPOINT_BEFORE,
                  AFUNPTR(modelInstructionCache),
                  IARG_ADDRINT, INS_Address(ins),
                  IARG_UINT32, INS_Size(ins),
                  IARG_END);
}
