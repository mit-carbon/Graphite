// Jonathan Eastep (eastep@mit.edu) 
// 09.24.08
// Modified cache sets to actually store the data. 
//
//
// It's been significantly modified, but original code by Artur Klauser of
// Intel and modified by Rodric Rabbah. My changes enable the cache to be 
// dynamically resized (size and associativity) as well as some new statistics 
// tracking
//
// RMR (rodric@gmail.com) {
//   - temporary work around because decstr()
//     casts 64 bit ints to 32 bit ones
//   - use safe_fdiv to avoid NaNs in output


#ifndef CACHE_H
#define CACHE_H

#include <iostream>
#include <cassert>

#include "pin.H"
#include "utils.h"
#include "cache_state.h"

#define k_KILO 1024
#define k_MEGA (k_KILO*k_KILO)
#define k_GIGA (k_KILO*k_MEGA)

// type of cache hit/miss counters
typedef UINT64 CacheStats; 

extern LEVEL_BASE::KNOB<bool> g_knob_simarch_has_shared_mem;


namespace CACHE_ALLOC
{
   typedef enum 
   {
      k_STORE_ALLOCATE,
      k_STORE_NO_ALLOCATE
   } StoreAllocation;
};


// Cache tag - self clearing on creation

class CacheTag
{
   private:
      ADDRINT the_tag;
      CacheState::cstate_t the_cstate;
   
   public:
      CacheTag(ADDRINT tag = ~0, CacheState::cstate_t cstate = CacheState::INVALID) : 
         the_tag(tag), the_cstate(cstate) {}

      bool operator==(const CacheTag& right) const 
      { 
	 return (the_tag == right.the_tag);
      }

      bool isValid() const { return (the_tag != ((ADDRINT) ~0)); }

      operator ADDRINT() const { return the_tag; }

      CacheState::cstate_t getCState() { return the_cstate; }

      void setCState(CacheState::cstate_t cstate) { the_cstate = cstate; }
};



// Everything related to cache sets

namespace CACHE_SET
{

   // Cache set direct mapped
   class DirectMapped
   {
      private:
         CacheTag the_tag;

      public:
         DirectMapped(UINT32 assoc = 1) 
         { 
            assert(assoc == 1); 
            the_tag = CacheTag();
         }

         //FIXME: this should be private. should only be used during instantiation
         VOID setAssociativity(UINT32 assoc) { assert(assoc == 1); }

         UINT32 getAssociativity(UINT32 assoc) { return 1; }

         UINT32 find(CacheTag& tag) { return(the_tag == tag); }

         VOID replace(CacheTag& tag) { the_tag = tag; }

         VOID modifyAssociativity(UINT32 assoc) { assert(assoc == 1); }

         VOID print() { cout << the_tag << endl; }

         bool invalidateTag(CacheTag& tag) 
         { 
	    if ( tag == the_tag )
	    {
               the_tag = CacheTag(); 
               return true;
            }
            return false;
         }
   };


   // Cache set with round robin replacement
   
	// Cache set with round robin replacement
   template <UINT32 k_MAX_ASSOCIATIVITY = 8, 
             UINT32 k_MAX_BLOCKSIZE = 128>
   class RoundRobin
   {
      private:
         CacheTag the_tags[k_MAX_ASSOCIATIVITY];
         UINT32 tags_last_index;
         UINT32 next_replace_index;
         char the_blocks[k_MAX_ASSOCIATIVITY*k_MAX_BLOCKSIZE];
         UINT32 blocksize;

      public:

         RoundRobin(UINT32 assoc = k_MAX_ASSOCIATIVITY, UINT32 blksize = k_MAX_BLOCKSIZE): 
            tags_last_index(assoc - 1), blocksize(blksize)
         {
            assert(assoc <= k_MAX_ASSOCIATIVITY);
            assert(blocksize <= k_MAX_BLOCKSIZE);
            assert(tags_last_index < k_MAX_ASSOCIATIVITY);

				next_replace_index = tags_last_index;

            for (INT32 index = tags_last_index; index >= 0; index--)
            {
               the_tags[index] = CacheTag();
            }
            memset(&the_blocks[0], 0x00, k_MAX_BLOCKSIZE * k_MAX_ASSOCIATIVITY);
         }

         VOID setBlockSize(UINT32 blksize) { blocksize = blksize; }

         UINT32 getBlockSize() { return blocksize; }

         //FIXME: this should be private. should only be used during instantiation
         VOID setAssociativity(UINT32 assoc)
         {
				//FIXME: possible evictions when shrinking
            assert(assoc <= k_MAX_ASSOCIATIVITY);
            tags_last_index = assoc - 1;
            next_replace_index = tags_last_index;
         }

         UINT32 getAssociativity() { return tags_last_index + 1; }


         pair<bool, CacheTag*> find(CacheTag& tag, UINT32* set_index = NULL)
         {
				// useful for debuggin
//				print();

				assert(tags_last_index < k_MAX_ASSOCIATIVITY);

            for (INT32 index = tags_last_index; index >= 0; index--)
            {
               if(the_tags[index] == tag) 
					{
                  if ( set_index != NULL )
							*set_index = index;
                  return make_pair(true, &the_tags[index]);
					}
            }
            return make_pair(false, (CacheTag*) NULL);
			}
      
         void read_line(UINT32 index, UINT32 offset, char *out_buff, UINT32 bytes)
         {
				assert( offset + bytes <= blocksize );

				if ( (out_buff != NULL) && (bytes != 0) )
					memcpy(out_buff, &the_blocks[index * blocksize + offset], bytes);
			}

         void write_line(UINT32 index, UINT32 offset, char *buff, UINT32 bytes)
         {
				assert( offset + bytes <= blocksize );

				if ( (buff != NULL) && (bytes != 0) )
					memcpy(&the_blocks[index * blocksize + offset], buff, bytes);
			}

         bool invalidateTag(CacheTag& tag) 
         { 
				for (INT32 index = tags_last_index; index >= 0; index--)
            {
					assert(0 <= index && (UINT32)index < k_MAX_ASSOCIATIVITY);
				  
					if(the_tags[index] == tag)
					{
						the_tags[index] = CacheTag();
                  return true;
					}
            }
            return false;
         }

         VOID replace(CacheTag& tag, char* fill_buff, bool* eviction, ADDRINT* evict_addr, char* evict_buff)
         {
            const UINT32 index = next_replace_index;
            //cout << "*** replacing index " << index << " ***" << endl;

            if ( the_tags[index].isValid() )
				{
					if ( eviction != NULL )
						*eviction = true;
					if ( evict_addr != NULL )
						*evict_addr = the_tags[index];
					if ( evict_buff != NULL )
						memcpy(evict_buff, &the_blocks[index * blocksize], blocksize);
            }
            
				assert(index < k_MAX_ASSOCIATIVITY);
            the_tags[index] = tag;

            if ( fill_buff != NULL )
				{
					memcpy(&the_blocks[index * blocksize], fill_buff, blocksize);
            }

            // condition typically faster than modulo
            next_replace_index = (index == 0 ? tags_last_index : index - 1);
         }

         VOID modifyAssociativity(UINT32 assoc)
         {
				//FIXME: potentially need to evict

            assert(assoc != 0 && assoc <= k_MAX_ASSOCIATIVITY);
            UINT32 associativity = getAssociativity();

            if ( assoc > associativity ) 
				{
               for (UINT32 i = tags_last_index + 1; i < assoc; i++)
					{
                  assert(i < k_MAX_ASSOCIATIVITY);
						the_tags[i] = CacheTag();
					}
               tags_last_index = assoc - 1;
               next_replace_index = tags_last_index;
            } 
            else 
            {
               if ( assoc < associativity ) 
					{
						// FIXME: if cache model ever starts including data in addition to just tags
                  // need to perform evictions here. Also if we have shared mem?

						assert( !g_knob_simarch_has_shared_mem );

                  for (UINT32 i = tags_last_index; i >= assoc; i--)
						{
                     assert(i < k_MAX_ASSOCIATIVITY);
							the_tags[i] = CacheTag();
						}

                  tags_last_index = assoc - 1;
                  if ( next_replace_index > tags_last_index )
						{
                     next_replace_index = tags_last_index;
						}
               }
            }      
         }

         VOID print()
         {
            cout << "associativity = " << tags_last_index + 1 << "; next set to replace = " 
                 << next_replace_index << "     tags = ";
            for (UINT32 i = 0; i < getAssociativity(); i++)
            {
	       cout << hex << the_tags[i] << " ";
            }
            cout << endl;
         }

   };


}; 
// end namespace CACHE_SET


// Generic cache base class; no allocate specialization, no cache set specialization

class CacheBase
{
   public:
      // types, constants
      typedef enum 
      {
         k_ACCESS_TYPE_LOAD,
         k_ACCESS_TYPE_STORE,
         k_ACCESS_TYPE_NUM
      } AccessType;

      typedef enum
      {
         k_CACHE_TYPE_ICACHE,
         k_CACHE_TYPE_DCACHE,
         k_CACHE_TYPE_NUM
      } CacheType;

   protected:
		//1 counter for hit==true, 1 counter for hit==false
      CacheStats access[k_ACCESS_TYPE_NUM][2];

   protected:
      // input params
      const string name;
      UINT32 cache_size;
      const UINT32 line_size;
      UINT32 associativity;

      // computed params
      const UINT32 line_shift;
      const UINT32 set_index_mask;

   private:
      CacheStats sumAccess(bool hit) const
      {
         CacheStats sum = 0;

         for (UINT32 access_type = 0; access_type < k_ACCESS_TYPE_NUM; access_type++)
         {
            sum += access[access_type][hit];
         }

         return sum;
      }

   public:
      // constructors/destructors
      CacheBase(string name, UINT32 size, UINT32 line_bytes, UINT32 assoc);    

      // accessors
      UINT32 getCacheSize() const { return cache_size; }
      UINT32 getLineSize() const { return line_size; }
      UINT32 getNumWays() const { return associativity; }
      UINT32 getNumSets() const { return set_index_mask + 1; }
    
      // stats
      CacheStats getHits(AccessType access_type) const { 
			assert(access_type < k_ACCESS_TYPE_NUM);
			return access[access_type][true]; 
		}
      CacheStats getMisses(AccessType access_type) const { 
			assert(access_type < k_ACCESS_TYPE_NUM);
			return access[access_type][false]; 
		}
      CacheStats getAccesses(AccessType access_type) const 
         { return getHits(access_type) + getMisses(access_type); }
      CacheStats getHits() const { return sumAccess(true); }
      CacheStats getMisses() const { return sumAccess(false); }
      CacheStats getAccesses() const { return getHits() + getMisses(); }

      // utilities
      VOID splitAddress(const ADDRINT addr, CacheTag& tag, UINT32& set_index) const
      {
	 tag = CacheTag(addr >> line_shift);
         set_index = tag & set_index_mask;
      }

      // FIXME: change the name? 
      VOID splitAddress(const ADDRINT addr, CacheTag& tag, UINT32& set_index, 
                        UINT32& line_index) const
      {
         const UINT32 line_mask = line_size - 1;
         line_index = addr & line_mask;
         splitAddress(addr, tag, set_index);
      }

      string statsLong(string prefix = "", 
                       CacheType cache_type = k_CACHE_TYPE_DCACHE) const;
};



//  Templated cache class with specific cache set allocation policies
//  All that remains to be done here is allocate and deallocate the right
//  type of cache sets.

template <class SET_t, UINT32 k_MAX_SETS, UINT32 k_MAX_SEARCH, UINT32 k_STORE_ALLOCATION>
class Cache : public CacheBase
{
   private:
      SET_t  sets[k_MAX_SETS];
      UINT64 accesses[k_MAX_SETS];
      UINT64 misses[k_MAX_SETS];
      UINT64 total_accesses[k_MAX_SETS];
      UINT64 total_misses[k_MAX_SETS];
      UINT32 set_ptrs[k_MAX_SETS+1];
      UINT32 max_search;

   public:
      VOID resetCounters()
      {
         assert(getNumSets() <= k_MAX_SETS);
			for(UINT32 i = 0; i < getNumSets(); i++) {
            accesses[i] = misses[i] = 0;
         }
      }

      UINT32 getSearchDepth() const { return max_search; }
      UINT32 getSetPtr(UINT32 set_index) 
      { 
         assert( set_index < getNumSets() ); 
         assert(getNumSets() <= k_MAX_SETS);
			return set_ptrs[set_index];
      }
      void setSetPtr(UINT32 set_index, UINT32 value)
      {
         assert( set_index < k_MAX_SETS );
         assert( (value < getNumSets()) || (value == k_MAX_SETS) );
         set_ptrs[set_index] = value;
      }

      // constructors/destructors
      Cache(string name, UINT32 size, UINT32 line_bytes, 
            UINT32 assoc, UINT32 max_search_depth) : 
         CacheBase(name, size, line_bytes, assoc)
      {
         assert(getNumSets() <= k_MAX_SETS);
         assert(max_search_depth < k_MAX_SEARCH);

         max_search = max_search_depth;

         //initialization for cache hashing
         srand( time(NULL) );

         for (UINT32 i = 0; i < getNumSets(); i++)
         {
            total_accesses[i] = total_misses[i] = 0;               
            sets[i].setAssociativity(assoc);
            sets[i].setBlockSize(line_bytes);
            set_ptrs[i] = k_MAX_SETS;
         }
         resetCounters();
      }


      //JME: added for dynamically resizing a cache
      VOID resize(UINT32 assoc)
      {
         // new configuration written out overly explicitly; basically nothing
         // but the cache size changes
         //    _newNumSets = getNumSets();
         //    _newLineSize = line_size;
         //    _newLineShift = line_shift;
         //    _newSetIndexMask = set_index_mask;        
   
         assert(getNumSets() <= k_MAX_SETS);
         cache_size = getNumSets() * assoc * line_size;
         associativity = assoc;

         // since the number of sets stays the same, no lines need to be relocated
         // internally; instead space for blocks within each set needs to be added 
         // or removed (possibly causing evictions in the real world)

         for (UINT32 i = 0; i < getNumSets(); i++)
         {
            sets[i].modifyAssociativity(assoc);
         } 
      }


      // functions for accessing the cache

      // External interface for invalidating a cache line. Returns whether or not line was in cache
      bool invalidateLine(ADDRINT addr)
      {
         CacheTag tag;
         UINT32 index;

         splitAddress(addr, tag, index);
         assert(index < k_MAX_SETS);
         return sets[index].invalidateTag(tag);
      }

      // Single line cache access at addr 
      pair<bool, CacheTag*> accessSingleLine(ADDRINT addr, AccessType access_type, 
                                             bool* fail_need_fill = NULL, char* fill_buff = NULL,
                                             char* buff = NULL, UINT32 bytes = 0, 
                                             bool* eviction = NULL, ADDRINT* evict_addr = NULL, char* evict_buff = NULL)
      {

	 /*
            Usage:
               fail_need_fill gets set by this function. indicates whether or not a fill buff is required.
               If you get one, retry specifying a valid fill_buff containing the line from DRAM. 
               For read accesses, bytes and buff are for retrieving data from a cacheline. 
               The var called bytes specifies how many bytes to copy into buff.
               Writes work similarly, but bytes specifies how many bytes to write into the cacheline. 
               The var called eviction indicates whether or not an eviction occured. 
               The vars called evict_addr and evict_buff contain the evict address and the cacheline
	 */ 

         // set these to correspond with each other
         assert( (buff == NULL) == (bytes == 0) );
 
         // these should be opposite. they signify whether or not to service a fill
         assert( (fail_need_fill == NULL) || ((fail_need_fill == NULL) != (fill_buff == NULL)) );

         // You might have an eviction at any time. If you care about evictions, all three parameters should be non-NULL
         assert( ((eviction == NULL) == (evict_addr == NULL)) && ((eviction == NULL) == (evict_buff == NULL)) );

         UINT32 history[k_MAX_SEARCH];

         CacheTag tag;
         UINT32 set_index;

         splitAddress(addr, tag, set_index);

         UINT32 index = set_index;
         UINT32 next_index = index;
         UINT32 depth = 0;
         bool hit; 

         pair<bool, CacheTag*> res;
         CacheTag *tagptr = (CacheTag*) NULL;
         UINT32 line_index = -1;

         do 
         {
	    index = next_index;
            history[depth] = index;
            SET_t &set = sets[index];
            //set.print();
            res = set.find(tag, &line_index);
            hit = res.first;
            next_index = set_ptrs[index];
         } while( !hit && ((++depth) < max_search ) && (index < k_MAX_SETS));


         if ( fail_need_fill != NULL )
	 {
	    if ( (fill_buff == NULL) && !hit )
	    {
	       *fail_need_fill = true;

               if ( eviction != NULL )
		  *eviction = false;

               return make_pair(false, (CacheTag*) NULL);
	    } else {
	       *fail_need_fill = false;
	    }
	 }

         if ( hit )
	 {
	    tagptr = res.second;

            if ( access_type == k_ACCESS_TYPE_LOAD )
	       sets[index].read_line(line_index, addr & (line_size - 1), buff, bytes);
            else
	       sets[index].write_line(line_index, addr & (line_size - 1), buff, bytes);

         }

         // on miss, loads always allocate, stores optionally
         if ( (! hit) && ((access_type == k_ACCESS_TYPE_LOAD) || 
                          (k_STORE_ALLOCATION == CACHE_ALLOC::k_STORE_ALLOCATE)) )
         {
            UINT32 r_num = rand() % depth;
            UINT32 which = history[r_num];
            sets[which].replace(tag, fill_buff, eviction, evict_addr, evict_buff);
            tagptr = sets[which].find(tag, &line_index).second;

            if ( depth > 1 )
               cout << "which = " << dec << which << endl;

            if ( access_type == k_ACCESS_TYPE_LOAD )
	       sets[which].read_line(line_index, addr & (line_size - 1), buff, bytes);
            else
	       sets[which].write_line(line_index, addr & (line_size - 1), buff, bytes);

         }
         else 
	 {
	    if ( eviction != NULL )
	       *eviction = false;
	 }

         access[access_type][hit]++;

         return make_pair(hit, tagptr);
      }


      // Single line cache access at addr 
      pair<bool, CacheTag*> accessSingleLinePeek(ADDRINT addr)
      {
         CacheTag tag;
         UINT32 set_index;

         splitAddress(addr, tag, set_index);

         UINT32 index = set_index;
         UINT32 depth = 0;
         bool hit; 

         pair<bool, CacheTag*> res;
         CacheTag *tagptr = (CacheTag*) NULL;

         do 
         {
            SET_t &set = sets[index];
            //set.print();
            res = set.find(tag);
            hit = res.first;
            index = set_ptrs[index];
         } while( !hit && ((++depth) < max_search ) && (index < k_MAX_SETS));

         if ( hit )
	    tagptr = res.second;

         return make_pair(hit, tagptr);
      }


};

#endif 
