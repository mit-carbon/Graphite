#include "cache_directory_entry.h"

CacheDirectoryEntry::CacheDirectoryEntry() : cstate(CacheDirectoryEntry::INVALID)
{}

CacheDirectoryEntry::~CacheDirectoryEntry()
{}

void CacheDirectoryEntry::setCState(cstate_t state)
{
   cstate = state;
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
