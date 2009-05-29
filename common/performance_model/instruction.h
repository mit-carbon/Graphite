#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "fixed_types.h"

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
        : 
            Instruction(INST_GENERIC)
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
        ArithInstruction(InstructionType type, Operand src1, Operand src2, Operand dest)
        : Instruction(type), m_src1(src1), m_src2(src2), m_dest(dest)
        {}
        Operand m_src1;
        Operand m_src2;
        Operand m_dest;
};

class AddInstruction : public ArithInstruction
{
    public:
        AddInstruction(Operand src1, Operand src2, Operand dest)
        : ArithInstruction(INST_ADD, src1, src2, dest)
        {}
};

class SubInstruction : public ArithInstruction
{
    public:
        SubInstruction(Operand src1, Operand src2, Operand dest)
        : ArithInstruction(INST_SUB, src1, src2, dest)
        {}
};

class MulInstruction : public ArithInstruction
{
    public:
        MulInstruction(Operand src1, Operand src2, Operand dest)
        : ArithInstruction(INST_MUL, src1, src2, dest)
        {}
};

class DivInstruction : public ArithInstruction
{
    public:
        DivInstruction(Operand src1, Operand src2, Operand dest)
        : ArithInstruction(INST_DIV, src1, src2, dest)
        {}
};

class FAddInstruction : public ArithInstruction
{
    public:
        FAddInstruction(Operand src1, Operand src2, Operand dest)
        : ArithInstruction(INST_FADD, src1, src2, dest)
        {}
};

class FSubInstruction : public ArithInstruction
{
    public:
        FSubInstruction(Operand src1, Operand src2, Operand dest)
        : ArithInstruction(INST_FSUB, src1, src2, dest)
        {}
};

class FMulInstruction : public ArithInstruction
{
    public:
        FMulInstruction(Operand src1, Operand src2, Operand dest)
        : ArithInstruction(INST_FMUL, src1, src2, dest)
        {}
};

class FDivInstruction : public ArithInstruction
{
    public:
        FDivInstruction(Operand src1, Operand src2, Operand dest)
        : ArithInstruction(INST_FDIV, src1, src2, dest)
        {}
};

class JmpInstruction : public Instruction
{
    public:
        JmpInstruction(Operand dest)
        : Instruction(INST_JMP)
        {}
};

#endif
