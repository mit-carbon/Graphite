#include "instruction.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "core_model.h"
#include "branch_predictor.h"

// Instruction

Instruction::Instruction(InstructionType type, UInt64 opcode, IntPtr address, UInt32 size, bool atomic,
                         const OperandList& operands, const McPATInstruction* mcpat_instruction)
   : _type(type)
   , _dynamic(false)
   , _opcode(opcode)
   , _address(address)
   , _size(size)
   , _atomic(atomic)
   , _operands(operands)
   , _mcpat_instruction(mcpat_instruction)
{
   bool simple_memory_load = ((_operands.getNumReadMemory() == 1) && (_operands.getNumWriteMemory() == 0));
   _simple_mov_memory_load = simple_memory_load && (_type == INST_MOV);
}

Instruction::Instruction(InstructionType type, bool dynamic)
   : _type(type)
   , _dynamic(true)
   , _mcpat_instruction(NULL)
{
}

Time
Instruction::getCost(CoreModel* perf)
{
   LOG_ASSERT_ERROR(_type < MAX_INSTRUCTION_COUNT, "Unknown instruction type: %d", _type);
   return perf->getCost(_type); 
}

// BranchInstruction

BranchInstruction::BranchInstruction(UInt64 opcode, IntPtr address, UInt32 size, bool atomic,
                                     const OperandList& operands, const McPATInstruction* mcpat_instruction)
   : Instruction(INST_BRANCH, opcode, address, size, atomic, operands, mcpat_instruction)
{}

Time
BranchInstruction::getCost(CoreModel* perf)
{
   double frequency = perf->getCore()->getFrequency();
   BranchPredictor *bp = perf->getBranchPredictor();

   const DynamicBranchInfo& info = perf->getDynamicBranchInfo();

   // branch prediction not modeled
   if (bp == NULL)
   {
      perf->popDynamicBranchInfo();
      return Time(Latency(1,frequency));
   }

   bool prediction = bp->predict(getAddress(), info._target);
   bool correct = (prediction == info._taken);

   bp->update(prediction, info._taken, getAddress(), info._target);
   Latency cost = correct ? Latency(1,frequency) : Latency(bp->getMispredictPenalty(),frequency);
      
   perf->popDynamicBranchInfo();
   return Time(cost);
}

// SpawnInstruction

SpawnInstruction::SpawnInstruction(Time cost)
   : DynamicInstruction(cost, INST_SPAWN)
{}

Time
SpawnInstruction::getCost(CoreModel* perf)
{
   perf->setCurrTime(_cost);
   throw CoreModel::AbortInstructionException(); // exit out of handleInstruction
}
