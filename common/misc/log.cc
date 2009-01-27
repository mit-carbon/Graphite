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
   _files = new FILE* [2 * _coreCount + 1];
   for (UInt32 i = 0; i < _coreCount; i++)
   {
      sprintf(filename, "output_files/app_%u", i);
      _files[i] = fopen(filename, "w");
      assert(_files[i] != NULL);
   }
   for (UInt32 i = _coreCount; i < 2 * _coreCount; i++)
   {
      sprintf(filename, "output_files/sim_%u", i-_coreCount);
      _files[i] = fopen(filename, "w");
      assert(_files[i] != NULL);
   }
   _files[_coreCount * 2] = fopen("output_files/system", "w");
   assert(_files[_coreCount * 2]);

   _locks = new Lock* [2 * _coreCount + 1];
   for (UInt32 i = 0; i < 2 * _coreCount + 1; i++)
   {
      _locks[i] = Lock::create();
   }

   g_config->getDisabledLogModules(_disabledModules);

   assert(_singleton == NULL);
   _singleton = this;
}

Log::~Log()
{
   _singleton = NULL;

   for (UInt32 i = 0; i < 2 * _coreCount + 1; i++)
   {
      fclose(_files[i]);
      delete _locks[i];
   }
   delete [] _files;
   delete [] _locks;
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
class Chip;
extern Chip* g_chip;
SInt32 chipRank(SInt32 *);

void Log::log(UInt32 rank, const char *module, const char *format, ...)
{
#ifdef DISABLE_LOGGING
   return;
#endif

   UInt32 fileRank;
   if (rank == (UInt32)-1)
      fileRank = 2 * _coreCount;
   else
   {
      // FIXME: This is an ugly hack. Net/shared mem threads do not have
      // a rank, so we can use chipRank to discover which thread we are
      // on.

      if (g_chip == NULL) 
      {         
         fileRank = rank + _coreCount;
      }
      else
      {
         SInt32 myChipRank;
         chipRank(&myChipRank);
         if (myChipRank == -1)
            fileRank = rank + _coreCount;
         else
            fileRank = rank;
      }
   }
   
   if (!isEnabled(module))
      return;

   _locks[fileRank]->acquire();

   fprintf(_files[fileRank], "%llu [%i] [%s] ", getTimestamp(), (rank > _coreCount ? -1 : (SInt32)rank), module);
   
   va_list args;
   va_start(args, format);
   vfprintf(_files[fileRank], format, args);
   va_end(args);

   fprintf(_files[fileRank], "\n");

   fflush(_files[fileRank]);

   _locks[fileRank]->release();
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

