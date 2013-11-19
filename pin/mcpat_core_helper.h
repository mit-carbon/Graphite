#pragma once

#include "mcpat_instruction.h"
#include "instruction.h"

void fillMcPATMicroOpList(McPATInstruction::MicroOpList& micro_op_list,
                          InstructionType type, UInt32 num_read_memory_operands, UInt32 num_write_memory_operands);
void fillMcPATRegisterFileAccessCounters(McPATInstruction::RegisterFile& register_file,
                                         const RegisterOperandList& read_register_operands,
                                         const RegisterOperandList& write_register_operands);
void fillMcPATExecutionUnitList(McPATInstruction::ExecutionUnitList& execution_unit_list,
                                InstructionType instruction_type);

// Utils
bool isIntegerReg(const RegisterOperand& reg);
bool isFloatingPointReg(const RegisterOperand& reg_id);
bool isXMMReg(const RegisterOperand& reg_id);
