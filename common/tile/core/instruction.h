#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
using std::vector;
using std::string;
using std::ostringstream;
using std::stringstream;

#include "fixed_types.h"
#include "time_types.h"
#include "mcpat_instruction.h"

// forward declaration
class CoreModel;

enum InstructionType
{
   INST_GENERIC,
   INST_MOV,
   INST_IALU,
   INST_IMUL,
   INST_IDIV,
   INST_FALU,
   INST_FMUL,
   INST_FDIV,
   INST_XMM_SS,
   INST_XMM_SD,
   INST_XMM_PS,
   INST_BRANCH,
   INST_LFENCE,
   INST_SFENCE,
   INST_MFENCE,
   INST_DYNAMIC_MISC,
   INST_RECV,
   INST_SYNC,
   INST_SPAWN,
   INST_STALL,
   MAX_INSTRUCTION_COUNT
};

__attribute__((unused)) static const char * INSTRUCTION_NAMES [] = 
{"generic","mov","ialu","imul","idiv","falu","fmul","fdiv","xmm_ss","xmm_sd","xmm_ps","branch","lfence","sfence","mfence","dynamic_misc","recv","sync","spawn","stall"};

typedef UInt32 RegisterOperand;
typedef vector<RegisterOperand> RegisterOperandList;
typedef vector<UInt64> ImmediateOperandList;

class OperandList
{
public:
   OperandList()
      : _num_read_memory_operands(0)
      , _num_write_memory_operands(0)
   {}
   OperandList(const RegisterOperandList& read_register_operands, const RegisterOperandList& write_register_operands,
               UInt32 num_read_memory_operands, UInt32 num_write_memory_operands,
               const ImmediateOperandList& immediate_operands)
      : _read_register_operands(read_register_operands)
      , _write_register_operands(write_register_operands)
      , _num_read_memory_operands(num_read_memory_operands)
      , _num_write_memory_operands(num_write_memory_operands)
      , _immediate_operands(immediate_operands)
   {}
   ~OperandList()
   {}

   const RegisterOperandList& getReadRegister() const    { return _read_register_operands; }
   const RegisterOperandList& getWriteRegister() const   { return _write_register_operands; }
   const UInt32& getNumReadMemory() const                { return _num_read_memory_operands; }
   const UInt32& getNumWriteMemory() const               { return _num_write_memory_operands; }
   const ImmediateOperandList& getImmediate() const      { return _immediate_operands; }

private:
   const RegisterOperandList _read_register_operands;
   const RegisterOperandList _write_register_operands;
   const UInt32 _num_read_memory_operands;
   const UInt32 _num_write_memory_operands;
   const ImmediateOperandList _immediate_operands;
};

class Instruction
{
public:
   Instruction(InstructionType type, UInt64 opcode, IntPtr address, UInt32 size, bool atomic,
               const OperandList& operands, const McPATInstruction* mcpat_instruction);
   Instruction(InstructionType type, bool dynamic);
   virtual ~Instruction() {}
   
   virtual Time getCost(CoreModel* perf);

   InstructionType getType() const
   { return _type; }
   bool isDynamic() const
   { return _dynamic; }
   UInt64 getOpcode() const
   { return _opcode; }
   IntPtr getAddress() const
   { return _address; }
   UInt32 getSize() const
   { return _size; }
   bool isAtomic() const
   { return _atomic; }

   const RegisterOperandList& getReadRegisterOperands() const
   { return _operands.getReadRegister(); }
   const RegisterOperandList& getWriteRegisterOperands() const
   { return _operands.getWriteRegister(); }
   const UInt32& getNumReadMemoryOperands() const
   { return _operands.getNumReadMemory(); }
   const UInt32& getNumWriteMemoryOperands() const
   { return _operands.getNumWriteMemory(); }
   const ImmediateOperandList& getImmediateOperands() const
   { return _operands.getImmediate(); }

   const McPATInstruction* getMcPATInstruction() const
   { return _mcpat_instruction; }
   
   bool isSimpleMovMemoryLoad() const
   { return _simple_mov_memory_load; }

private:
   InstructionType _type;
   bool _dynamic;
   UInt64 _opcode;
   IntPtr _address;
   UInt32 _size;
   bool _atomic;

   const OperandList _operands;
   const McPATInstruction* _mcpat_instruction;
   bool _simple_mov_memory_load;
};

// conditional branches
class BranchInstruction : public Instruction
{
public:
   BranchInstruction(UInt64 opcode, IntPtr address, UInt32 size, bool atomic,
                     const OperandList& operands, const McPATInstruction* mcpat_instruction);
   Time getCost(CoreModel* perf);
};

// for operations not associated with the binary -- such as processing
// a packet
class DynamicInstruction : public Instruction
{
public:
   DynamicInstruction(Time cost, InstructionType type)
      : Instruction(type, true)
      , _cost(cost)
   {}
   ~DynamicInstruction() {}

   Time getCost(CoreModel* perf)
   { return _cost; }

protected:
   Time _cost;
};

// RecvInstruction - called for netRecv
class RecvInstruction : public DynamicInstruction
{
public:
   RecvInstruction(Time cost)
      : DynamicInstruction(cost, INST_RECV)
   {}
};

// SyncInstruction - called for SYNC instructions
class SyncInstruction : public DynamicInstruction
{
public:
   SyncInstruction(Time cost)
      : DynamicInstruction(cost, INST_SYNC)
   {}
};

// Stall instruction
class StallInstruction : public DynamicInstruction
{
public:
   StallInstruction(Time cost)
      : DynamicInstruction(cost, INST_STALL)
   {}
};

// SpawnInstruction - set clock to particular time
class SpawnInstruction : public DynamicInstruction
{
public:
   SpawnInstruction(Time time);
   Time getCost(CoreModel* perf);
};

#endif
