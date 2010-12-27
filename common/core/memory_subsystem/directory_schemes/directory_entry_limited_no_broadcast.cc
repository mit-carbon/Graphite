#include "directory_entry_limited_no_broadcast.h"
#include "simulator.h"
#include "log.h"

using namespace std;

DirectoryEntryLimitedNoBroadcast::DirectoryEntryLimitedNoBroadcast(
      UInt32 max_hw_sharers, 
      UInt32 max_num_sharers):
   DirectoryEntry(max_hw_sharers, max_num_sharers)
{}

DirectoryEntryLimitedNoBroadcast::~DirectoryEntryLimitedNoBroadcast()
{}

bool
DirectoryEntryLimitedNoBroadcast::hasSharer(tile_id_t sharer_id)
{
   return m_sharers->at(sharer_id);
}

// Return value says whether the sharer was successfully added
//              'True' if it was successfully added
//              'False' if there will be an eviction before adding
bool
DirectoryEntryLimitedNoBroadcast::addSharer(tile_id_t sharer_id)
{
   assert(! m_sharers->at(sharer_id) || Config::getSingleton()->getEnablePepCores());
   if (m_sharers->size() == m_max_hw_sharers)
   {
      return false;
   }

   m_sharers->set(sharer_id);
   return true;
}

void
DirectoryEntryLimitedNoBroadcast::removeSharer(tile_id_t sharer_id, bool reply_expected)
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

tile_id_t
DirectoryEntryLimitedNoBroadcast::getOwner()
{
   return m_owner_id;
}

void
DirectoryEntryLimitedNoBroadcast::setOwner(tile_id_t owner_id)
{
   if (owner_id != INVALID_TILE_ID)
      assert(m_sharers->at(owner_id));
   m_owner_id = owner_id;
}

tile_id_t
DirectoryEntryLimitedNoBroadcast::getOneSharer()
{
   pair<bool, vector<tile_id_t> > sharers_list = getSharersList();
   assert(!sharers_list.first);
   assert(sharers_list.second.size() > 0);

   SInt32 index = m_rand_num.next(sharers_list.second.size());
   return sharers_list.second[index];
}

pair<bool, vector<tile_id_t> >&
DirectoryEntryLimitedNoBroadcast::getSharersList()
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

UInt32
DirectoryEntryLimitedNoBroadcast::getLatency()
{
   return 0;
}
