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
DirectoryEntryLimitless::hasSharer(tile_id_t sharer_id)
{
   return m_sharers->at(sharer_id);
}

// Return value says whether the sharer was successfully added
//              'True' if it was successfully added
//              'False' if there will be an eviction before adding
bool
DirectoryEntryLimitless::addSharer(tile_id_t sharer_id)
{
   assert(! m_sharers->at(sharer_id));

   // I have to calculate the latency properly here
   if (m_sharers->size() == m_max_hw_sharers)
   {
      m_software_trap_enabled = true;
   }

   m_sharers->set(sharer_id);
   return true;;
}

void
DirectoryEntryLimitless::removeSharer(tile_id_t sharer_id, bool reply_expected)
{
   assert(!reply_expected);

   assert(m_sharers->at(sharer_id));
   m_sharers->clear(sharer_id);
}

UInt32
DirectoryEntryLimitless::getNumSharers()
{
   return m_sharers->size();
}

tile_id_t
DirectoryEntryLimitless::getOwner()
{
   return m_owner_id;
}

void
DirectoryEntryLimitless::setOwner(tile_id_t owner_id)
{
   if (owner_id != INVALID_TILE_ID)
      assert(m_sharers->at(owner_id));
   m_owner_id = owner_id;
}

tile_id_t
DirectoryEntryLimitless::getOneSharer()
{
   m_sharers->resetFind();
   tile_id_t sharer_id = m_sharers->find();
   assert(sharer_id != -1);
   return sharer_id;
}

pair<bool, vector<tile_id_t> >&
DirectoryEntryLimitless::getSharersList()
{
   m_cached_sharers_list.first = false;
   m_cached_sharers_list.second.resize(m_sharers->size());

   m_sharers->resetFind();

   tile_id_t new_sharer = -1;
   SInt32 i = 0;
   while ((new_sharer = m_sharers->find()) != -1)
   {
      m_cached_sharers_list.second[i] = new_sharer;
      i++;
      assert (i <= (tile_id_t) m_sharers->size());
   }

   return m_cached_sharers_list;
}
