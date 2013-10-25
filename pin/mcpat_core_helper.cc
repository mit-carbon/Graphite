#include "mcpat_core_interface.h"
#include "pin.H"
#include "instruction.h"

McPATCoreInterface::McPATInstructionType
getMcPATInstructionType(InstructionType type)
{
   // can be {INTEGER_INST, FLOATING_POINT_INST, BRANCH_INST}
     
   if ((type == INST_IALU) || (type == INST_IMUL) || (type == INST_IDIV))
   {
      return (McPATCoreInterface::INTEGER_INST);
   }
   else if ((type == INST_FALU) || (type == INST_FMUL) || (type == INST_FDIV) ||
            (type == INST_XMM_SS) || (type == INST_XMM_SD) || (type == INST_XMM_PS))
   {
      return (McPATCoreInterface::FLOATING_POINT_INST);
   }
   else if (type == INST_BRANCH)
   {
      return (McPATCoreInterface::BRANCH_INST);
   }
   else
   {
      return (McPATCoreInterface::GENERIC_INST);
   }
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

void
getExecutionUnitAccessList(InstructionType type, McPATCoreInterface::ExecutionUnitList& unit_list)
{
   // can be a vector of {ALU, MUL, FPU}
   // 
   // For SSE instructions, make it two FPU accesses for now
   // This is not entirely correct (but good for a 1st pass)

   if (type == INST_IALU)
   {   
      unit_list.push_back(McPATCoreInterface::ALU);
   }
   else if ((type == INST_IMUL) || (type == INST_IDIV))
   {
      unit_list.push_back(McPATCoreInterface::MUL);
   }
   else if ((type == INST_FALU) || (type == INST_FMUL) || (type == INST_FDIV))
   {
      unit_list.push_back(McPATCoreInterface::FPU);
   }
   else if ((type == INST_XMM_SS) || (type == INST_XMM_SD))
   {
      unit_list.push_back(McPATCoreInterface::FPU);
   }
   else if (type == INST_XMM_PS)
   {
      unit_list.push_back(McPATCoreInterface::FPU);
      unit_list.push_back(McPATCoreInterface::FPU);
   }
}
