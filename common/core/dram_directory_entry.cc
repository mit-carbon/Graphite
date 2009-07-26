#include "dram_directory_entry.h"
#include "simulator.h"

#include <string.h>

UInt32 DramDirectoryEntry::cache_line_size;
UInt32 DramDirectoryEntry::max_sharers;
UInt32 DramDirectoryEntry::total_cores;

DramDirectoryEntry::DramDirectoryEntry(IntPtr cache_line_address)
   :
      dstate(UNCACHED),
      exclusive_sharer_rank(0),
      number_of_sharers(0)
{
   this->cache_line_address = cache_line_address;

   this->sharers = new BitVector(DramDirectoryEntry::total_cores);
   this->cache_line = new Byte[DramDirectoryEntry::cache_line_size];

   //clear memory_line
   memset(this->cache_line, '\0', DramDirectoryEntry::cache_line_size);

   stat_min_sharers = 0;
   stat_max_sharers = 0;
   stat_access_count = 0;
   stat_avg_sharers = 0;
}

DramDirectoryEntry::DramDirectoryEntry(IntPtr cache_line_address, Byte* data_buffer)
   :
      dstate(UNCACHED),
      exclusive_sharer_rank(0),
      number_of_sharers(0)
{
   this->cache_line_address = cache_line_address;

   this->sharers = new BitVector(DramDirectoryEntry::total_cores);
   this->cache_line = new Byte[DramDirectoryEntry::cache_line_size];

   //copy cache_line
   memcpy(this->cache_line, data_buffer, DramDirectoryEntry::cache_line_size);

   stat_min_sharers = 0;
   stat_max_sharers = 0;
   stat_access_count = 0;
   stat_avg_sharers = 0;

}

DramDirectoryEntry::~DramDirectoryEntry()
{
   assert (sharers != NULL);
   delete sharers;
   delete [] cache_line;
}

// Static Functions - To set static variables
void DramDirectoryEntry::setCacheLineSize(UInt32 cache_line_size_arg)
{
   cache_line_size = cache_line_size_arg;
}

void DramDirectoryEntry::setMaxSharers(UInt32 max_sharers_arg)
{
   max_sharers = max_sharers_arg;
}

void DramDirectoryEntry::setTotalCores(UInt32 total_cores_arg)
{
   total_cores = total_cores_arg;
}

void DramDirectoryEntry::fillDramDataLine(Byte* data_buffer)
{
   assert(data_buffer != NULL);
   assert(cache_line != NULL);

   memcpy(cache_line, data_buffer, DramDirectoryEntry::cache_line_size);

}

void DramDirectoryEntry::getDramDataLine(Byte* data_buffer)
{
   assert(data_buffer != NULL);
   assert(cache_line != NULL);

   memcpy(data_buffer, cache_line, DramDirectoryEntry::cache_line_size);
}

/*
 * addSharer() tries to add a core to the sharer list
 * Returns: Whether there was an eviction or not
 *  'true': If it cannot add (We have reached the maximum number of pointers in the
 *       sharer list. No side effects in this case)
 *  'false': If there was no eviction (In this case, it adds the sharer to the sharer list)
 */
bool DramDirectoryEntry::addSharer(UInt32 sharer_rank)
{
   assert(! sharers->at(sharer_rank));
   if (number_of_sharers == DramDirectoryEntry::max_sharers)
   {
      return (true);
   }

   sharers->set(sharer_rank);
   number_of_sharers++;
   return (false);
}

UInt32 DramDirectoryEntry::getEvictionId()
{
   vector<UInt32> sharers_list = getSharersList();
   assert(sharers_list.size() > 0);
   return sharers_list[0];
}

void DramDirectoryEntry::removeSharer(UInt32 sharer_rank)
{
   assert(sharers->at(sharer_rank));
   assert(number_of_sharers > 0);
   sharers->clear(sharer_rank);
   number_of_sharers--;
}

bool DramDirectoryEntry::hasSharer(UInt32 sharer_rank)
{
   return(sharers->at(sharer_rank));
}   

void DramDirectoryEntry::addExclusiveSharer(UInt32 sharer_rank)
{
   sharers->reset();
   sharers->set(sharer_rank);
   number_of_sharers = 1;
   exclusive_sharer_rank = sharer_rank;
}

DramDirectoryEntry::dstate_t DramDirectoryEntry::getDState()
{
   return dstate;
}

void DramDirectoryEntry::setDState(dstate_t new_dstate)
{
   assert((SInt32)(new_dstate) >= 0 && (SInt32)(new_dstate) < NUM_DSTATE_STATES);

   if ((new_dstate == UNCACHED) && (number_of_sharers != 0))
   {
      assert(false);
   }

   dstate = new_dstate;
}

/* Return the number of cores currently sharing this entry */
SInt32 DramDirectoryEntry::numSharers()
{
   return number_of_sharers;
}

UInt32 DramDirectoryEntry::getExclusiveSharerRank()
{
   assert(number_of_sharers == 1);
   assert(dstate = EXCLUSIVE);

   return exclusive_sharer_rank;
}

vector<UInt32> DramDirectoryEntry::getSharersList()
{
   vector<UInt32> sharers_list(number_of_sharers);

   sharers->resetFind();

   SInt32 new_sharer = -1;

   SInt32 i = 0;
   while ((new_sharer = sharers->find()) != -1)
   {
      sharers_list[i] = new_sharer;
      i++;
      assert (i <= (SInt32) number_of_sharers);
   }

   return sharers_list;
}

string DramDirectoryEntry::dStateToString(dstate_t dstate)
{
   switch (dstate)
   {
   case UNCACHED:
      return "UNCACHED  ";
   case EXCLUSIVE:
      return "EXCLUSIVE ";
   case SHARED :
      return "SHARED    ";
   default:
      return "ERROR: DSTATETOSTRING";
   }

   return "ERROR: DSTATETOSTRING";
}
