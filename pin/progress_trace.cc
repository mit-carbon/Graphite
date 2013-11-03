#include <sys/time.h>
#include <stdio.h>
#include <vector>
using std::vector;

#include "pin.H"
#include "progress_trace.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "core_model.h"
#include "hash_map.h"

extern HashMap core_map;

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

static FILE* getFileDescriptor(Tile* tile)
{
   tile_id_t id = tile->getId();

   if (!tile) return NULL;

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

static VOID traceProgress(THREADID thread_id)
{
   UInt64* counter_ptr = (UInt64*) PIN_GetThreadData(threadCounterKey);
   UInt64 counter = *counter_ptr;

   Core *core = core_map.get<Core>(thread_id);
   CoreModel *pm = core->getModel();

   UInt64 curr_time = pm->getCurrTime().getTime();

   LOG_ASSERT_ERROR(counter <= curr_time, "counter(%llu) > curr_time(%llu)", counter, curr_time);

   if (curr_time - counter > interval)
   {
      FILE *f = getFileDescriptor(core->getTile());

      if (f)
         fprintf(f, "time: %llu, curr_time: %llu\n", \
               (long long unsigned int) getTime(),\
               (long long unsigned int) curr_time);

      *counter_ptr = curr_time;
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

   files.resize(Sim()->getConfig()->getTotalTiles());
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

   INS_InsertCall(ins, IPOINT_BEFORE,
                  AFUNPTR(traceProgress),
                  IARG_THREAD_ID,
                  IARG_END);
}
