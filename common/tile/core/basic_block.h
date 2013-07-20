#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H

#include <vector>

class Instruction;

class BasicBlock : public std::vector<Instruction*>
{};

#endif
