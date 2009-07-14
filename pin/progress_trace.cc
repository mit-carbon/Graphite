#include <sys/time.h>

#include "pin.H"
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "progress_trace.h"

static UInt64 applicationStartTime;
static TLS_KEY threadCounterKey;
static int interval;

static bool enabled()
{
   return Log::getSingleton()->isLoggingEnabled() &&
      Sim()->getCfg()->getBool("log/trace_progress", false);
}

static UInt64 getTime()
{
   timeval t;
   gettimeofday(&t, NULL);
   UInt64 time = (((UInt64)t.tv_sec) * 1000000 + t.tv_usec);
   return time - applicationStartTime;
}

static VOID traceProgress()
{
   int counter = (int)PIN_GetThreadData(threadCounterKey);

   ++counter;
   counter %= interval;

   PIN_SetThreadData(threadCounterKey, (void*)counter);

   PerformanceModel *pm = Sim()->getCoreManager()->getCurrentCore()->getPerformanceModel();

   if (counter == 0 && pm->isEnabled())
   {
      LOG_PRINT("Progress Trace -- time: %llu, cycles: %llu", getTime(), pm->getCycleCount());
   }
}

VOID initProgressTrace()
{
   if (!enabled())
      return;

   try
   {
      interval = Sim()->getCfg()->getInt("log/trace_progress_interval");

      if (interval == 0)
      {
         LOG_PRINT_ERROR("Progress trace interval is zero.");
      }
   }
   catch (...)
   {
      LOG_PRINT_ERROR("No interval given for trace progress.");
   }

   applicationStartTime = getTime();

   threadCounterKey = PIN_CreateThreadDataKey(NULL);
}

VOID addProgressTrace(INS ins)
{
   if (!enabled())
      return;

   getTime();

   INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(traceProgress), IARG_END);
}
