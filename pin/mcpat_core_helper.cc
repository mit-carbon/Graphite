#include "mcpat_core_helper.h"
#include "pin.H"

void
fillMcPATMicroOpList(McPATInstruction::MicroOpList& micro_op_list,
                     InstructionType type, UInt32 num_read_memory_operands, UInt32 num_write_memory_operands)
{
   // can be {INTEGER_INST, FLOATING_POINT_INST, BRANCH_INST, LOAD_INST, STORE_INST}
     
   if ((type == INST_IALU) || (type == INST_IMUL) || (type == INST_IDIV))
   {
      micro_op_list.push_back(McPATInstruction::INTEGER_INST);
   }
   else if ((type == INST_FALU) || (type == INST_FMUL) || (type == INST_FDIV) ||
            (type == INST_XMM_SS) || (type == INST_XMM_SD))
   {
      micro_op_list.push_back(McPATInstruction::FLOATING_POINT_INST);
   }
   else if (type == INST_XMM_PS)
   {
      micro_op_list.push_back(McPATInstruction::FLOATING_POINT_INST);
      micro_op_list.push_back(McPATInstruction::FLOATING_POINT_INST);
   }
   else if (type == INST_BRANCH)
   {
      micro_op_list.push_back(McPATInstruction::BRANCH_INST);
   }
   else
   {
      micro_op_list.push_back(McPATInstruction::GENERIC_INST);
   }

   for (unsigned int i = 0; i < num_read_memory_operands; i++)
      micro_op_list.push_back(McPATInstruction::LOAD_INST);
   for (unsigned int i = 0; i < num_write_memory_operands; i++)
      micro_op_list.push_back(McPATInstruction::STORE_INST);
}

void
fillMcPATRegisterFileAccessCounters(McPATInstruction::RegisterFile& register_file,
                                    const RegisterOperandList& read_register_operands,
                                    const RegisterOperandList& write_register_operands)
{
   for (unsigned int i = 0; i < read_register_operands.size(); i++)
   {
      const RegisterOperand& reg = read_register_operands[i];
      if (isIntegerReg(reg))
         register_file._num_integer_reads ++;
      else if (isFloatingPointReg(reg))
         register_file._num_floating_point_reads ++;
      else if (isXMMReg(reg))
         register_file._num_floating_point_reads += 2;
   }
   for (unsigned int i = 0; i < write_register_operands.size(); i++)
   {
      const RegisterOperand& reg = write_register_operands[i];
      if (isIntegerReg(reg))
         register_file._num_integer_writes ++;
      else if (isFloatingPointReg(reg))
         register_file._num_floating_point_writes ++;
      else if (isXMMReg(reg))
         register_file._num_floating_point_writes += 2;
   }
}

void
fillMcPATExecutionUnitList(McPATInstruction::ExecutionUnitList& execution_unit_list,
                           InstructionType instruction_type)
{
   // can be a vector of {ALU, MUL, FPU}
   // 
   // For SSE instructions, make it two FPU accesses for now
   // This is not entirely correct (but good for a 1st pass)

   if (instruction_type == INST_IALU)
   {   
      execution_unit_list.push_back(McPATInstruction::ALU);
   }
   else if ((instruction_type == INST_IMUL) || (instruction_type == INST_IDIV))
   {
      execution_unit_list.push_back(McPATInstruction::MUL);
   }
   else if ((instruction_type == INST_FALU) || (instruction_type == INST_FMUL) || (instruction_type == INST_FDIV))
   {
      execution_unit_list.push_back(McPATInstruction::FPU);
   }
   else if ((instruction_type == INST_XMM_SS) || (instruction_type == INST_XMM_SD))
   {
      execution_unit_list.push_back(McPATInstruction::FPU);
   }
   else if (instruction_type == INST_XMM_PS)
   {
      execution_unit_list.push_back(McPATInstruction::FPU);
      execution_unit_list.push_back(McPATInstruction::FPU);
   }
}

bool
isIntegerReg(const RegisterOperand& reg_id)
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
isFloatingPointReg(const RegisterOperand& reg_id)
{
   return REG_is_fr((REG) reg_id);
}

bool
isXMMReg(const RegisterOperand& reg_id)
{
   return REG_is_xmm((REG) reg_id);
}
