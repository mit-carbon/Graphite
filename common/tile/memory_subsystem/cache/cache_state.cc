#include "cache_state.h"
#include "log.h"

string
CacheState::getName(Type type)
{
   switch (type)
   {
   case INVALID:
      return "INVALID";
   case SHARED:
      return "SHARED";
   case OWNED:
      return "OWNED";
   case EXCLUSIVE:
      return "EXCLUSIVE";
   case MODIFIED:
      return "MODIFIED";
   case DATA_INVALID:
      return "DATA_INVALID";
   case CLEAN:
      return "CLEAN";
   case DIRTY:
      return "DIRTY";
   default:
      LOG_PRINT_ERROR("Unrecognized cache state(%u)", type);
      return "";
   }
}
