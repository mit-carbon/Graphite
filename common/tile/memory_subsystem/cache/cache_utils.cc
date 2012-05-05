#include "cache_utils.h"

UInt64 computeLifetime(UInt64 birth_time, UInt64 curr_time)
{ return (curr_time > birth_time) ? (curr_time - birth_time) : 0; }
