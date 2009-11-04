#include "directory_entry_limitless.h"

using namespace std;

DirectoryEntryLimitless::DirectoryEntryLimitless(
      UInt32 max_hw_sharers, 
      UInt32 max_num_sharers, 
      UInt32 software_trap_penalty):
   DirectoryEntry(max_hw_sharers, max_num_sharers),
   m_software_trap_enabled(false),
   m_software_trap_penalty(software_trap_penalty)
{}

DirectoryEntryLimitless::~DirectoryEntryLimitless()
{}

UInt32
DirectoryEntryLimitless::getLatency()
{
   if (m_software_trap_enabled)
      return m_software_trap_penalty;
   else
      return 0;
}

bool
DirectoryEntryLimitless::hasSharer(SInt32 sharer_id)
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
DirectoryEntryLimitless::addSharer(SInt32 sharer_id)
{
   assert(! m_sharers->at(sharer_id));

   // I have to calculate the latency properly here
   if (m_sharers->size() == m_max_hw_sharers)
   {
      m_software_trap_enabled = true;
   }

   m_sharers->set(sharer_id);
   return make_pair(true, -1);
}

void
DirectoryEntryLimitless::removeSharer(SInt32 sharer_id)
{
   assert(m_sharers->at(sharer_id));
   m_sharers->clear(sharer_id);
}

UInt32
DirectoryEntryLimitless::numSharers()
{
   return m_sharers->size();
}

SInt32
DirectoryEntryLimitless::getOwner()
{
   assert(m_sharers->at(m_owner_id));
   return m_owner_id;
}

void
DirectoryEntryLimitless::setOwner(SInt32 owner_id)
{
   if (owner_id != INVALID_CORE_ID)
      assert(m_sharers->at(owner_id));
   m_owner_id = owner_id;
}

pair<bool, vector<SInt32> >
DirectoryEntryLimitless::getSharersList()
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
