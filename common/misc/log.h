#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <set>
#include <string>
#include <map>
#include "fixed_types.h"

class Lock;

class Log
{
   public:
      Log(UInt32 coreCount);
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

      void notifyWarning();
      void notifyError();
      void discoverCore(UInt32 *core_id, bool *sim_thread);
      void getFile(UInt32 core_id, bool sim_thread, FILE ** f, Lock ** l);
      std::string getModule(const char *filename);

      ErrorState _state;

      // when core id is known
      FILE** _coreFiles;
      Lock** _coreLocks;

      // when core is id unknown but process # is
      FILE** _systemFiles;
      Lock** _systemLocks;

      // when both no. procs and core id are unknown
      // there is the possibility of race conditions and stuff being
      // overwritten between multiple processes for this file
      FILE *_defaultFile;
      Lock *_defaultLock;

      UInt32 _coreCount;
      std::set<std::string> _disabledModules;
      std::map<const char*, std::string> _modules;
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
      if (Log::getSingleton()->isEnabled(__FILE__))                     \
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

#endif // LOG_H
