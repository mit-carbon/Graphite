#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <sstream>
#include <vector>
#include "fixed_types.h"

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
               OperandList &operands);

   Instruction(InstructionType type);

   virtual ~Instruction() { };
   virtual UInt64 getCost();

   InstructionType getType();

   static void initializeStaticInstructionModel();

   const OperandList& getOperands()
   { return m_operands; }

   void setAddress(IntPtr addr)
   { m_addr = addr; }
   IntPtr getAddress() const
   { return m_addr; }

   void setSize(UInt32 size)
   { m_size = size; }
   UInt32 getSize() const
   { return m_size; }

   bool isSimpleMemoryLoad() const;
   void print() const;

private:
   typedef std::vector<unsigned int> StaticInstructionCosts;
   static StaticInstructionCosts m_instruction_costs;

   InstructionType m_type;

   IntPtr m_addr;
   UInt32 m_size;

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

// CMPSB or SCASB instruction
class StringInstruction : public Instruction
{
public:
   StringInstruction(OperandList &ops);

   UInt64 getCost();
};

// for operations not associated with the binary -- such as processing
// a packet
class DynamicInstruction : public Instruction
{
public:
   DynamicInstruction(UInt64 cost, InstructionType type = INST_DYNAMIC_MISC);
   ~DynamicInstruction();

   UInt64 getCost();

private:
   UInt64 m_cost;
};

class RecvInstruction : public DynamicInstruction
{
public:
   RecvInstruction(UInt64 cost)
      : DynamicInstruction(cost, INST_RECV)
   {}
};

class SyncInstruction : public DynamicInstruction
{
public:
   SyncInstruction(UInt64 cost);
};

// set clock to particular time
class SpawnInstruction : public Instruction
{
public:
   SpawnInstruction(UInt64 time);
   UInt64 getCost();

private:
   UInt64 m_time;
};

// conditional branches
class BranchInstruction : public Instruction
{
public:
   BranchInstruction(OperandList &l);

   UInt64 getCost();
};

#endif
