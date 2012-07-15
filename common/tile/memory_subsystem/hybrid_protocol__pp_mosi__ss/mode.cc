#include "mode.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

string
Mode::getName(Type type)
{
   switch(type)
   {
   case INVALID:
      return "INVALID";
   case PRIVATE:
      return "PRIVATE";
   case REMOTE_LINE:
      return "REMOTE_LINE";
   case REMOTE_SHARER:
      return "REMOTE_SHARER";
   default:
      LOG_PRINT_ERROR("Unrecognized mode(%u)", type);
      return "";
   }
}

}
