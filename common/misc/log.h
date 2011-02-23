#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <set>
#include <string>
#include <map>
#include "fixed_types.h"
#include "lock.h"

class Config;

class Log
{
   public:
      Log(Config &config);
      ~Log();

      static Log *getSingleton();

      enum ErrorState
      {
         None,
         Warning,
         Error,
      };

      void log(ErrorState err, const char *source_file, SInt32 source_line, const char* format, ...);

      bool isEnabled(const char* module);
      bool isLoggingEnabled();
      std::string getModule(const char *filename);

   private:
      UInt64 getTimestamp();

      void initFileDescriptors();
      static void parseModules(std::set<std::string> &mods, std::string list);
      void getDisabledModules();
      void getEnabledModules();
      bool initIsLoggingEnabled();

      void discoverCore(tile_id_t *tile_id, bool *sim_thread);
      void getFile(tile_id_t tile_id, bool sim_thread, FILE ** f, Lock ** l);

      ErrorState _state;

      // when tile id is known
      FILE** _tileFiles;
      FILE** _simFiles;
      Lock* _tileLocks;
      Lock *_simLocks;

      // when tile is id unknown but process # is
      FILE* _systemFile;
      Lock _systemLock;

      // when both no. procs and tile id are unknown
      // there is the possibility of race conditions and stuff being
      // overwritten between multiple processes for this file
      FILE *_defaultFile;
      Lock _defaultLock;

      tile_id_t _tileCount;
      UInt64 _startTime;
      std::set<std::string> _disabledModules;
      std::set<std::string> _enabledModules;
      bool _loggingEnabled;

      /* std::map<const char*, std::string> _modules; */
      /* Lock _modules_lock; */

      static const size_t MODULE_LENGTH = 10;

      static Log *_singleton;
};

// Macros

#ifdef NDEBUG

// see assert.h

#define __LOG_PRINT(...) ((void)(0))
#define _LOG_PRINT(...) ((void)(0))
#define LOG_PRINT(...) ((void)(0))
#define LOG_PRINT_WARNING(...) ((void)(0))
#define LOG_PRINT_ERROR(...) ((void)(0))
#define LOG_ASSERT_WARNING(...) ((void)(0))
#define LOG_ASSERT_ERROR(...) ((void)(0))

#else

#define __LOG_PRINT(err, file, line, ...)                               \
   {                                                                    \
      if (Log::getSingleton()->isLoggingEnabled() || err != Log::None)  \
      {                                                                 \
         std::string module = Log::getSingleton()->getModule(file);     \
         if (err != Log::None ||                                        \
             Log::getSingleton()->isEnabled(module.c_str()))            \
         {                                                              \
            Log::getSingleton()->log(err, module.c_str(), line, __VA_ARGS__); \
         }                                                              \
      }                                                                 \
   }                                                                    \

#define _LOG_PRINT(err, ...)                                            \
   {                                                                    \
   __LOG_PRINT(err, __FILE__, __LINE__, __VA_ARGS__);                   \
   }                                                                    \
 
#define LOG_PRINT(...)                                                  \
   _LOG_PRINT(Log::None, __VA_ARGS__);                                  \
 
#define LOG_PRINT_WARNING(...)                  \
   _LOG_PRINT(Log::Warning, __VA_ARGS__);
 
#define LOG_PRINT_ERROR(...)                    \
   _LOG_PRINT(Log::Error, __VA_ARGS__);

#define LOG_ASSERT_WARNING(expr, ...)                   \
   {                                                    \
      if (!(expr))                                      \
      {                                                 \
         LOG_PRINT_WARNING(__VA_ARGS__);                \
      }                                                 \
   }                                                    \
 
#define LOG_ASSERT_ERROR(expr, ...)                     \
   {                                                    \
      if (!(expr))                                      \
      {                                                 \
         LOG_PRINT_ERROR(__VA_ARGS__);                  \
      }                                                 \
   }                                                    \
 
#endif // NDEBUG

// Helpers

class FunctionTracer
{
public:
   FunctionTracer(const char *file, int line, const char *fn)
      : m_file(file)
      , m_line(line)
      , m_fn(fn)
   {
      __LOG_PRINT(Log::None, m_file, m_line, "Entering: %s", m_fn);
   }

   ~FunctionTracer()
   {
      __LOG_PRINT(Log::None, m_file, m_line, "Exiting: %s", m_fn);
   }

private:
   const char *m_file;
   int m_line;
   const char *m_fn;
};

#define LOG_FUNC_TRACE()   FunctionTracer func_tracer(__FILE__,__LINE__,__PRETTY_FUNCTION__);

#endif // LOG_H
