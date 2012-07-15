#include <cmath>

#include "fixed_types.h"
#include "shmem_perf_model.h"
#include "simulator.h"
#include "tile_manager.h"
#include "fxsupport.h"

ShmemPerfModel::ShmemPerfModel():
   _enabled(false)
{
   for (UInt32 i = 0; i < NUM_THREAD_TYPES; i++)
      _cycle_count[i] = 0;
   initializePerformanceCounters();
}

ShmemPerfModel::~ShmemPerfModel()
{}

void
ShmemPerfModel::initializePerformanceCounters()
{
   _num_memory_accesses = 0;
   _total_memory_access_latency_in_clock_cycles = 0;
   _total_memory_access_latency_in_ns = 0;
}

ShmemPerfModel::ThreadType 
ShmemPerfModel::getThreadType()
{
   if (Sim()->getTileManager()->amiAppThread())
   {
      assert(!Sim()->getTileManager()->amiSimThread());
      return _APP_THREAD;
   }
   else if (Sim()->getTileManager()->amiSimThread())
   {
      assert(!Sim()->getTileManager()->amiAppThread());
      return _SIM_THREAD;
   }
   else
   {
      assert(false);
      return NUM_THREAD_TYPES;
   }
}

void 
ShmemPerfModel::setCycleCount(ThreadType thread_type, UInt64 count)
{
   LOG_PRINT("setCycleCount: thread(%u), count(%llu)", thread_type, count);
   ScopedLock sl(_shmem_perf_model_lock);

   assert(thread_type < NUM_THREAD_TYPES);
   _cycle_count[thread_type] = count;
}

UInt64
ShmemPerfModel::getCycleCount()
{
   ScopedLock sl(_shmem_perf_model_lock);
   
   return _cycle_count[getThreadType()];
}

void
ShmemPerfModel::updateCycleCount(UInt64 cycle_count)
{
   LOG_PRINT("updateCycleCount: cycle_count(%llu)", cycle_count);
   ScopedLock sl(_shmem_perf_model_lock);

   ThreadType thread_type = getThreadType();
   if (_cycle_count[thread_type] < cycle_count)
      _cycle_count[thread_type] = cycle_count;
}

void
ShmemPerfModel::incrCycleCount(UInt64 count)
{
   ScopedLock sl(_shmem_perf_model_lock);

   ThreadType thread_type = getThreadType();
   UInt64 i_cycle_count = _cycle_count[thread_type];
   UInt64 f_cycle_count = i_cycle_count + count;

   LOG_ASSERT_ERROR(f_cycle_count >= i_cycle_count, "f_cycle_count(%llu) < i_cycle_count(%llu)",
                    f_cycle_count, i_cycle_count);

   LOG_PRINT("Initial(%llu), Final(%llu)", i_cycle_count, f_cycle_count);
   _cycle_count[thread_type] = f_cycle_count;
}

void
ShmemPerfModel::incrTotalMemoryAccessLatency(UInt64 memory_access_latency)
{
   if (_enabled)
   {
      ScopedLock sl(_shmem_perf_model_lock);
      
      _num_memory_accesses ++;
      _total_memory_access_latency_in_clock_cycles += memory_access_latency;
   }
}

void
ShmemPerfModel::updateInternalVariablesOnFrequencyChange(volatile float core_frequency)
{
   _total_memory_access_latency_in_ns +=
      static_cast<UInt64>(ceil(static_cast<float>(_total_memory_access_latency_in_clock_cycles) / core_frequency));
   _total_memory_access_latency_in_clock_cycles = 0;
}

void
ShmemPerfModel::outputSummary(ostream& out, volatile float core_frequency)
{
   _total_memory_access_latency_in_ns +=
      static_cast<UInt64>(ceil(static_cast<float>(_total_memory_access_latency_in_clock_cycles) / core_frequency));
   
   out << "Shared Memory Model summary: " << endl;
   out << "    Total Memory Accesses: " << _num_memory_accesses << endl;
   out << "    Average Memory Access Latency (in ns): " << 
      static_cast<float>(_total_memory_access_latency_in_ns) / _num_memory_accesses << endl;
}
