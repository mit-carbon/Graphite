#ifndef IOCOOM_PERFORMANCE_MODEL_H
#define IOCOOM_PERFORMANCE_MODEL_H

#include <vector>

#include "core_model.h"

/*
  In-order core, out-of-order memory performance model.

  We use a simpe scoreboard to keep track of registers.

  We also keep a store buffer to short circuit loads.
 */
class IOCOOMPerformanceModel : public CoreModel
{
public:
   IOCOOMPerformanceModel(Core* core, float frequency);
   ~IOCOOMPerformanceModel();

   void reset();
   void outputSummary(std::ostream &os);

private:

   void handleInstruction(Instruction *instruction);

   std::pair<UInt64,UInt64> executeLoad(UInt64 time, const DynamicInstructionInfo &);
   UInt64 executeStore(UInt64 time, const DynamicInstructionInfo &);

   UInt64 modelICache(IntPtr ins_address, UInt32 ins_size);
   void initializeRegisterScoreboard();

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

   Scoreboard m_register_scoreboard;
   StoreBuffer *m_store_buffer;
   LoadUnit *m_load_unit;
};

#endif // IOCOOM_PERFORMANCE_MODEL_H
