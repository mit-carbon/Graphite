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

   void reset();
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

   void modelIcache(IntPtr address);
   std::pair<UInt64,UInt64> executeLoad(UInt64 time, const DynamicInstructionInfo &);
   UInt64 executeStore(UInt64 time, const DynamicInstructionInfo &);

   void initializeRegisterScoreboard();
   void initializeRegisterWaitUnitList();

   typedef std::vector<UInt64> Scoreboard;

   class LoadUnit
   {
   public:
      LoadUnit(unsigned int num_units);
      ~LoadUnit();

      UInt64 execute(UInt64 time, UInt64 occupancy);

      void reset();

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

      void reset();

   private:
      Scoreboard m_scoreboard;
      std::vector<IntPtr> m_addresses;
      
      void initialize();
   };

   UInt64 m_instruction_count;
   UInt64 m_total_memory_stall_cycles;

   Scoreboard m_register_scoreboard;
   std::vector<CoreUnit> m_register_wait_unit_list;

   StoreBuffer *m_store_buffer;
   LoadUnit *m_load_unit;

   McPATCoreInterface* m_mcpat_core_interface;
};

#endif // IOCOOM_CORE_MODEL_H
