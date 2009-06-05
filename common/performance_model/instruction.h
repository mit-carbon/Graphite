#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "fixed_types.h"
#include <vector>

enum InstructionType
{
    INST_GENERIC,
    INST_LOAD,
    INST_STORE,
    INST_ADD,
    INST_SUB,
    INST_MUL,
    INST_DIV,
    INST_FADD,
    INST_FSUB,
    INST_FMUL,
    INST_FDIV,
    INST_JMP,
};

class Instruction
{
    public:
        Instruction(InstructionType type)
            : m_type(type) {}
        InstructionType getInstructionType()
        {
            return m_type;
        }

    private:
        InstructionType m_type;
};

enum OperandType
{
    OPERAND_REG,
    OPERAND_MEMORY
};

typedef UInt64 OperandValue;

class Operand
{
    public:
        Operand(const Operand &src)
            : m_type(src.m_type), m_value(src.m_value) {}

        Operand(OperandType type, OperandValue value)
            : m_type(type), m_value(value) {}

        OperandType m_type;
        OperandValue m_value;
};

class GenericInstruction : public Instruction
{
    public:
        GenericInstruction()
        : Instruction(INST_GENERIC)
        {}

};

class LoadInstruction : public Instruction
{
    public:
        LoadInstruction(Operand src, Operand dest)
        : 
            Instruction(INST_LOAD),
            m_src(src), m_dest(dest)
        {}

        Operand m_src;
        Operand m_dest;
};

class StoreInstruction : public Instruction
{
    public:
        StoreInstruction(Operand src, Operand dest)
        :
            Instruction(INST_STORE),
            m_src(src), m_dest(dest)
        {}
        Operand m_src;
        Operand m_dest;
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
