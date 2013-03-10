#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <sstream>
#include <vector>
#include "fixed_types.h"
#include "time_types.h"

// forward declaration
class CoreModel;

enum InstructionType
{
   INST_GENERIC,
   INST_ADD,
   INST_SUB,
   INST_MUL,
   INST_DIV,
   INST_FADD,
   INST_FSUB,
   INST_FMUL,
   INST_FDIV,
   INST_JMP,
   INST_DYNAMIC_MISC,
   INST_RECV,
   INST_SYNC,
   INST_SPAWN,
   INST_STRING,
   INST_BRANCH,
   MAX_INSTRUCTION_COUNT
};

__attribute__ ((unused)) static const char * INSTRUCTION_NAMES [] = 
{"generic","add","sub","mul","div","fadd","fsub","fmul","fdiv","jmp","dynamic_misc","recv","sync","spawn","string","branch"};

class Operand
{
public:
   enum Type
   {
      REG,
      MEMORY,
      IMMEDIATE
   };

   enum Direction
   {
      READ,
      WRITE
   };

   typedef UInt64 Value;

   Operand(const Operand &src)
      : m_type(src.m_type), m_value(src.m_value), m_direction(src.m_direction) {}

   Operand(Type type, Value value = 0, Direction direction = READ)
      : m_type(type), m_value(value), m_direction(direction) {}

   Type m_type;
   Value m_value;
   Direction m_direction;

   void print(std::ostringstream& out) const;
};

typedef std::vector<Operand> OperandList;

class Instruction
{
public:
   Instruction(InstructionType type,
               UInt64 opcode,
               OperandList &operands);

   Instruction(InstructionType type);

   virtual ~Instruction() {}
   virtual Time getCost(CoreModel* perf);

   static void initializeStaticInstructionModel();

   InstructionType getType()
   { return m_type; }
   UInt64 getOpcode() const
   { return m_opcode; }
   IntPtr getAddress() const
   { return m_address; }
   UInt32 getSize() const
   { return m_size; }
   const OperandList& getOperands() const
   { return m_operands; }

   void setAddress(IntPtr address)
   { m_address = address; }
   void setSize(UInt32 size)
   { m_size = size; }

   bool isSimpleMemoryLoad() const;
   bool isDynamic() const
   { return ((m_type == INST_DYNAMIC_MISC) || (m_type == INST_RECV) || (m_type == INST_SYNC)); }

   void print() const;

   typedef std::vector<unsigned int> StaticInstructionCosts;

   static StaticInstructionCosts getStaticInstructionCosts(){ return m_instruction_costs; }

private:
   static StaticInstructionCosts m_instruction_costs;

   InstructionType m_type;
   UInt64 m_opcode;

   IntPtr m_address;
   UInt32 m_size;


protected:
   OperandList m_operands;
};

class GenericInstruction : public Instruction
{
public:
   GenericInstruction(UInt64 opcode, OperandList &operands)
      : Instruction(INST_GENERIC, opcode, operands)
   {}
};

class ArithInstruction : public Instruction
{
public:
   ArithInstruction(InstructionType type, UInt64 opcode, OperandList &operands)
      : Instruction(type, opcode, operands)
   {}
};

class JmpInstruction : public Instruction
{
public:
   JmpInstruction(UInt64 opcode, OperandList &dest)
      : Instruction(INST_JMP, opcode, dest)
   {}
};

// conditional branches
class BranchInstruction : public Instruction
{
public:
   BranchInstruction(UInt64 opcode, OperandList &l);

   Time getCost(CoreModel* perf);
};

// for operations not associated with the binary -- such as processing
// a packet
class DynamicInstruction : public Instruction
{
public:
   DynamicInstruction(Time cost, InstructionType type = INST_DYNAMIC_MISC)
      : Instruction(type)
      , m_cost(cost)
   {}
   ~DynamicInstruction() {}

   Time getCost(CoreModel* perf)
   { return m_cost; }

protected:
   Time m_cost;
};

// RecvInstruction
class RecvInstruction : public DynamicInstruction
{
public:
   RecvInstruction(Time cost)
      : DynamicInstruction(cost, INST_RECV)
   {}
};

// SyncInstruction
class SyncInstruction : public DynamicInstruction
{
public:
   SyncInstruction(Time cost)
      : DynamicInstruction(cost, INST_SYNC)
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
