#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <set>
#include <string>
#include "fixed_types.h"

class Lock;

class Log
{
public:
   Log(UInt32 coreCount);
   ~Log();

   static Log *getSingleton();

   void log(UInt32 rank, const char* module, const char* format, ...);
   void notifyWarning();
   void notifyError();

   Boolean isEnabled(const char* module);

private:
   UInt64 getTimestamp();

   void getFile(UInt32 core_id, FILE ** f, Lock ** l);

   enum ErrorState
   {
      None,
      Warning,
      Error,
   };
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

   static Log *_singleton;
};

// Macros

#undef LOG_DEFAULT_RANK
#undef LOG_DEFAULT_MODULE

#ifdef NDEBUG

// see assert.h

#define LOG_PRINT_EXPLICIT(rank, module, ...)  ((void)(0))
#define LOG_PRINT(...) ((void)(0))
#define LOG_NOTIFY_WARNING() ((void)(0))
#define LOG_NOTIFY_ERROR() ((void)(0))
#define LOG_ASSERT_WARNING(...) ((void)(0))
#define LOG_ASSERT_ERROR(...) ((void)(0))

#else

#define LOG_PRINT_EXPLICIT(rank, module, ...)                           \
   {                                                                    \
      if (Log::getSingleton()->isEnabled(#module))                      \
      {                                                                 \
         Log::getSingleton()->log(rank, #module, __VA_ARGS__);          \
      }                                                                 \
   }                                                                    \

#define LOG_PRINT_EXPLICIT2(rank, module, ...)          \
   LOG_PRINT_EXPLICIT(rank, module, __VA_ARGS__)        \

#define LOG_PRINT(...)                                                  \
   LOG_PRINT_EXPLICIT2(LOG_DEFAULT_RANK, LOG_DEFAULT_MODULE, __VA_ARGS__) \

#define LOG_NOTIFY_WARNING()                    \
   {                                            \
      Log::getSingleton()->notifyWarning();     \
   }                                            \

#define LOG_NOTIFY_ERROR()                      \
   {                                            \
      Log::getSingleton()->notifyError();       \
      abort();                                  \
   }                                            \

#define LOG_ASSERT_WARNING(expr, ...)                   \
   {                                                    \
      if (!(expr))                                      \
      {                                                 \
         LOG_NOTIFY_WARNING();                          \
         LOG_PRINT(__VA_ARGS__);                        \
      }                                                 \
   }                                                    \

#define LOG_ASSERT_ERROR(expr, ...)                     \
   {                                                    \
      if (!(expr))                                      \
      {                                                 \
         LOG_NOTIFY_ERROR();                            \
         LOG_PRINT(__VA_ARGS__);                        \
      }                                                 \
   }                                                    \

#endif // NDEBUG

#endif // LOG_H
