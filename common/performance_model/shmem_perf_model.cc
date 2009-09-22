#include "fixed_types.h"
#include "shmem_perf_model.h"
#include "simulator.h"
#include "core_manager.h"

ShmemPerfModel::ShmemPerfModel():
   m_enabled(true),
   m_num_memory_accesses(0),
   m_total_memory_access_latency(0.0)
{
   for (UInt32 i = 0; i < NUM_CORE_THREADS; i++)
      m_cycle_count[i] = 0;
}

ShmemPerfModel::~ShmemPerfModel()
{}

ShmemPerfModel::Thread_t 
ShmemPerfModel::getThreadNum()
{
   if (Sim()->getCoreManager()->amiUserThread())
   {
      assert(!Sim()->getCoreManager()->amiSimThread());
      return _USER_THREAD;
   }
   else if (Sim()->getCoreManager()->amiSimThread())
   {
      assert(!Sim()->getCoreManager()->amiUserThread());
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
ShmemPerfModel::updateCycleCount(UInt64 count)
{
   ScopedLock sl(m_shmem_perf_model_lock);

   Thread_t thread_num = getThreadNum();
   if (m_cycle_count[thread_num] < count)
      m_cycle_count[thread_num] = count;
}

void
ShmemPerfModel::incrCycleCount(UInt64 count)
{
   LOG_PRINT("incrCycleCount: %llu", count);
   ScopedLock sl(m_shmem_perf_model_lock);

   UInt64 i_cycle_count = m_cycle_count[getThreadNum()];
   UInt64 t_cycle_count = i_cycle_count + count;

   LOG_ASSERT_ERROR(t_cycle_count >= i_cycle_count,
         "t_cycle_count(%llu) < i_cycle_count(%llu)",
         t_cycle_count, i_cycle_count);

   m_cycle_count[getThreadNum()] = t_cycle_count;
}

void
ShmemPerfModel::incrTotalMemoryAccessLatency(UInt64 shmem_time)
{
   if (isEnabled())
   {
      m_num_memory_accesses ++;
      m_total_memory_access_latency += (double) shmem_time;
   }
}
      
void
ShmemPerfModel::outputSummary(ostream& out)
{
   out << "Shmem Perf Model summary: " << endl;
   out << "    num memory accesses: " << m_num_memory_accesses << endl;
   out << "    average memory access latency: " << 
      (float) (m_total_memory_access_latency / m_num_memory_accesses) << endl;
}
