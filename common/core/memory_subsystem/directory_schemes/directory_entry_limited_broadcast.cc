#include "directory_entry_limited_broadcast.h"
#include "log.h"

using namespace std;

DirectoryEntryLimitedBroadcast::DirectoryEntryLimitedBroadcast(
      UInt32 max_hw_sharers, 
      UInt32 max_num_sharers):
   DirectoryEntry(max_hw_sharers, max_num_sharers),
   m_global_enabled(false),
   m_num_untracked_sharers(0)
{}

DirectoryEntryLimitedBroadcast::~DirectoryEntryLimitedBroadcast()
{}

bool
DirectoryEntryLimitedBroadcast::hasSharer(core_id_t sharer_id)
{
   return m_sharers->at(sharer_id);
}

// Returns: Says whether the sharer was successfully added
//              'True' if it was successfully added
//              'False' if there will be an eviction before adding
bool
DirectoryEntryLimitedBroadcast::addSharer(core_id_t sharer_id)
{
   if (m_global_enabled)
   {
      m_num_untracked_sharers ++;
   }
   else
   {
      assert(! m_sharers->at(sharer_id));
      if (m_sharers->size() == m_max_hw_sharers)
      {
         m_global_enabled = true;
         m_num_untracked_sharers = 1;
      }
      else
      {
         m_sharers->set(sharer_id);
      }
   }

   return true;
}

void
DirectoryEntryLimitedBroadcast::removeSharer(core_id_t sharer_id)
{
   if (m_global_enabled)
   {
      assert(m_num_untracked_sharers > 0);
      if (m_sharers->at(sharer_id))
      {
         m_sharers->clear(sharer_id);
      }
      else
      {
         m_num_untracked_sharers --;
         if (m_num_untracked_sharers == 0)
            m_global_enabled = false;
      }
   }
   else
   {
      assert(m_sharers->at(sharer_id));
      m_sharers->clear(sharer_id);
   }
}

UInt32
DirectoryEntryLimitedBroadcast::getNumSharers()
{
   if (m_global_enabled)
      return m_sharers->size() + m_num_untracked_sharers;
   else
      return m_sharers->size();
}

core_id_t
DirectoryEntryLimitedBroadcast::getOwner()
{
   return m_owner_id;
}

void
DirectoryEntryLimitedBroadcast::setOwner(core_id_t owner_id)
{
   if (owner_id != INVALID_CORE_ID)
   {
      LOG_ASSERT_ERROR(m_sharers->at(owner_id),
            "owner_id(%i), m_owner_id(%i), num sharers(%u), one sharer(%i)",
            owner_id, m_owner_id, getNumSharers(), getOneSharer());
   }
   m_owner_id = owner_id;
}

core_id_t
DirectoryEntryLimitedBroadcast::getOneSharer()
{
   m_sharers->resetFind();
   core_id_t sharer_id = m_sharers->find();
   assert((sharer_id == -1) == (m_sharers->size() == 0));
   if (sharer_id != -1)
      return sharer_id;
   else
      return INVALID_CORE_ID;
}

// Return a pair:
// val.first :- 'True' if all cores are sharers
//              'False' if NOT all cores are sharers
// val.second :- 'Empty' if all cores are sharers
//               A list of sharers if NOT all cores are sharers
pair<bool, vector<core_id_t> >&
DirectoryEntryLimitedBroadcast::getSharersList()
{
   if (m_global_enabled)
   {
      assert(m_num_untracked_sharers > 0);
      m_cached_sharers_list.first = true;
      m_cached_sharers_list.second.clear();
      return m_cached_sharers_list;
   }
   else
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
}

UInt32
DirectoryEntryLimitedBroadcast::getLatency()
{
   return 0;
}

