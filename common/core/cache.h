// Jonathan Eastep (eastep@mit.edu) 
// 04.07.08
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

#include "pin.H"
#include "utils.h"
#include "cache_state.h"

#define k_KILO 1024
#define k_MEGA (k_KILO*k_KILO)
#define k_GIGA (k_KILO*k_MEGA)

// type of cache hit/miss counters
typedef UINT64 CacheStats; 

extern LEVEL_BASE::KNOB<bool> g_knob_simarch_has_shared_mem;

// Cache tag - self clearing on creation

class CacheTag
{
   private:
      ADDRINT the_tag;
      CacheState::cstate_t the_cstate;
   
	public:
      CacheTag(ADDRINT tag = ~0, CacheState::cstate_t cstate = CacheState::INVALID) : 
         the_tag(tag), the_cstate(cstate) { }

      bool operator==(const CacheTag &right) const 
      { 
	 //return (the_tag == right.the_tag) && (the_cstate == right.the_cstate); 
	 return (the_tag == right.the_tag);
      }

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
            ASSERTX(assoc == 1); 
            the_tag = CacheTag();
         }

         VOID setAssociativity(UINT32 assoc) { ASSERTX(assoc == 1); }

         UINT32 getAssociativity(UINT32 assoc) { return 1; }

         UINT32 find(CacheTag& tag) { return(the_tag == tag); }

         VOID replace(CacheTag& tag) { the_tag = tag; }

         VOID modifyAssociativity(UINT32 assoc) { ASSERTX(assoc == 1); }

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

// CELIO TODO make a subclass inherent this, that has the necessary shared memory tags/hooks to work
// set_base -> set RoundRobin (cche) -> setRoundRobin with SMem 
// need to make certain privates protected
// set flag in ocache to see if shmem is on, then instantiate correct class
//
//
// Cache set with round robin replacement
   
	// Cache set with round robin replacement
   template <UINT32 k_MAX_ASSOCIATIVITY = 8>
   class RoundRobin
   {
      private:
         CacheTag the_tags[k_MAX_ASSOCIATIVITY];
         UINT32 tags_last_index;
         UINT32 next_replace_index;

      public:

         RoundRobin(UINT32 assoc = k_MAX_ASSOCIATIVITY): 
            tags_last_index(assoc - 1)
         {
            ASSERTX(assoc <= k_MAX_ASSOCIATIVITY);
            next_replace_index = tags_last_index;

            for (INT32 index = tags_last_index; index >= 0; index--)
            {
               the_tags[index] = CacheTag();
            }
         }

         VOID setAssociativity(UINT32 assoc)
         {
            ASSERTX(assoc <= k_MAX_ASSOCIATIVITY);
            tags_last_index = assoc - 1;
            next_replace_index = tags_last_index;
         }

         UINT32 getAssociativity() { return tags_last_index + 1; }

#if 0    
         UINT32 find(CacheTag& tag)
         {
            bool result = true;

            for (INT32 index = tags_last_index; index >= 0; index--)
            {
               // this is an ugly micro-optimization, but it does cause a
               // tighter assembly loop for ARM that way ...
               if(the_tags[index] == tag) goto end;
            }
            result = false;

         end: 
            return result;
         }
#endif

         pair<bool, CacheTag*> find(CacheTag& tag)
         {
            bool result = true;
            INT32 index;

            for (index = tags_last_index; index >= 0; index--)
            {
               // this is an ugly micro-optimization, but it does cause a
               // tighter assembly loop for ARM that way ...
               if(the_tags[index] == tag) goto end;
            }
            result = false;

         end: 
            return result ? make_pair(true, &the_tags[index]) : make_pair(false, (CacheTag*) NULL);
	 }

         bool invalidateTag(CacheTag& tag) 
         { 
            bool result = true;


//>>>>>>>> this is where jonahtan's code got cut off >>>>>>>>>
//==============
#if 0

/*
template <UINT32 k_MAX_ASSOCIATIVITY = 8>
class RoundRoundSharedMem: public RoundRobin<k_MAX_ASSOCIATIVITY>
{
	private:
	public:
		setCSTate();
		getCSTate();
}
  */

template <UINT32 k_MAX_ASSOCIATIVITY = 8>
class RoundRobin                                                        
{
	private:
		UINT32 tags_last_index;
		UINT32 next_replace_index;
	
	protected:
		CacheTag the_tags[k_MAX_ASSOCIATIVITY];

	public:

		RoundRobin(UINT32 assoc = k_MAX_ASSOCIATIVITY): 
			tags_last_index(assoc - 1)
		{
			ASSERTX(assoc <= k_MAX_ASSOCIATIVITY);
			next_replace_index = tags_last_index;

			for (INT32 index = tags_last_index; index >= 0; index--)
			{
				the_tags[index] = CacheTag();
			}
		}

		VOID setAssociativity(UINT32 assoc)
		{
			ASSERTX(assoc <= k_MAX_ASSOCIATIVITY);
			tags_last_index = assoc - 1;
			next_replace_index = tags_last_index;
		}

		UINT32 getAssociativity() { return tags_last_index + 1; }
 
		UINT32 find(CacheTag tag)
		{
			bool result = true;

			for (INT32 index = tags_last_index; index >= 0; index--)
			{
				// this is an ugly micro-optimization, but it does cause a
				// tighter assembly loop for ARM that way ...
				if(the_tags[index] == tag) goto end;
			}
			result = false;

		end: 
			return result;
		}

		bool invalidateTag(CacheTag tag) 
		{ 
			bool result = true;
#endif
//cut above this i believe
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<================


			INT32 index;
			for (index = tags_last_index; index >= 0; index--)
            {
               // this is an ugly micro-optimization, but it does cause a
               // tighter assembly loop for ARM that way ...
               if(the_tags[index] == tag) goto end;
            }
            result = false;

         end: 
            if ( result )
 	       the_tags[index] = CacheTag();

            return result;
         }

         VOID replace(CacheTag& tag)
         {
            // g++ -O3 too dumb to do CSE on following lines?!
            const UINT32 index = next_replace_index;

            the_tags[index] = tag;
            // condition typically faster than modulo
            next_replace_index = (index == 0 ? tags_last_index : index - 1);
         }

         VOID modifyAssociativity(UINT32 assoc)
         {
            ASSERTX(assoc != 0 && assoc <= k_MAX_ASSOCIATIVITY);
            UINT32 associativity = getAssociativity();

            if ( assoc > associativity ) {
               for (UINT32 i = tags_last_index + 1; i < assoc; i++)
	       {
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

		  ASSERTX( !g_knob_simarch_has_shared_mem );

                  for (UINT32 i = tags_last_index; i >= assoc; i--)
	          {
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
            cout << tags_last_index + 1 << " " << next_replace_index << "     ";
            for (UINT32 i = 0; i < getAssociativity(); i++)
            {
               cout << hex << the_tags[i] << " ";
            }
            cout << endl;
         }

   };


}; 
// end namespace CACHE_SET

namespace CACHE_ALLOC
{
   typedef enum 
   {
      k_STORE_ALLOCATE,
      k_STORE_NO_ALLOCATE
   } StoreAllocation;
};


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
      static const UINT32 k_HIT_MISS_NUM = 2;
      CacheStats access[k_ACCESS_TYPE_NUM][k_HIT_MISS_NUM];

   protected:
      // input params
      const std::string name;
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
      CacheBase(std::string name, UINT32 size, UINT32 line_bytes, UINT32 assoc);    

      // accessors
      UINT32 getCacheSize() const { return cache_size; }
      UINT32 getLineSize() const { return line_size; }
      UINT32 getNumWays() const { return associativity; }
      UINT32 getNumSets() const { return set_index_mask + 1; }
    
      // stats
      CacheStats getHits(AccessType access_type) const { return access[access_type][true]; }
      CacheStats getMisses(AccessType access_type) const { return access[access_type][false]; }
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
         for(UINT32 i = 0; i < getNumSets(); i++) {
            accesses[i] = misses[i] = 0;
         }
      }

      UINT32 getSearchDepth() const { return max_search; }
      UINT32 getSetPtr(UINT32 set_index) 
      { 
         ASSERTX( set_index < getNumSets() ); 
         return set_ptrs[set_index];
      }
      void setSetPtr(UINT32 set_index, UINT32 value)
      {
         ASSERTX( set_index < k_MAX_SETS );
         ASSERTX( (value < getNumSets()) || (value == k_MAX_SETS) );
         set_ptrs[set_index] = value;
      }

      // constructors/destructors
      Cache(std::string name, UINT32 size, UINT32 line_bytes, 
            UINT32 assoc, UINT32 max_search_depth) : 
         CacheBase(name, size, line_bytes, assoc)
      {
         ASSERTX(getNumSets() <= k_MAX_SETS);
         ASSERTX(max_search_depth < k_MAX_SEARCH);

         max_search = max_search_depth;

         //initialization for cache hashing
         srand( time(NULL) );

         for (UINT32 i = 0; i < getNumSets(); i++)
         {
            total_accesses[i] = total_misses[i] = 0;               
            sets[i].setAssociativity(assoc);
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
         return sets[index].invalidateTag(tag);
      }

#if 0
      // Multi-line cache access from addr to addr+size-1
      bool accessMultiLine(ADDRINT addr, UINT32 size, AccessType access_type)
      {

         const ADDRINT high_addr = addr + size;
         bool all_hit = true;

         const ADDRINT line_bytes = getLineSize();
         const ADDRINT not_line_mask = ~(line_bytes - 1);

         UINT32 history[k_MAX_SEARCH];

         do
         {
             CacheTag tag;
             UINT32 set_index;

             splitAddress(addr, tag, set_index);

             UINT32 index = set_index;
             UINT32 depth = 0;
             bool local_hit;
        
             do
	     {
                //if ( depth > 0)
	        //cout << "index = " << index << endl;
                history[depth] = index;
                SET_t &set = sets[index];
                local_hit = set.find(tag).first;
                index = set_ptrs[index];
             } while ( !local_hit && ((++depth) < max_search) && (index < k_MAX_SETS));

             all_hit &= local_hit;

             // on miss, loads always allocate, stores optionally
             if ( (! local_hit) && ((access_type == k_ACCESS_TYPE_LOAD) || 
                                   (k_STORE_ALLOCATION == CACHE_ALLOC::k_STORE_ALLOCATE)) )
             {
	        UINT32 r_num = rand() % depth;
                UINT32 which = history[r_num];
                sets[which].replace(tag);
                //if ( depth > 1 )
	        //cout << "which = " << which << endl;
             }

            // start of next cache line
            addr = (addr & not_line_mask) + line_bytes;
         }
         while (addr < high_addr);

         access[access_type][all_hit]++;

         return all_hit;
      }

      // Single line cache access at addr 
      bool accessSingleLine(ADDRINT addr, AccessType access_type)
      {
         UINT32 history[k_MAX_SEARCH];

         CacheTag tag;
         UINT32 set_index;

         splitAddress(addr, tag, set_index);

         UINT32 index = set_index;
         UINT32 depth = 0;
         bool hit; 

         do 
         {
            //cout << "index = " << index << endl;
            history[depth] = index;
            SET_t &set = sets[index];
            //set.print();
            hit = set.find(tag).first;
            index = set_ptrs[index];
         } while( !hit && ((++depth) < max_search ) && (index < k_MAX_SETS));

         // on miss, loads always allocate, stores optionally
         if ( (! hit) && ((access_type == k_ACCESS_TYPE_LOAD) || 
                          (k_STORE_ALLOCATION == CACHE_ALLOC::k_STORE_ALLOCATE)) )
         {
            UINT32 r_num = rand() % depth;
            UINT32 which = history[r_num];
            sets[which].replace(tag);
            if ( depth > 1 )
               cout << "which = " << dec << which << endl;
         }

         access[access_type][hit]++;

         return hit;
      }

#endif


      // Single line cache access at addr 
      pair<bool, CacheTag*> accessSingleLine(ADDRINT addr, AccessType access_type)
      {
         UINT32 history[k_MAX_SEARCH];

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
            //cout << "index = " << index << endl;
            history[depth] = index;
            SET_t &set = sets[index];
            //set.print();
            res = set.find(tag);
            hit = res.first;
            index = set_ptrs[index];
         } while( !hit && ((++depth) < max_search ) && (index < k_MAX_SETS));

         if ( hit )
	    tagptr = res.second;

         // on miss, loads always allocate, stores optionally
         if ( (! hit) && ((access_type == k_ACCESS_TYPE_LOAD) || 
                          (k_STORE_ALLOCATION == CACHE_ALLOC::k_STORE_ALLOCATE)) )
         {
            UINT32 r_num = rand() % depth;
            UINT32 which = history[r_num];
            sets[which].replace(tag);
            tagptr = sets[which].find(tag).second;
            if ( depth > 1 )
               cout << "which = " << dec << which << endl;
         }

         access[access_type][hit]++;

         return make_pair(hit, tagptr);
      }



};

#endif 
