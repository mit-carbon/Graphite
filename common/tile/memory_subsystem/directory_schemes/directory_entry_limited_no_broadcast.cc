#include "directory_entry_limited_no_broadcast.h"
#include "log.h"

DirectoryEntryLimitedNoBroadcast::DirectoryEntryLimitedNoBroadcast(SInt32 max_hw_sharers)
   : DirectoryEntryLimited(max_hw_sharers)
{}

DirectoryEntryLimitedNoBroadcast::~DirectoryEntryLimitedNoBroadcast()
{}

void
DirectoryEntryLimitedNoBroadcast::removeSharer(tile_id_t sharer_id, bool reply_expected)
{
   LOG_ASSERT_ERROR(!reply_expected, "reply_expected(true)");
   DirectoryEntryLimited::removeSharer(sharer_id);
}

UInt32
DirectoryEntryLimitedNoBroadcast::getLatency()
{
   return 0;
}
