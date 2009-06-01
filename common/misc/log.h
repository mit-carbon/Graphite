#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <set>
#include <string>
#include <map>
#include "fixed_types.h"
#include "lock.h"

#define LOCK_LOGS

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

      Boolean isEnabled(const char* module);

   private:
      UInt64 getTimestamp();

      void discoverCore(core_id_t *core_id, bool *sim_thread);
      void getFile(core_id_t core_id, bool sim_thread, FILE ** f, Lock ** l);
      std::string getModule(const char *filename);

      ErrorState _state;

      // when core id is known
      FILE** _coreFiles;
      FILE** _simFiles;
      Lock* _coreLocks;
      Lock *_simLocks;

      // when core is id unknown but process # is
      FILE* _systemFile;
      Lock _systemLock;

      // when both no. procs and core id are unknown
      // there is the possibility of race conditions and stuff being
      // overwritten between multiple processes for this file
      FILE *_defaultFile;
      Lock _defaultLock;

      core_id_t _coreCount;
      UInt64 _startTime;
      std::set<std::string> _disabledModules;
      bool _loggingEnabled;
      std::map<const char*, std::string> _modules;

// By defining LOCK_LOGS we ensure no race conditions on the modules
// map above. In practice this isn't very likely and using locks
// suffers a high performance hit.
#ifdef LOCK_LOGS
      Lock _modules_lock;
#endif
      static const UInt32 MODULE_LENGTH = 10;

      static Log *_singleton;
};

// Macros

#ifdef NDEBUG

// see assert.h

#define LOG_PRINT(...) ((void)(0))
#define LOG_PRINT_WARNING(...) ((void)(0))
#define LOG_PRINT_ERROR(...) ((void)(0))
#define LOG_ASSERT_WARNING(...) ((void)(0))
#define LOG_ASSERT_ERROR(...) ((void)(0))

#else

#define _LOG_PRINT(err, ...)                                            \
   {                                                                    \
      if (err == Log::Error ||                                          \
          Log::getSingleton()->isEnabled(__FILE__))                     \
      {                                                                 \
         Log::getSingleton()->log(err, __FILE__, __LINE__, __VA_ARGS__); \
      }                                                                 \
   }                                                                    \
 
#define LOG_PRINT(...)                                                  \
   _LOG_PRINT(Log::None, __VA_ARGS__);                                   \
 
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
   FunctionTracer(const char *fn)
      : m_fn(fn)
   {
      LOG_PRINT("Entering: %s", m_fn);
   }

   ~FunctionTracer()
   {
      LOG_PRINT("Exiting:  %s", m_fn);
   }

private:
   const char *m_fn;
};

#define LOG_FUNC_TRACE()   FunctionTracer func_tracer(__PRETTY_FUNCTION__);

#endif // LOG_H
