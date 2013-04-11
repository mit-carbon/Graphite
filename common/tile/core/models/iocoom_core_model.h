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
   IOCOOMCoreModel(Core* core);
   ~IOCOOMCoreModel();

   void outputSummary(std::ostream &os);

   // Change voltage, frequency
   void setDVFS(double old_frequency, double new_voltage, double new_frequency);

   void computeEnergy();

   double getDynamicEnergy();
   double getStaticPower();

private:

   enum CoreUnit
   {
      INVALID_UNIT = 0,
      LOAD_UNIT = 1,
      STORE_UNIT = 2,
      EXECUTION_UNIT = 3
   };

   void handleInstruction(Instruction *instruction);

   Time modelICache(IntPtr ins_address, UInt32 ins_size);
   std::pair<Time,Time> executeLoad(Time time, const DynamicInstructionInfo &);
   Time executeStore(Time time, const DynamicInstructionInfo &);

   void initializeRegisterScoreboard();
   void initializeRegisterWaitUnitList();

   typedef std::vector<Time> Scoreboard;

   class LoadBuffer
   {
   public:
      LoadBuffer(unsigned int num_units);
      ~LoadBuffer();

      Time execute(Time time, Time occupancy);

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
      Time executeStore(Time time, Time occupancy, IntPtr addr);

      /*
        @return True if addr is in store buffer at given time.
        @param time Time to check for addr.
        @param addr Address to check.
      */
      Status isAddressAvailable(Time time, IntPtr addr);

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
   Time m_total_load_buffer_stall_time;
   Time m_total_store_buffer_stall_time;
   Time m_total_l1icache_stall_time;
   Time m_total_intra_ins_l1dcache_read_stall_time;
   Time m_total_inter_ins_l1dcache_read_stall_time;
   Time m_total_l1dcache_write_stall_time;
   Time m_total_intra_ins_execution_unit_stall_time;
   Time m_total_inter_ins_execution_unit_stall_time;
   void initializePipelineStallCounters();

   bool m_enable_area_and_power_modeling;
   McPATCoreInterface* m_mcpat_core_interface;
};

#endif // IOCOOM_CORE_MODEL_H
