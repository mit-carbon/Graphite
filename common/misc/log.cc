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

   assert(Config::getSingleton()->getProcessCount() != 0);

   _systemFiles = new FILE* [Config::getSingleton()->getProcessCount()];
   _systemLocks = new Lock* [Config::getSingleton()->getProcessCount()];
   for (UInt32 i = 0; i < Config::getSingleton()->getProcessCount(); i++)
   {
      sprintf(filename, "output_files/system_%u", i);
      _systemFiles[i] = fopen(filename, "w");
      assert(_systemFiles[i] != NULL);

      _systemLocks[i] = Lock::create();
   }

   _defaultFile = fopen("output_files/system-default","w");
   _defaultLock = Lock::create();

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
      delete _coreLocks[i];
   }

   delete [] _coreLocks;
   delete [] _coreFiles;

   for (UInt32 i = 0; i < Config::getSingleton()->getProcessCount(); i++)
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
   return _disabledModules.find(module) == _disabledModules.end();
}

UInt64 Log::getTimestamp()
{
   UInt64 hi, lo;
   __asm__ __volatile__ ("rdtsc\n"
                         "movl %%edx,%0\n"
                         "movl %%eax,%1" : "=r"(hi), "=r"(lo) : : "edx", "eax");
   return ((UInt64) hi << 32ULL) + (UInt64) lo;
}

void Log::discoverCore(UInt32 *core_id, bool *sim_thread)
{
   CoreManager *core_manager;

   if (!Sim() || !(core_manager = Sim()->getCoreManager()))
   {
      *core_id = -1;
      *sim_thread = false;
      return;
   }

   *core_id = core_manager->getCurrentCoreID();
   if (*core_id != (UInt32)-1)
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

void Log::getFile(UInt32 core_id, bool sim_thread, FILE **file, Lock **lock)
{
   *file = NULL;
   *lock = NULL;

   if (core_id == (UInt32)-1)
   {
      // System file -- use process num if available
      if (Config::getSingleton()->getCurrentProcessNum() != (UInt32) -1)
      {
         assert(Config::getSingleton()->getCurrentProcessNum() < Config::getSingleton()->getProcessCount());
         *file = _systemFiles[Config::getSingleton()->getCurrentProcessNum()];
         *lock = _systemLocks[Config::getSingleton()->getCurrentProcessNum()];
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
      UInt32 fileID = core_id + (sim_thread ? _coreCount : 0);
      *file = _coreFiles[fileID];
      *lock = _coreLocks[fileID];
   }
}

std::string Log::getModule(const char *filename)
{
   std::map<const char*, std::string>::const_iterator it = _modules.find(filename);

   if (it != _modules.end())
   {
      return it->second;
   }
   else
   {
      // build module string
      string mod;

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

   UInt32 core_id;
   bool sim_thread;
   discoverCore(&core_id, &sim_thread);
   
   if (!isEnabled(source_file))
      return;

   FILE *file;
   Lock *lock;

   getFile(core_id, sim_thread, &file, &lock);

   lock->acquire();

   std::string module = getModule(source_file);

   // This is ugly, but it just prints the time stamp, process number, core number, source file/line
   if (core_id != (UInt32)-1) // valid core id
      fprintf(file, "%llu {%2i}  [%2i]  [%s:%4d]%s", getTimestamp(), Config::getSingleton()->getCurrentProcessNum(), core_id, module.c_str(), source_line, (sim_thread ? "* " : "  "));
   else if (Config::getSingleton()->getCurrentProcessNum() != (UInt32)-1) // valid proc id
      fprintf(file, "%llu {%2i}  [  ]  [%s:%4d]  ", getTimestamp(), Config::getSingleton()->getCurrentProcessNum(), module.c_str(), source_line);
   else // who knows
      fprintf(file, "%llu {  }  [  ]  [%s:%4d]  ", getTimestamp(), module.c_str(), source_line);

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
