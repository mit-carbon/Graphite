#ifndef __SHMEM_PERF_MODEL_H__
#define __SHMEM_PERF_MODEL_H__

#include <cassert>
#include "simulator.h"
#include "core_manager.h"

class ShmemPerfModel
{
   public:
      enum CoreThread_t
      {
         _USER_THREAD = 0,
         _SIM_THREAD,
         NUM_CORE_THREADS
      };

   private:
      UInt64 m_cycle_count[NUM_CORE_THREADS];
      bool m_enabled;

      UInt32 getThreadNum()
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


   public:
      ShmemPerfModel()
         : m_enabled(true)
      {
         for (UInt32 i = 0; i < NUM_CORE_THREADS; i++)
            m_cycle_count[i] = 0;
      }

      ~ShmemPerfModel() {}

      void setCycleCount(CoreThread_t thread_num, UInt64 count)
      {
         assert(thread_num < NUM_CORE_THREADS);
         m_cycle_count[thread_num] = count;
      }

      void setCycleCount(UInt64 count)
      {
         m_cycle_count[getThreadNum()] = count;
      }

      UInt64 getCycleCount()
      {
         return m_cycle_count[getThreadNum()];
      }

      void updateCycleCount(UInt64 count)
      {
         m_cycle_count[getThreadNum()] += count;
      }

      void enable()
      {
         m_enabled = true;
      }

      void disable()
      {
         m_enabled = false;
      }

      bool isEnabled()
      {
         return m_enabled;
      }
};

#endif /* __SHMEM_PERF_MODEL_H__ */
