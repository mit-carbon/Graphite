#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <set>
#include <string>
#include "fixed_types.h"

class Log
{
public:
   Log(UInt32 coreCount);
   ~Log();

   static Log *getSingleton();

   void log(UInt32 rank, const char* module, const char* format, ...);

   Boolean isEnabled(const char* module);

private:
   UInt64 getTimestamp();

   FILE ** _files;
   UInt32 _coreCount;
   std::set<std::string> _disabledModules;

   static Log *_singleton;
};

// Macros

#undef LOG_DEFAULT_RANK
#undef LOG_DEFAULT_MODULE

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

#endif // LOG_H
