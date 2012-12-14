#ifndef IOCOOM_CORE_MODEL_H
#define IOCOOM_CORE_MODEL_H

#include <vector>
#include <string>
using std::string;

#include "core_model.h"
#include "mcpat_core_interface.h"
/*
  In-order core, out-of-order memory model.
  We use a simple scoreboard to keep track of registers.
  We also keep a store buffer to short circuit loads.
 */
class IOCOOMCoreModel : public CoreModel
{
public:
   IOCOOMCoreModel(Core* core, float frequency);
   ~IOCOOMCoreModel();

   void updateInternalVariablesOnFrequencyChange(volatile float frequency);
   void outputSummary(std::ostream &os);

private:

   enum CoreUnit
   {
      INVALID_UNIT = 0,
      LOAD_UNIT = 1,
      STORE_UNIT = 2,
      EXECUTION_UNIT = 3
   };

   void handleInstruction(Instruction *instruction);

   UInt64 modelICache(IntPtr ins_address, UInt32 ins_size);
   std::pair<UInt64,UInt64> executeLoad(UInt64 time, const DynamicInstructionInfo &);
   UInt64 executeStore(UInt64 time, const DynamicInstructionInfo &);

   void initializeRegisterScoreboard();
   void initializeRegisterWaitUnitList();

   typedef std::vector<UInt64> Scoreboard;

   class LoadBuffer
   {
   public:
      LoadBuffer(unsigned int num_units);
      ~LoadBuffer();

      UInt64 execute(UInt64 time, UInt64 occupancy);

   private:
      Scoreboard m_scoreboard;
      
      void initialize();
   };

   class StoreBuffer
   {
   public:
      enum Status
      {
         VALID,
         COMPLETED,
         NOT_FOUND
      };

      StoreBuffer(unsigned int num_entries);
      ~StoreBuffer();

      /*
        @return Time store finishes.
        @param time Time store starts.
        @param addr Address of store.
      */
      UInt64 executeStore(UInt64 time, UInt64 occupancy, IntPtr addr);

      /*
        @return True if addr is in store buffer at given time.
        @param time Time to check for addr.
        @param addr Address to check.
      */
      Status isAddressAvailable(UInt64 time, IntPtr addr);

   private:
      Scoreboard m_scoreboard;
      std::vector<IntPtr> m_addresses;
      
      void initialize();
   };

   Scoreboard m_register_scoreboard;
   std::vector<CoreUnit> m_register_wait_unit_list;

   StoreBuffer *m_store_buffer;
   LoadBuffer *m_load_buffer;

   // Pipeline Stall Counters
   UInt64 m_total_load_buffer_stall_cycles;
   UInt64 m_total_store_buffer_stall_cycles;
   UInt64 m_total_l1icache_stall_cycles;
   UInt64 m_total_intra_ins_l1dcache_read_stall_cycles;
   UInt64 m_total_inter_ins_l1dcache_read_stall_cycles;
   UInt64 m_total_l1dcache_write_stall_cycles;
   UInt64 m_total_intra_ins_execution_unit_stall_cycles;
   UInt64 m_total_inter_ins_execution_unit_stall_cycles;
   void initializePipelineStallCounters();

   McPATCoreInterface* m_mcpat_core_interface;
};

#endif // IOCOOM_CORE_MODEL_H
