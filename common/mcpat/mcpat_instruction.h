#pragma once

#include <vector>
using std::vector;
#include "fixed_types.h"

class McPATInstruction
{
public:
   enum MicroOpType
   {
      GENERIC_INST = 0,
      INTEGER_INST,
      FLOATING_POINT_INST,
      LOAD_INST,
      STORE_INST,
      BRANCH_INST
   };
   typedef vector<MicroOpType> MicroOpList;
   
   class RegisterFile
   {
   public:
      RegisterFile()
         : _num_integer_reads(0)
         , _num_integer_writes(0)
         , _num_floating_point_reads(0)
         , _num_floating_point_writes(0)
      {}
      UInt32 _num_integer_reads;
      UInt32 _num_integer_writes;
      UInt32 _num_floating_point_reads;
      UInt32 _num_floating_point_writes;
   };

   enum ExecutionUnitType
   {
      ALU = 0,
      MUL,
      FPU
   };
   typedef vector<ExecutionUnitType> ExecutionUnitList;
 
   McPATInstruction(const MicroOpList& micro_op_list, const RegisterFile& register_file, const ExecutionUnitList& execution_unit_list)
      : _micro_op_list(micro_op_list)
      , _register_file(register_file)
      , _execution_unit_list(execution_unit_list)
   {}
   ~McPATInstruction()
   {}
   
   const MicroOpList& getMicroOpList() const
   { return _micro_op_list; }
   const RegisterFile& getRegisterFile() const
   { return _register_file; }
   const ExecutionUnitList& getExecutionUnitList() const
   { return _execution_unit_list; }

private:
   const MicroOpList _micro_op_list;
   const RegisterFile _register_file;
   const ExecutionUnitList _execution_unit_list;
};
