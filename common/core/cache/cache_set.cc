#include "cache_set.h"
#include "log.h"

CacheSetBase* 
CacheSetBase::createModel (ReplacementPolicy replacement_policy,
      UInt32 associativity, UInt32 blocksize)
{
   switch(replacement_policy)
   {
      case ROUND_ROBIN:
         return new RoundRobin(associativity, blocksize);               
               
      default:
         LOG_PRINT_ERROR("Unrecognized Cache Replacement Policy: %i",
               replacement_policy);
         break;
   }

   return (CacheSetBase*) NULL;
}

CacheSetBase::ReplacementPolicy 
CacheSetBase::parsePolicyType(string policy)
{
   if (policy == "round_robin")
      return ROUND_ROBIN;
   else
      return (ReplacementPolicy) -1;
}
