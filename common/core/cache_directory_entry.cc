#include "cache_directory_entry.h"

CacheDirectoryEntry::CacheDirectoryEntry() : cstate(CacheDirectoryEntry::INVALID)
{}

CacheDirectoryEntry::~CacheDirectoryEntry()
{}

void setCState(c_state_t state)
{
   c_state = state;
}

bool CacheDirectoryEntry::readable() {
   return (cstate == CacheDirectoryEntry::EXCLUSIVE) || (cstate == CacheDirectoryEntry::SHARED);
}

bool CacheDirectoryEntry::writable() {
   return (cstate == CacheDirectoryEntry::EXCLUSIVE);
}

void CacheDirectoryEntry::toString()
{
   cout << "state= " << cstate << endl;
   return;
}
