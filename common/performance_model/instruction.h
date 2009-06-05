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
    MAX_INSTRUCTION_COUNT
};

__attribute__ ((unused)) static const char * INSTRUCTION_NAMES [] = 
{"GENERIC","ADD","SUB","MUL","DIV","FADD","FSUB","FMUL","FDIV","JMP"};


enum OperandType
{
    OPERAND_REG,
    OPERAND_MEMORY
};

enum OperandDirection
{
    OPERAND_READ,
    OPERAND_WRITE
};

typedef UInt64 OperandValue;

class Operand
{
    public:
        Operand(const Operand &src)
            : m_type(src.m_type), m_value(src.m_value), m_direction(src.m_direction) {}

        Operand(OperandType type, OperandValue value = 0, OperandDirection direction = OPERAND_READ)
            : m_type(type), m_value(value), m_direction(direction) {}

        OperandType m_type;
        OperandValue m_value;
        OperandDirection m_direction;
};

typedef std::vector<Operand> OperandList;

class Instruction
{
    public:
        Instruction(InstructionType type)
            : m_type(type) {}

        InstructionType getInstructionType();
        UInt64 getStaticCycleCount();

        static void initializeStaticInstructionModel();

    private:
        typedef std::vector<unsigned int> StaticInstructionCosts;
        static StaticInstructionCosts m_instruction_costs;
        InstructionType m_type;
};

class GenericInstruction : public Instruction
{
    public:
        GenericInstruction()
        : Instruction(INST_GENERIC)
        {}

        GenericInstruction(OperandList *operand_list)
        : Instruction(INST_GENERIC), m_operand_list(operand_list)
        {}


        OperandList *m_operand_list;
};

class ArithInstruction : public Instruction
{
    public:
        ArithInstruction(InstructionType type, const Operand &src1, const Operand &src2, const Operand &dest)
        : Instruction(type), m_src1(src1), m_src2(src2), m_dest(dest)
        {}
        Operand m_src1;
        Operand m_src2;
        Operand m_dest;
};

class JmpInstruction : public Instruction
{
    public:
        JmpInstruction(const Operand &dest)
        : Instruction(INST_JMP)
        {}
};

typedef std::vector<Instruction *> BasicBlock;

#endif
