#include <sys/time.h>
#include <stdio.h>
#include <vector>

using std::vector;

#include "pin.H"
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "progress_trace.h"

static UInt64 applicationStartTime;
static TLS_KEY threadCounterKey;
static unsigned int interval;
static vector<FILE*> files;
static const char* BASE_OUTPUT_FILENAME = "progress_trace";

static bool enabled()
{
   return Sim()->getCfg()->getBool("progress_trace/enabled", false);
}

static UInt64 getTime()
{
   timeval t;
   gettimeofday(&t, NULL);
   UInt64 time = (((UInt64)t.tv_sec) * 1000000 + t.tv_usec);
   return time - applicationStartTime;
}

static FILE* getFileDescriptor()
{
   Tile *core = Sim()->getTileManager()->getCurrentTile();
   core_id_t id = core->getId();

   if (!core) return NULL;

   FILE *f = files[id];

   if (!f)
   {
      char filename[256];
      sprintf(filename, "%s_%d", BASE_OUTPUT_FILENAME, id);
      files[id] = fopen(Config::getSingleton()->formatOutputFileName(filename).c_str(),"w");
      assert(files[id]);
      f = files[id];
   }

   return f;
}

static VOID traceProgress()
{
   UInt64* counter_ptr = (UInt64*) PIN_GetThreadData(threadCounterKey);
   UInt64 counter = *counter_ptr;

   PerformanceModel *pm = Sim()->getTileManager()->getCurrentTile()->getPerformanceModel();

   UInt64 cycles = pm->getCycleCount();

   LOG_ASSERT_ERROR(counter <= cycles, "counter(%llu) > cycles(%llu)", counter, cycles);

   if (cycles - counter > interval)
   {
      FILE *f = getFileDescriptor();

      if (f)
         fprintf(f, "time: %llu, cycles: %llu\n", \
               (long long unsigned int) getTime(),\
               (long long unsigned int) cycles);

      *counter_ptr = cycles;
   }
}

VOID initProgressTrace()
{
   if (!enabled())
      return;

   try
   {
      interval = (unsigned int)Sim()->getCfg()->getInt("progress_trace/interval");

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

   files.resize(Sim()->getConfig()->getTotalCores());
   for (unsigned int i = 0; i < files.size(); i++)
      files[i] = NULL;

   threadCounterKey = PIN_CreateThreadDataKey(NULL);
}

VOID shutdownProgressTrace()
{
   for (unsigned int i = 0; i < files.size(); i++)
   {
      if (files[i])
         fclose(files[i]);
   }
}

VOID threadStartProgressTrace()
{
   if (!enabled())
      return;

   UInt64* counter_ptr = new UInt64(0);
   LOG_ASSERT_ERROR(*counter_ptr == 0, "*counter_ptr = %llu", *counter_ptr);

   PIN_SetThreadData(threadCounterKey, counter_ptr);
}

VOID addProgressTrace(INS ins)
{
   if (!enabled())
      return;

   INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(traceProgress), IARG_END);
}
