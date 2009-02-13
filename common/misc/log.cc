#include "log.h"
#include "config.h"
#include <sys/time.h>
#include <stdarg.h>
#include "lock.h"

//#define DISABLE_LOGGING

using namespace std;

Log *Log::_singleton;

Log::Log(UInt32 coreCount)
      : _coreCount(coreCount)
{
   char filename[256];

   _coreFiles = new FILE* [2 * _coreCount];
   for (UInt32 i = 0; i < _coreCount; i++)
   {
      sprintf(filename, "output_files/app_%u", i);
      _coreFiles[i] = fopen(filename, "w");
      assert(_coreFiles[i] != NULL);
   }
   for (UInt32 i = _coreCount; i < 2 * _coreCount; i++)
   {
      sprintf(filename, "output_files/sim_%u", i-_coreCount);
      _coreFiles[i] = fopen(filename, "w");
      assert(_coreFiles[i] != NULL);
   }

   _coreLocks = new Lock* [2 * _coreCount];
   for (UInt32 i = 0; i < 2 * _coreCount; i++)
   {
      _coreLocks[i] = Lock::create();
   }

   assert(g_config->getProcessCount() != 0);

   _systemFiles = new FILE* [g_config->getProcessCount()];
   _systemLocks = new Lock* [g_config->getProcessCount()];
   for (UInt32 i = 0; i < g_config->getProcessCount(); i++)
   {
      sprintf(filename, "output_files/system_%u", i);
      _systemFiles[i] = fopen(filename, "w");
      assert(_systemFiles[i] != NULL);

      _systemLocks[i] = Lock::create();
   }

   _defaultFile = fopen("output_files/system-default","w");
   _defaultLock = Lock::create();

   g_config->getDisabledLogModules(_disabledModules);

   assert(_singleton == NULL);
   _singleton = this;
}

Log::~Log()
{
   _singleton = NULL;

   for (UInt32 i = 0; i < 2 * _coreCount; i++)
   {
      fclose(_coreFiles[i]);
      delete _coreLocks[i];
   }

   delete [] _coreLocks;
   delete [] _coreFiles;

   for (UInt32 i = 0; i < g_config->getProcessCount(); i++)
   {
      fclose(_systemFiles[i]);
      delete _systemLocks[i];
   }

   delete [] _systemFiles;
   delete [] _systemLocks;

   fclose(_defaultFile);
   delete _defaultLock;
}

Log* Log::getSingleton()
{
   assert(_singleton);
   return _singleton;
}

Boolean Log::isEnabled(const char* module)
{
#ifdef DISABLE_LOGGING
   return false;
#endif
   return _disabledModules.find(module) == _disabledModules.end();
}

UInt64 Log::getTimestamp()
{
   timeval t;
   gettimeofday(&t, NULL);
   return (((UInt64)t.tv_sec) * 1000000 + t.tv_usec);
}

// FIXME: See note below.
#include "core_manager.h"

void Log::getFile(UInt32 core_id, FILE **file, Lock **lock)
{
   *file = NULL;
   *lock = NULL;

   if (core_id == (UInt32)-1)
   {
      // System file -- use process num if available
      if (g_config->getCurrentProcessNum() != (UInt32) -1)
      {
         assert(g_config->getCurrentProcessNum() < g_config->getProcessCount());
         *file = _systemFiles[g_config->getCurrentProcessNum()];
         *lock = _systemLocks[g_config->getCurrentProcessNum()];
      }
      else
      {
         *file = _defaultFile;
         *lock = _defaultLock;
      }
   }
   else
   {
      // Core file

      // FIXME: This is an ugly hack. Net/shared mem threads do not
      // have a core_id, so we can use the core ID to discover which
      // thread we are on.
      UInt32 fileID;

      if (g_core_manager == NULL)
      {
         fileID = core_id + _coreCount;
      }
      else
      {
         SInt32 myCoreID;
         myCoreID = g_core_manager->getCurrentCoreID();
         if (myCoreID == -1)
            fileID = core_id + _coreCount;
         else
            fileID = core_id;
      }

      *file = _coreFiles[fileID];
      *lock = _coreLocks[fileID];
   }
}

void Log::log(UInt32 core_id, const char *module, const char *format, ...)
{
#ifdef DISABLE_LOGGING
   return;
#endif

   if (!isEnabled(module))
      return;

   FILE *file;
   Lock *lock;

   getFile(core_id, &file, &lock);

   lock->acquire();

   if (core_id < _coreCount)
      fprintf(file, "%llu {%i}\t[%i]\t[%s] ", getTimestamp(), g_config->getCurrentProcessNum(), core_id, module);
   else if (g_config->getCurrentProcessNum() != (UInt32)-1)
      fprintf(file, "%llu {%i}\t[ ]\t[%s] ", getTimestamp(), g_config->getCurrentProcessNum(), module);
   else
      fprintf(file, "%llu { }\t[ ]\t[%s] ", getTimestamp(), module);

   va_list args;
   va_start(args, format);
   vfprintf(file, format, args);
   va_end(args);

   fprintf(file, "\n");

   fflush(file);

   lock->release();
}

void Log::notifyWarning()
{
   if (_state == None)
   {
      fprintf(stderr, "LOG : Check logs -- there is a warning!\n");
      _state = Warning;
   }
}

void Log::notifyError()
{
   if (_state == None || _state == Warning)
   {
      fprintf(stderr, "LOG : Check logs -- there is an ERROR!\n");
      _state = Error;
   }
}

