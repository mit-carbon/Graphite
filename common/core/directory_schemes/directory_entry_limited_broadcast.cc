#include "directory_entry_limited_broadcast.h"

using namespace std;

DirectoryEntryLimitedBroadcast::DirectoryEntryLimitedBroadcast(
      UInt32 max_hw_sharers, 
      UInt32 max_num_sharers):
   DirectoryEntry(max_hw_sharers, max_num_sharers),
   m_global_enabled(false),
   m_number_of_sharers(0)
{}

DirectoryEntryLimitedBroadcast::~DirectoryEntryLimitedBroadcast()
{}

bool
DirectoryEntryLimitedBroadcast::hasSharer(SInt32 sharer_id)
{
   if (m_global_enabled)
   {
      // FIXME: This may be anything: Make sure we dont use it inappropriately
      return true;
   }
   else
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
DirectoryEntryLimitedBroadcast::addSharer(SInt32 sharer_id)
{
   if (m_global_enabled)
   {
      m_number_of_sharers ++;
   }
   else
   {
      assert(! m_sharers->at(sharer_id));
      if (m_sharers->size() == m_max_hw_sharers)
      {
         m_global_enabled = true;
         m_number_of_sharers = m_max_hw_sharers + 1;
         m_sharers->reset();
      }
      else
      {
         m_sharers->set(sharer_id);
      }
   }

   return make_pair(true, -1);
}

void
DirectoryEntryLimitedBroadcast::removeSharer(SInt32 sharer_id)
{
   if (m_global_enabled)
   {
      assert(m_number_of_sharers > 0);
      m_number_of_sharers --;
      if (m_number_of_sharers == 0)
         m_global_enabled = false;
   }
   else
   {
      assert(m_sharers->at(sharer_id));
      m_sharers->clear(sharer_id);
   }
}

UInt32
DirectoryEntryLimitedBroadcast::numSharers()
{
   if (m_global_enabled)
      return m_number_of_sharers;
   else
      return m_sharers->size();
}

SInt32
DirectoryEntryLimitedBroadcast::getOwner()
{
   if (m_global_enabled)
   {
      assert(m_number_of_sharers > 0);
   }
   else
   {
      assert(m_sharers->at(m_owner_id));
   }
   return m_owner_id;
}

void
DirectoryEntryLimitedBroadcast::setOwner(SInt32 owner_id)
{
   if (owner_id != INVALID_CORE_ID)
   {
      if (m_global_enabled)
      {
         assert(m_number_of_sharers > 0);
      }
      else
      {
         assert(m_sharers->at(owner_id));
      }
   }
   m_owner_id = owner_id;
}

// Return a pair:
// val.first :- 'True' if all cores are sharers
//              'False' if NOT all cores are sharers
// val.second :- 'Empty' if all cores are sharers
//               A list of sharers if NOT all cores are sharers
pair<bool, vector<SInt32> >
DirectoryEntryLimitedBroadcast::getSharersList()
{
   vector<SInt32> sharers_list;
   
   if (m_global_enabled)
   {
      assert(m_number_of_sharers > 0);
      return make_pair(true, sharers_list);
   }
   else
   {
      sharers_list.resize(m_sharers->size());
   
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
}

UInt32
DirectoryEntryLimitedBroadcast::getLatency()
{
   return 0;
}

