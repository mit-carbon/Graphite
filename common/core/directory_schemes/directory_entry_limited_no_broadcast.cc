#include "directory_entry_limited_no_broadcast.h"

using namespace std;

DirectoryEntryLimitedNoBroadcast::DirectoryEntryLimitedNoBroadcast(
      UInt32 max_hw_sharers, 
      UInt32 max_num_sharers):
   DirectoryEntry(max_hw_sharers, max_num_sharers)
{}

DirectoryEntryLimitedNoBroadcast::~DirectoryEntryLimitedNoBroadcast()
{}

bool
DirectoryEntryLimitedNoBroadcast::hasSharer(SInt32 sharer_id)
{
   return m_sharers->at(sharer_id);
}

// Returns a pair 'val':
// val.first :- Says whether the sharer was successfully added
//              'True' if it was successfully added
//              'False' if there will be an eviction before adding
// val.second :- If there will be an eviction, 
//                  return the ID of the sharer that will be evicted
//               Else
//                  return -1
pair<bool, SInt32>
DirectoryEntryLimitedNoBroadcast::addSharer(SInt32 sharer_id)
{
   assert(! m_sharers->at(sharer_id));
   if (m_sharers->size() == m_max_hw_sharers)
   {
      return make_pair(false, getNextEvictionId());
   }

   m_sharers->set(sharer_id);
   return make_pair(true, -1);
}

void
DirectoryEntryLimitedNoBroadcast::removeSharer(SInt32 sharer_id)
{
   assert(m_sharers->at(sharer_id));
   m_sharers->clear(sharer_id);
}

UInt32
DirectoryEntryLimitedNoBroadcast::numSharers()
{
   return m_sharers->size();
}

SInt32
DirectoryEntryLimitedNoBroadcast::getOwner()
{
   assert(m_sharers->at(m_owner_id));
   return m_owner_id;
}

void
DirectoryEntryLimitedNoBroadcast::setOwner(SInt32 owner_id)
{
   if (owner_id != INVALID_CORE_ID)
      assert(m_sharers->at(owner_id));
   m_owner_id = owner_id;
}

pair<bool, vector<SInt32> >
DirectoryEntryLimitedNoBroadcast::getSharersList()
{
   vector<SInt32> sharers_list(m_sharers->size());

   m_sharers->resetFind();

   SInt32 new_sharer = -1;
   SInt32 i = 0;
   while ((new_sharer = m_sharers->find()) != -1)
   {
      sharers_list[i] = new_sharer;
      i++;
      assert (i <= (SInt32) m_sharers->size());
   }

   return make_pair(false, sharers_list);
}

SInt32 
DirectoryEntryLimitedNoBroadcast::getNextEvictionId()
{
   vector<SInt32> sharers_list = getSharersList().second;
   assert(sharers_list.size() > 0);
   return sharers_list[0];
}

UInt32
DirectoryEntryLimitedNoBroadcast::getLatency()
{
   return 0;
}
