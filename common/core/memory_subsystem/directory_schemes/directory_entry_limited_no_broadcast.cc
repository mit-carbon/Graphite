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
DirectoryEntryLimitedNoBroadcast::hasSharer(core_id_t sharer_id)
{
   return m_sharers->at(sharer_id);
}

// Return value says whether the sharer was successfully added
//              'True' if it was successfully added
//              'False' if there will be an eviction before adding
bool
DirectoryEntryLimitedNoBroadcast::addSharer(core_id_t sharer_id)
{
   assert(! m_sharers->at(sharer_id));
   if (m_sharers->size() == m_max_hw_sharers)
   {
      return false;
   }

   m_sharers->set(sharer_id);
   return true;
}

void
DirectoryEntryLimitedNoBroadcast::removeSharer(core_id_t sharer_id, bool reply_expected)
{
   assert(!reply_expected);
   assert(m_sharers->at(sharer_id));
   m_sharers->clear(sharer_id);
}

UInt32
DirectoryEntryLimitedNoBroadcast::getNumSharers()
{
   return m_sharers->size();
}

core_id_t
DirectoryEntryLimitedNoBroadcast::getOwner()
{
   return m_owner_id;
}

void
DirectoryEntryLimitedNoBroadcast::setOwner(core_id_t owner_id)
{
   if (owner_id != INVALID_CORE_ID)
      assert(m_sharers->at(owner_id));
   m_owner_id = owner_id;
}

core_id_t
DirectoryEntryLimitedNoBroadcast::getOneSharer()
{
   m_sharers->resetFind();
   core_id_t sharer_id = m_sharers->find();
   assert(sharer_id != -1);
   return sharer_id;
}

pair<bool, vector<core_id_t> >&
DirectoryEntryLimitedNoBroadcast::getSharersList()
{
   m_cached_sharers_list.first = false;
   m_cached_sharers_list.second.resize(m_sharers->size());

   m_sharers->resetFind();

   core_id_t new_sharer = -1;
   SInt32 i = 0;
   while ((new_sharer = m_sharers->find()) != -1)
   {
      m_cached_sharers_list.second[i] = new_sharer;
      i++;
      assert (i <= (core_id_t) m_sharers->size());
   }

   return m_cached_sharers_list;
}

UInt32
DirectoryEntryLimitedNoBroadcast::getLatency()
{
   return 0;
}
