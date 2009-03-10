#include <sys/time.h>
#include <sys/syscall.h>
#include <stdarg.h>

#include "log.h"
#include "config.h"
#include "simulator.h"
#include "core_manager.h"

#define DISABLE_LOGGING

using namespace std;

Log *Log::_singleton;

Log::Log(UInt32 coreCount)
      : _coreCount(coreCount)
      , _startTime(0)
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

   _coreLocks = new Lock [2 * _coreCount];

   assert(Config::getSingleton()->getProcessCount() != 0);

   _systemFiles = new FILE* [Config::getSingleton()->getProcessCount()];
   _systemLocks = new Lock [Config::getSingleton()->getProcessCount()];
   for (UInt32 i = 0; i < Config::getSingleton()->getProcessCount(); i++)
   {
      sprintf(filename, "output_files/system_%u", i);
      _systemFiles[i] = fopen(filename, "w");
      assert(_systemFiles[i] != NULL);
   }

   _defaultFile = fopen("output_files/system-default","w");

   Config::getSingleton()->getDisabledLogModules(_disabledModules);

   assert(_singleton == NULL);
   _singleton = this;
}

Log::~Log()
{
   _singleton = NULL;

   for (UInt32 i = 0; i < 2 * _coreCount; i++)
   {
      fclose(_coreFiles[i]);
   }

   delete [] _coreLocks;
   delete [] _coreFiles;

   for (UInt32 i = 0; i < Config::getSingleton()->getProcessCount(); i++)
   {
      fclose(_systemFiles[i]);
   }

   delete [] _systemFiles;

   fclose(_defaultFile);
}

Log* Log::getSingleton()
{
   assert(_singleton);
   return _singleton;
}

Boolean Log::isEnabled(const char* module)
{
   return _disabledModules.find(module) == _disabledModules.end();
}

UInt64 Log::getTimestamp()
{
   timeval t;
   gettimeofday(&t, NULL);
   UInt64 time = (((UInt64)t.tv_sec) * 1000000 + t.tv_usec);
   if (_startTime == 0) _startTime = time;
   return time - _startTime;
}
void Log::discoverCore(core_id_t *core_id, bool *sim_thread)
{
   CoreManager *core_manager;

   if (!Sim() || !(core_manager = Sim()->getCoreManager()))
   {
      *core_id = INVALID_CORE_ID;
      *sim_thread = false;
      return;
   }

   *core_id = core_manager->getCurrentCoreID();
   if (*core_id != INVALID_CORE_ID)
   {
      *sim_thread = false;
      return;
   }
   else
   {
      *core_id = core_manager->getCurrentSimThreadCoreID();
      *sim_thread = true;
      return;
   }
}

void Log::getFile(core_id_t core_id, bool sim_thread, FILE **file, Lock **lock)
{
   *file = NULL;
   *lock = NULL;

   if (core_id == INVALID_CORE_ID)
   {
      // System file -- use process num if available
      if (Config::getSingleton()->getCurrentProcessNum() != (UInt32) -1)
      {
         assert(Config::getSingleton()->getCurrentProcessNum() < Config::getSingleton()->getProcessCount());
         *file = _systemFiles[Config::getSingleton()->getCurrentProcessNum()];
         *lock = &_systemLocks[Config::getSingleton()->getCurrentProcessNum()];
      }
      else
      {
         *file = _defaultFile;
         *lock = &_defaultLock;
      }
   }
   else
   {
      // Core file
      UInt32 fileID = core_id + (sim_thread ? _coreCount : 0);
      *file = _coreFiles[fileID];
      *lock = &_coreLocks[fileID];
   }
}

std::string Log::getModule(const char *filename)
{
#ifdef LOCK_LOGS
   ScopedLock sl(_modules_lock);
#endif
   std::map<const char*, std::string>::const_iterator it = _modules.find(filename);

   if (it != _modules.end())
   {
      return it->second;
   }
   else
   {
      // build module string
      string mod;

      // find actual file name ...
      const char *ptr = strrchr(filename, '/');
      if (ptr != NULL)
         filename = ptr + 1;

      for (UInt32 i = 0; i < MODULE_LENGTH && filename[i] != '\0'; i++)
         mod.push_back(filename[i]);

      while (mod.length() < MODULE_LENGTH)
         mod.push_back(' ');

      pair<const char*, std::string> p(filename, mod);
      _modules.insert(p);

      return mod;
   }
}

void Log::log(ErrorState err, const char* source_file, SInt32 source_line, const char *format, ...)
{
#ifdef DISABLE_LOGGING
   if (err != Error)
      return;
#endif

   core_id_t core_id;
   bool sim_thread;
   discoverCore(&core_id, &sim_thread);
   
   if (!isEnabled(source_file))
      return;

   FILE *file;
   Lock *lock;

   getFile(core_id, sim_thread, &file, &lock);
   int tid = syscall(__NR_gettid);

   lock->acquire();

   std::string module = getModule(source_file);

   // This is ugly, but it just prints the time stamp, process number, core number, source file/line
   if (core_id != INVALID_CORE_ID) // valid core id
      fprintf(file, "%-10llu [%5d]  (%2i) [%2i]%s[%s:%4d]  ", getTimestamp(), tid, Config::getSingleton()->getCurrentProcessNum(), core_id, (sim_thread ? "* " : "  "), module.c_str(), source_line);
   else if (Config::getSingleton()->getCurrentProcessNum() != (UInt32)-1) // valid proc id
      fprintf(file, "%-10llu [%5d]  (%2i) [  ]  [%s:%4d]  ", getTimestamp(), tid, Config::getSingleton()->getCurrentProcessNum(), module.c_str(), source_line);
   else // who knows
      fprintf(file, "%-10llu [%5d]  (  ) [  ]  [%s:%4d]  ", getTimestamp(), tid, module.c_str(), source_line);

   switch (err)
   {
   case None:
   default:
      break;

   case Warning:
      fprintf(file, "*WARNING* ");
      break;

   case Error:
      fprintf(file, "*ERROR* ");
      break;
   };

   va_list args;
   va_start(args, format);
   vfprintf(file, format, args);
   va_end(args);

   fprintf(file, "\n");

   fflush(file);

   lock->release();

   if (err == Error)
      abort();
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
   abort();
}
