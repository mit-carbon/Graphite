#include <sys/time.h>
#include <sys/syscall.h>
#include <stdarg.h>
#include <string.h>

#include "log.h"
#include "config.h"
#include "simulator.h"
#include "tile_manager.h"

using namespace std;

Log *Log::_singleton;

const size_t Log::MODULE_LENGTH;

static string formatFileName(const char* s)
{
   return Sim()->getConfig()->formatOutputFileName(s);
}

Log::Log(Config &config)
   : _tileCount(config.getTotalTiles())
   , _startTime(0)
{
   assert(Config::getSingleton()->getProcessCount() != 0);

   initFileDescriptors();
   getEnabledModules();
   getDisabledModules();

   _loggingEnabled = initIsLoggingEnabled();

   assert(_singleton == NULL);
   _singleton = this;
}

Log::~Log()
{
   _singleton = NULL;

   for (tile_id_t i = 0; i < _tileCount; i++)
   {
      if (_tileFiles[i])
         fclose(_tileFiles[i]);
      if (_simFiles[i])
         fclose(_simFiles[i]);
   }

   delete [] _tileLocks;
   delete [] _simLocks;
   delete [] _tileFiles;
   delete [] _simFiles;

   if (_systemFile)
      fclose(_systemFile);
   fclose(_defaultFile);
}

Log* Log::getSingleton()
{
   assert(_singleton);
   return _singleton;
}

bool Log::isEnabled(const char* module)
{
   // either the module is specifically enabled, or all logging is
   // enabled and this one isn't disabled
   return (
            (_enabledModules.find(module) != _enabledModules.end())
             ||
            (_loggingEnabled && (_disabledModules.find(module) == _disabledModules.end()))
          );
}

bool Log::isLoggingEnabled()
{
   return (!_enabledModules.empty()) || _loggingEnabled;
}

void Log::initFileDescriptors()
{
   _tileFiles = new FILE* [_tileCount];
   _simFiles = new FILE* [_tileCount];

   for (tile_id_t i = 0; i < _tileCount; i++)
   {
      _tileFiles[i] = NULL;
      _simFiles[i] = NULL;
   }

   _tileLocks = new Lock [_tileCount];
   _simLocks = new Lock [_tileCount];

   _systemFile = NULL;

   _defaultFile = fopen(formatFileName("system-default.log").c_str(),"w");
}

void Log::parseModules(set<string> &mods, string list)
{
   string delimiters = " ";

   string::size_type lastPos = list.find_first_not_of(delimiters, 0);
   string::size_type pos     = list.find_first_of(delimiters, lastPos);

   set<string> unformatted;

   while (string::npos != pos || string::npos != lastPos)
   {
      unformatted.insert(list.substr(lastPos, pos - lastPos));
      lastPos = list.find_first_not_of(delimiters, pos);
      pos = list.find_first_of(delimiters, lastPos);
   }

   for (set<string>::iterator it = unformatted.begin();
        it != unformatted.end();
        it++)
   {
      string formatted;

      for (unsigned int i = 0; i < min(MODULE_LENGTH, it->length()); i++)
      {
         formatted.push_back((*it)[i]);
      }

      for (unsigned int i = formatted.length(); i < MODULE_LENGTH; i++)
      {
         formatted.push_back(' ');
      }

      assert(formatted.length() == MODULE_LENGTH);
      mods.insert(formatted);
   }
}

void Log::getDisabledModules()
{
   try 
   {
      string disabledModules = Sim()->getCfg()->getString("log/disabled_modules", "");
      parseModules(_disabledModules, disabledModules);
   }
   catch (...)
   {
      // FIXME: is log initialized at this point?
      LOG_PRINT_ERROR("Exception while reading disabled modules.");
   }
}

void Log::getEnabledModules()
{
   try 
   {
      string enabledModules = Sim()->getCfg()->getString("log/enabled_modules", "");
      parseModules(_enabledModules, enabledModules);
   }
   catch (...)
   {
      // FIXME: is log initialized at this point?
      LOG_PRINT_ERROR("Exception while reading enabled modules.");
   }
}

bool Log::initIsLoggingEnabled()
{
   try
   {
      return Sim()->getCfg()->getBool("log/enabled",false);
   }
   catch (...)
   {
      assert(false);
      return false;
   }
}

UInt64 Log::getTimestamp()
{
   timeval t;
   gettimeofday(&t, NULL);
   UInt64 time = (((UInt64)t.tv_sec) * 1000000 + t.tv_usec);
   if (_startTime == 0) _startTime = time;
   return time - _startTime;
}

void Log::discoverCore(tile_id_t *tile_id, bool *sim_thread)
{
   TileManager *tile_manager;

   if ( !Sim() || !(tile_manager = Sim()->getTileManager()) )
   {

      *tile_id = INVALID_TILE_ID;
      *sim_thread = false;
   }
   else
   {
      *tile_id = tile_manager->getCurrentTileID();
      *sim_thread = tile_manager->amiSimThread();
   }
}

void Log::getFile(tile_id_t tile_id, bool sim_thread, FILE **file, Lock **lock)
{
   // we use on-demand file allocation to prevent contention between
   // processes for files

   *file = NULL;
   *lock = NULL;

   if (tile_id == INVALID_TILE_ID)
   {
      // System file -- use process num if available
      UInt32 procNum = Config::getSingleton()->getCurrentProcessNum();
      
      if (procNum != (UInt32)-1)
      {
         if (_systemFile == NULL)
         {
            assert(procNum < Config::getSingleton()->getProcessCount());
            char filename[256];
            sprintf(filename, "system_%u.log", procNum);
            _systemFile = fopen(formatFileName(filename).c_str(), "w");
            assert(_systemFile != NULL);
         }

         *file = _systemFile;
         *lock = &_systemLock;
      }
      else
      {
         *file = _defaultFile;
         *lock = &_defaultLock;
      }
   }
   else if (sim_thread)
   {
      // sim thread file
      if (_simFiles[tile_id] == NULL)
      {
         assert(tile_id < _tileCount);
         char filename[256];
         sprintf(filename, "sim_%u.log", tile_id);
         _simFiles[tile_id] = fopen(formatFileName(filename).c_str(), "w");
         assert(_simFiles[tile_id] != NULL);
      }

      *file = _simFiles[tile_id];
      *lock = &_simLocks[tile_id];
   }
   else
   {
      // tile file
      if (_tileFiles[tile_id] == NULL)
      {
         assert(tile_id < _tileCount);
         char filename[256];
         sprintf(filename, "app_%u.log", tile_id);
         _tileFiles[tile_id] = fopen(formatFileName(filename).c_str(), "w");
         assert(_tileFiles[tile_id] != NULL);
      }

      // Tile file
      *file = _tileFiles[tile_id];
      *lock = &_tileLocks[tile_id];
   }
}

string Log::getModule(const char *filename)
{
   // TODO: Give each thread a _module map to cache entries. (Hash table?)

   // ScopedLock sl(_modules_lock);
   // map<const char*, string>::const_iterator it = _modules.find(filename);

   // if (it != _modules.end())
   // {
   //    return it->second;
   // }
   // else
   // {
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

   //   pair<const char*, string> p(filename, mod);
   //   _modules.insert(p);

      return mod;
   // }
}

void Log::log(ErrorState err, const char* source_file, SInt32 source_line, const char *format, ...)
{
   tile_id_t tile_id;
   bool sim_thread;
   discoverCore(&tile_id, &sim_thread);
   
   FILE *file;
   Lock *lock;

   getFile(tile_id, sim_thread, &file, &lock);
   int tid = syscall(__NR_gettid);


   char message[512];
   char *p = message;

   // This is ugly, but it just prints the time stamp, process number, tile number, source file/line
   if (tile_id != INVALID_TILE_ID) // valid tile id
      p += sprintf(p, "%-10llu [%5d]  (%2i) [%2i]%s[%s:%4d]  ", (long long unsigned int) getTimestamp(), tid, Config::getSingleton()->getCurrentProcessNum(), tile_id, (sim_thread ? "* " : "  "), source_file, source_line);
   else if (Config::getSingleton()->getCurrentProcessNum() != (UInt32)-1) // valid proc id
      p += sprintf(p, "%-10llu [%5d]  (%2i) [  ]  [%s:%4d]  ", (long long unsigned int) getTimestamp(), tid, Config::getSingleton()->getCurrentProcessNum(), source_file, source_line);
   else // who knows
      p += sprintf(p, "%-10llu [%5d]  (  ) [  ]  [%s:%4d]  ", (long long unsigned int) getTimestamp(), tid, source_file, source_line);

   switch (err)
   {
   case None:
   default:
      break;

   case Warning:
      p += sprintf(p, "*WARNING* ");
      break;

   case Error:
      p += sprintf(p, "*ERROR* ");
      break;
   };

   va_list args;
   va_start(args, format);
   p += vsprintf(p, format, args);
   va_end(args);

   p += sprintf(p, "\n");

   lock->acquire();

   fputs(message, file);
   fflush(file);

   lock->release();

   switch (err)
   {
   case Error:
      fputs(message, stderr);
      abort();
      break;

   case Warning:
      fputs(message, stderr);
      break;

   case None:
   default:
      break;
   }
}
