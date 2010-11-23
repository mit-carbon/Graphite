#include "directory_entry_limited_broadcast.h"
#include "config.h"
#include "log.h"

using namespace std;

DirectoryEntryLimitedBroadcast::DirectoryEntryLimitedBroadcast(
      UInt32 max_hw_sharers, 
      UInt32 max_num_sharers):
   DirectoryEntry(max_hw_sharers, max_num_sharers),
   m_global_enabled(false),
   m_num_sharers(0)
{}

DirectoryEntryLimitedBroadcast::~DirectoryEntryLimitedBroadcast()
{}

bool
DirectoryEntryLimitedBroadcast::hasSharer(tile_id_t sharer_id)
{
   return m_sharers->at(sharer_id);
}

// Returns: Says whether the sharer was successfully added
//              'True' if it was successfully added
//              'False' if there will be an eviction before adding
bool
DirectoryEntryLimitedBroadcast::addSharer(tile_id_t sharer_id)
{
   if (m_global_enabled)
   {
      assert(m_num_sharers == Config::getSingleton()->getTotalTiles());
   }
   else
   {
      assert(! m_sharers->at(sharer_id));
      if (m_sharers->size() == m_max_hw_sharers)
      {
         m_global_enabled = true;
         m_num_sharers = Config::getSingleton()->getTotalTiles();
      }
      else
      {
         m_sharers->set(sharer_id);
      }
   }
   return true;
}

void
DirectoryEntryLimitedBroadcast::removeSharer(tile_id_t sharer_id, bool reply_expected)
{
   if (m_global_enabled)
   {
      if (m_sharers->at(sharer_id))
      {
         m_sharers->clear(sharer_id);
      }
   }
   else
   {
      assert(m_sharers->at(sharer_id));
      m_sharers->clear(sharer_id);
   }

   if (reply_expected)
   {
      assert(m_global_enabled);
      m_num_sharers --;
      if (m_num_sharers == 0)
      {
         m_global_enabled = false;
         assert(m_sharers->size() == 0);
      }
   }
}

UInt32
DirectoryEntryLimitedBroadcast::getNumSharers()
{
   if (m_global_enabled)
      return Config::getSingleton()->getTotalTiles();
   else
      return m_sharers->size();
}

tile_id_t
DirectoryEntryLimitedBroadcast::getOwner()
{
   return m_owner_id;
}

void
DirectoryEntryLimitedBroadcast::setOwner(tile_id_t owner_id)
{
   if (owner_id != INVALID_TILE_ID)
   {
      LOG_ASSERT_ERROR(m_sharers->at(owner_id),
            "owner_id(%i), m_owner_id(%i), num sharers(%u), one sharer(%i)",
            owner_id, m_owner_id, getNumSharers(), getOneSharer());
   }
   m_owner_id = owner_id;
}

tile_id_t
DirectoryEntryLimitedBroadcast::getOneSharer()
{
   pair<bool, vector<tile_id_t> > sharers_list = getSharersList();
   assert(sharers_list.first || (sharers_list.second.size() > 0));
   if (sharers_list.second.size() > 0)
   {
      SInt32 index = m_rand_num.next(sharers_list.second.size());
      return sharers_list.second[index];
   }
   else
   {
      return INVALID_TILE_ID;
   }
}

// Return a pair:
// val.first :- 'True' if all tiles are sharers
//              'False' if NOT all tiles are sharers
// val.second :- 'Empty' if all tiles are sharers
//               A list of sharers if NOT all tiles are sharers
pair<bool, vector<tile_id_t> >&
DirectoryEntryLimitedBroadcast::getSharersList()
{
   if (m_global_enabled)
   {
      assert(m_num_sharers == Config::getSingleton()->getTotalTiles());
      m_cached_sharers_list.first = true;
   }
   else
   {
      m_cached_sharers_list.first = false;
   }

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
DirectoryEntryLimitedBroadcast::getLatency()
{
   return 0;
}

