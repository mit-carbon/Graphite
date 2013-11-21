#ifndef IOCOOM_CORE_MODEL_H
#define IOCOOM_CORE_MODEL_H

#include "core_model.h"
/*
  In-order core, out-of-order memory model.
  We use a simple scoreboard to keep track of registers.
  We also keep a store buffer to enable load bypassing.
 */
class IOCOOMCoreModel : public CoreModel
{
private:
   enum CoreUnit
   {
      INVALID_UNIT = 0,
      LOAD_UNIT = 1,
      STORE_UNIT = 2,
      EXECUTION_UNIT = 3
   };

   typedef vector<Time> Scoreboard;

public:
   IOCOOMCoreModel(Core* core);
   ~IOCOOMCoreModel();

   void outputSummary(ostream &os, const Time& target_completion_time);

   class LoadQueue
   {
   public:
      LoadQueue(CoreModel* core_model);
      ~LoadQueue();

      pair<Time,Time> execute(const Time& schedule_time, const Time& occupancy);
      const Time& getLastDeallocateTime();
      friend ostringstream& operator<<(ostringstream& os, const LoadQueue& queue);

   private:
      Scoreboard _scoreboard;
      UInt32 _num_entries;
      bool _speculative_loads_enabled;
      UInt32 _allocate_idx;
      Time _ONE_CYCLE;
   };

   class StoreQueue
   {
   public:
      enum Status
      {
         VALID,
         COMPLETED,
         NOT_FOUND
      };

      StoreQueue(CoreModel* core_model);
      ~StoreQueue();

      Time execute(const Time& schedule_time, const Time& occupancy,
                   const Time& last_load_deallocate_time, IntPtr address);
      const Time& getLastDeallocateTime();
      Status isAddressAvailable(const Time& schedule_time, IntPtr address);
      friend ostringstream& operator<<(ostringstream& os, const StoreQueue& queue);

   private:
      Scoreboard _scoreboard;
      vector<IntPtr> _addresses;
      UInt32 _num_entries;
      bool _multiple_outstanding_RFOs_enabled;
      UInt32 _allocate_idx;
      Time _ONE_CYCLE;
   };

private:

   static const UInt32 _NUM_REGISTERS = 512;
   
   StoreQueue *_store_queue;
   LoadQueue *_load_queue;

   Scoreboard _register_scoreboard;
   vector<CoreUnit> _register_dependency_list;

   Time _ONE_CYCLE;

   McPATCoreInterface* _mcpat_core_interface;
   
   // Pipeline Stall Counters
   Time _total_load_queue_stall_time;
   Time _total_store_queue_stall_time;
   Time _total_l1icache_stall_time;
   Time _total_intra_ins_l1dcache_stall_time;
   Time _total_inter_ins_l1dcache_stall_time;
   Time _total_intra_ins_execution_unit_stall_time;
   Time _total_inter_ins_execution_unit_stall_time;
   
   void handleInstruction(Instruction *instruction);

   pair<Time,Time> executeLoad(const Time& schedule_time, const DynamicMemoryInfo& info);
   Time executeStore(const Time& schedule_time, const DynamicMemoryInfo& info);

   void initializePipelineStallCounters();
};

#endif // IOCOOM_CORE_MODEL_H
