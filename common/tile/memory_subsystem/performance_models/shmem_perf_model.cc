#include <cmath>

#include "fixed_types.h"
#include "shmem_perf_model.h"
#include "simulator.h"
#include "tile_manager.h"
#include "fxsupport.h"

ShmemPerfModel::ShmemPerfModel():
   m_enabled(false)
{
   for (UInt32 i = 0; i < NUM_CORE_THREADS; i++)
      m_cycle_count[i] = 0;
   initializePerformanceCounters();
}

ShmemPerfModel::~ShmemPerfModel()
{}

void
ShmemPerfModel::initializePerformanceCounters()
{
   m_num_memory_accesses = 0;
   m_total_memory_access_latency_in_clock_cycles = 0;
   m_total_memory_access_latency_in_ns = 0;
}

ShmemPerfModel::Thread_t 
ShmemPerfModel::getThreadNum()
{
   if (Sim()->getTileManager()->amiUserThread())
   {
      assert(!Sim()->getTileManager()->amiSimThread());
      return _USER_THREAD;
   }
   else if (Sim()->getTileManager()->amiSimThread())
   {
      assert(!Sim()->getTileManager()->amiUserThread());
      return _SIM_THREAD;
   }
   else
   {
      assert(false);
      return NUM_CORE_THREADS;
   }
}

void 
ShmemPerfModel::setCycleCount(Thread_t thread_num, UInt64 count)
{
   LOG_PRINT("setCycleCount: thread(%u), count(%llu)", thread_num, count);
   ScopedLock sl(m_shmem_perf_model_lock);

   assert(thread_num < NUM_CORE_THREADS);
   m_cycle_count[thread_num] = count;
}

UInt64
ShmemPerfModel::getCycleCount()
{
   ScopedLock sl(m_shmem_perf_model_lock);
   
   return m_cycle_count[getThreadNum()];
}

void
ShmemPerfModel::updateCycleCount(UInt64 cycle_count)
{
   LOG_PRINT("updateCycleCount: cycle_count(%llu)", cycle_count);
   ScopedLock sl(m_shmem_perf_model_lock);

   Thread_t thread_num = getThreadNum();
   if (m_cycle_count[thread_num] < cycle_count)
      m_cycle_count[thread_num] = cycle_count;
}

void
ShmemPerfModel::incrCycleCount(UInt64 count)
{
   ScopedLock sl(m_shmem_perf_model_lock);

   UInt64 i_cycle_count = m_cycle_count[getThreadNum()];
   UInt64 t_cycle_count = i_cycle_count + count;

   LOG_ASSERT_ERROR(t_cycle_count >= i_cycle_count,
         "t_cycle_count(%llu) < i_cycle_count(%llu)",
         t_cycle_count, i_cycle_count);

   m_cycle_count[getThreadNum()] = t_cycle_count;
}

void
ShmemPerfModel::incrTotalMemoryAccessLatency(UInt64 memory_access_latency)
{
   if (m_enabled)
   {
      ScopedLock sl(m_shmem_perf_model_lock);
      
      m_num_memory_accesses ++;
      m_total_memory_access_latency_in_clock_cycles += memory_access_latency;
   }
}

void
ShmemPerfModel::enable()
{
   m_enabled = true;
}

void
ShmemPerfModel::disable()
{
   m_enabled = false;
}

void
ShmemPerfModel::reset()
{
   initializePerformanceCounters();
}

void
ShmemPerfModel::updateInternalVariablesOnFrequencyChange(volatile float core_frequency)
{
   m_total_memory_access_latency_in_ns +=
      static_cast<UInt64>(ceil(static_cast<float>(m_total_memory_access_latency_in_clock_cycles) / core_frequency));
   m_total_memory_access_latency_in_clock_cycles = 0;
}

void
ShmemPerfModel::outputSummary(ostream& out, volatile float core_frequency)
{
   m_total_memory_access_latency_in_ns +=
      static_cast<UInt64>(ceil(static_cast<float>(m_total_memory_access_latency_in_clock_cycles) / core_frequency));
   
   out << "Shared Memory Model summary: " << endl;
   out << "    Total Memory Accesses: " << m_num_memory_accesses << endl;
   out << "    Average Memory Access Latency (in ns): " << 
      static_cast<float>(m_total_memory_access_latency_in_ns) / m_num_memory_accesses << endl;
}
