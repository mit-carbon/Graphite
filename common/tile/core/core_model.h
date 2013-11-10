#ifndef CORE_MODEL_H
#define CORE_MODEL_H
// This class represents the actual performance model for a given core

#include <boost/circular_buffer.hpp>
#include <iostream>
#include <string>

// Forward Decls
class Core;
class BranchPredictor;
class McPATCoreInterface;

#include "instruction.h"
#include "basic_block.h"
#include "fixed_types.h"
#include "dynamic_instruction_info.h"

class CoreModel
{
public:
   CoreModel(Core* core);
   virtual ~CoreModel();

   void processDynamicInstruction(DynamicInstruction* i);
   void queueInstruction(Instruction* instruction);
   void iterate();

   void setDVFS(double old_frequency, double new_voltage, double new_frequency, const Time& curr_time);
   void recomputeAverageFrequency(double frequency); 

   Time getCurrTime() { return m_curr_time; }
   void setCurrTime(Time time);

   void pushDynamicInstructionInfo(DynamicInstructionInfo &i);
   void popDynamicInstructionInfo();
   DynamicInstructionInfo& getDynamicInstructionInfo();

   static CoreModel* create(Core* core);

   BranchPredictor *getBranchPredictor() { return m_bp; }

   void enable();
   void disable();
   bool isEnabled()  { return m_enabled; }

   virtual void outputSummary(std::ostream &os, const Time& target_completion_time) = 0;

   void computeEnergy(const Time& curr_time);
   double getDynamicEnergy();
   double getLeakageEnergy();

   class AbortInstructionException { };

   Time getCost(InstructionType type);

   Core* getCore()   { return m_core; }

protected:
   enum RegType
   {
      INTEGER = 0,
      FLOATING_POINT
   };
   enum AccessType
   {
      READ = 0,
      WRITE
   };
   enum ExecutionUnitType
   {
   };

   friend class SpawnInstruction;

   typedef boost::circular_buffer<DynamicInstructionInfo> DynamicInstructionInfoQueue;
   typedef boost::circular_buffer<Instruction*> InstructionQueue;

   Core* m_core;

   Time m_curr_time;
   UInt64 m_instruction_count;
   
   void updatePipelineStallCounters(Instruction* i, Time memory_stall_time, Time execution_unit_stall_time);

   // Power/Area modeling
   void initializeMcPATInterface(UInt32 num_load_buffer_entries, UInt32 num_store_buffer_entries);
   void updateMcPATCounters(Instruction* instruction);

private:

   class DynamicInstructionInfoNotAvailableException { };

   virtual void handleInstruction(Instruction *instruction) = 0;

   // Pipeline Stall Counters
   void initializePipelineStallCounters();

   double m_average_frequency;
   Time m_total_time;
   Time m_checkpointed_time;
   UInt64 m_total_cycles;

   BranchPredictor *m_bp;

   InstructionQueue m_instruction_queue;
   DynamicInstructionInfoQueue m_dynamic_info_queue;

   bool m_enabled;

   // Instruction costs
   typedef std::vector<Time> CoreStaticInstructionCosts;
   CoreStaticInstructionCosts m_core_instruction_costs;
   void initializeCoreStaticInstructionModel(double frequency);
   void updateCoreStaticInstructionModel(double frequency);

   // Pipeline Stall Counters
   UInt64 m_total_recv_instructions;
   UInt64 m_total_sync_instructions;
   Time m_total_recv_instruction_stall_time;
   Time m_total_sync_instruction_stall_time;
   Time m_total_memory_stall_time;
   Time m_total_execution_unit_stall_time;

   // Power/Area modeling
   McPATCoreInterface* m_mcpat_core_interface;
};

#endif
