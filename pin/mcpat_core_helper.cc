#include "mcpat_core_interface.h"
#include "pin.H"

McPATCoreInterface::InstructionType
getInstructionType(UInt64 opcode)
{
   // Note: Sabrina, please implement this function
   // can be {INTEGER_INST, FLOATING_POINT_INST, BRANCH_INST}
   return (McPATCoreInterface::INTEGER_INST);
}

bool
isIntegerReg(UInt32 reg_id)
{
   return (REG_is_gr((REG) reg_id) || 
           REG_is_gr8((REG) reg_id)|| 
           REG_is_gr16((REG) reg_id) ||
           REG_is_gr32((REG) reg_id) ||
           REG_is_gr64((REG) reg_id) ||
           REG_is_seg((REG) reg_id) ||
           REG_is_br((REG) reg_id));
}

bool
isFloatingPointReg(UInt32 reg_id)
{
   return REG_is_fr((REG) reg_id);
}

bool
isXMMReg(UInt32 reg_id)
{
   return REG_is_xmm((REG) reg_id);
}

McPATCoreInterface::ExecutionUnitList
getExecutionUnitAccessList(UInt64 opcode)
{
   // Note: Sabrina, please implement this function
   // can be a vector of {ALU, MUL, FPU}
   // 
   // For SSE instructions, make it two FPU accesses for now
   // This is not entirely correct (but good for a 1st pass)
   McPATCoreInterface::ExecutionUnitList access_list;
   access_list.push_back(McPATCoreInterface::ALU);
   return access_list;
}
