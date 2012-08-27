#include "mem_op.h"
#include "log.h"

string
MemOp::getName(Core::mem_op_t mem_op_type)
{
   switch (mem_op_type)
   {
   case Core::READ:
      return "READ";
   case Core::READ_EX:
      return "READ_EX";
   case Core::WRITE:
      return "WRITE";
   default:
      LOG_PRINT_ERROR("Unrecognized mem op type(%u)", mem_op_type);
      return "";
   }
}
