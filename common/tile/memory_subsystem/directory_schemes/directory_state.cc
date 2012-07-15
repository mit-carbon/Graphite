#include "directory_state.h"
#include "log.h"

string
DirectoryState::getName(Type type)
{
   switch (type)
   {
   case UNCACHED:
      return "UNCACHED";
   case SHARED:
      return "SHARED";
   case OWNED:
      return "OWNED";
   case EXCLUSIVE:
      return "EXCLUSIVE";
   case MODIFIED:
      return "MODIFIED";
   default:
      LOG_PRINT_ERROR("Unrecognized type(%u)", type);
      return "";
   }
}
