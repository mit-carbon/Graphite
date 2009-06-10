#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "fixed_types.h"
#include "dynamic_instruction_info.h"
#include <vector>

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
   MAX_INSTRUCTION_COUNT
};

__attribute__ ((unused)) static const char * INSTRUCTION_NAMES [] = 
{"GENERIC","ADD","SUB","MUL","DIV","FADD","FSUB","FMUL","FDIV","JMP","DYNAMIC"};

class Operand
{
public:
   enum Type
   {
      REG,
      MEMORY
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
};

typedef std::vector<Operand> OperandList;

class Instruction
{
public:
   Instruction(InstructionType type,
               OperandList &operands)
      : m_type(type)
      , m_operands(operands)
   {}

   Instruction(InstructionType type)
      : m_type(type)
   {}

   virtual ~Instruction() { };

   InstructionType getType();

   virtual UInt64 getCost();

   static void initializeStaticInstructionModel();

   const OperandList& getOperands()
   { return m_operands; }

private:
   typedef std::vector<unsigned int> StaticInstructionCosts;
   static StaticInstructionCosts m_instruction_costs;

   InstructionType m_type;

protected:
   OperandList m_operands;
};

class GenericInstruction : public Instruction
{
public:
   GenericInstruction(OperandList &operands)
      : Instruction(INST_GENERIC, operands)
   {}
};

class ArithInstruction : public Instruction
{
public:
   ArithInstruction(InstructionType type, OperandList &operands)
      : Instruction(type, operands)
   {}
};

class JmpInstruction : public Instruction
{
public:
   JmpInstruction(OperandList &dest)
      : Instruction(INST_JMP, dest)
   {}
};

// for operations not associated with the binary -- such as processing
// a packet
class DynamicInstruction : public Instruction
{
public:
   DynamicInstruction(UInt64 cost, InstructionType type = INST_DYNAMIC_MISC)
      : Instruction(type)
      , m_cost(cost)
   {
   }

   UInt64 getCost() { return m_cost; }
  
private:
   UInt64 m_cost;
};

class RecvInstruction : public DynamicInstruction
{
public:
   RecvInstruction(UInt64 cost)
      : DynamicInstruction(cost, INST_RECV)
   {
   }
};

class SyncInstruction : public DynamicInstruction
{
public:
   SyncInstruction(UInt64 cost)
      : DynamicInstruction(cost, INST_SYNC)
   {
   }
};

#endif
