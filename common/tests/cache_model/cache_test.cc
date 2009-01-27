
#include <iostream>
#include <cassert>
#include <cstring>
#include "cache.h"

using namespace std;

// should match definition in ocache to be a meaningful test
typedef Cache<CACHE_SET::RoundRobin<16, 128>, 1024, 64, CACHE_ALLOC::k_STORE_ALLOCATE> 
        RRSACache;

// 
#define DC_SIZE (32768)
#define DC_BLOCKSIZE (32)
#define DC_ASSOC (8)


int main(int argc, char* argv[])
{
   // get valid aligned memory address to point to
   IntPtr dummy = (IntPtr) malloc((DC_SIZE + DC_BLOCKSIZE) * 2);
   dummy = (dummy / DC_BLOCKSIZE) * DC_BLOCKSIZE + DC_BLOCKSIZE;

   RRSACache* dc = new RRSACache("dl1", DC_SIZE, DC_BLOCKSIZE, DC_ASSOC, 1);

   bool fail_need_fill;
   char fill_buff[DC_BLOCKSIZE];

   char buff[DC_BLOCKSIZE];

   bool eviction;
   IntPtr evict_addr;
   char evict_buff[DC_BLOCKSIZE];

   IntPtr addr;
   CacheBase::AccessType access_type;

   char scratch[DC_BLOCKSIZE];
   pair<bool, CacheTag*> res;

   /* 
      test 1: load (cold miss) 
   */
   cout << "**** running test 1 ****" << endl;
   memset( buff, 0, DC_BLOCKSIZE );
   memset( fill_buff, 'a', DC_BLOCKSIZE );
   addr = dummy;
   access_type = CacheBase::k_ACCESS_TYPE_LOAD;
   res = dc->accessSingleLine(addr, access_type, 
			      &fail_need_fill, NULL,
			      buff, DC_BLOCKSIZE,
			      &eviction, &evict_addr, evict_buff);
   assert( res.first == false );
   assert( res.second == NULL );
   assert( fail_need_fill == true );
   assert( eviction == false );
   res = dc->accessSingleLine(addr, access_type,
			      NULL, fill_buff,
			      buff, DC_BLOCKSIZE,
			      &eviction, &evict_addr, evict_buff);
   assert( res.first == false );
   assert( res.second != NULL ); //why is this != NULL?
   assert( eviction == false );
   //buff should be equal to the fill
   for(UINT32 i = 0; i < DC_BLOCKSIZE; i++)
   {
      char tmp = 'a';
      if ( fill_buff[i] != tmp )
			cout << "got unexpected value at fill_buff[" << i << "]: " << fill_buff[i] << endl;

      if ( buff[i] != tmp )
			cout << "got unexpected value at buff[" << i << "]: " << buff[i] << endl;
      assert(buff[i] == tmp);
   }
   cout << "**** test 1 passed  ****" << endl << endl;


   /* 
      test 2: load (hit) 
      NOTE: builds on test 1
   */   
   cout << "**** running test 2 ****" << endl;
   memcpy(scratch, buff, DC_BLOCKSIZE);
   memset(buff, 0, DC_BLOCKSIZE);
   addr = dummy;
   access_type = CacheBase::k_ACCESS_TYPE_LOAD;
   res = dc->accessSingleLine(addr, access_type, 
			      &fail_need_fill, NULL,
			      buff, DC_BLOCKSIZE,
			      &eviction, &evict_addr, evict_buff);
   assert( res.first == true );
   assert( res.second != NULL );
   assert( fail_need_fill == false );
   assert( eviction == false );
   //buff should be equal to scratch (buff from last 
   for(UINT32 i = 0; i < DC_BLOCKSIZE; i++)
   {
      char tmp = 'a';
      if ( scratch[i] != tmp )
			cout << "got unexpected value at scratch[" << i << "]: " << scratch[i] << endl;

      if ( buff[i] != tmp )
			cout << "got unexpected value at buff[" << i << "]: " << buff[i] << endl;
      assert(buff[i] == tmp);
   }
   cout << "**** test 2 passed  ****" << endl << endl;


   /* 
      test 3: store (hit) 
      NOTE: relies on load (hit) working, builds on tests 1-2
   */   
   cout << "**** running test 3 ****" << endl;
   memset(buff, 0xCC, DC_BLOCKSIZE);
   memset(scratch, 0x00, DC_BLOCKSIZE);
   addr = dummy;
   access_type = CacheBase::k_ACCESS_TYPE_STORE;
   res = dc->accessSingleLine(addr, access_type, 
			      &fail_need_fill, NULL,
			      buff, DC_BLOCKSIZE,
			      &eviction, &evict_addr, evict_buff);
   assert( res.first == true );
   assert( res.second != NULL );
   assert( fail_need_fill == false );
   assert( eviction == false );
   access_type = CacheBase::k_ACCESS_TYPE_LOAD;
   res = dc->accessSingleLine(addr, access_type, 
			      &fail_need_fill, NULL,
			      scratch, DC_BLOCKSIZE,
			      &eviction, &evict_addr, evict_buff);
   assert( res.first == true );
   assert( res.second != NULL );
   assert( fail_need_fill == false );
   assert( eviction == false );
   //buff should be equal to scratch
   for(UINT32 i = 0; i < DC_BLOCKSIZE; i++)
   {
      if ( buff[i] != scratch[i] )
	cout << "got unexpected value at scratch[" << i << "]: " << scratch[i] << endl;
      assert(buff[i] == scratch[i]);
   }
   cout << "**** test 3 passed  ****" << endl << endl;


   /* 
      test 4: store (cold miss) 
      NOTE: relies on load (hit) working, builds on tests 1-3
   */   
   cout << "**** running test 4 ****" << endl;
   memset(buff, 0xEE, DC_BLOCKSIZE);
   memset(fill_buff, 0xDD, DC_BLOCKSIZE);
   memset(scratch, 0x00, DC_BLOCKSIZE);
   addr = dummy + DC_SIZE;
   access_type = CacheBase::k_ACCESS_TYPE_STORE;
   res = dc->accessSingleLine(addr, access_type, 
			      &fail_need_fill, NULL,
			      buff, DC_BLOCKSIZE,
			      &eviction, &evict_addr, evict_buff);
   assert( res.first == false );
   assert( res.second == NULL );
   assert( fail_need_fill == true );
   assert( eviction == false );
   res = dc->accessSingleLine(addr, access_type,
			      NULL, fill_buff,
			      buff, DC_BLOCKSIZE,
			      &eviction, &evict_addr, evict_buff);
   assert( res.first == false );
   assert( res.second != NULL );
   assert( eviction == false );
   access_type = CacheBase::k_ACCESS_TYPE_LOAD;
   res = dc->accessSingleLine(addr, access_type, 
			      &fail_need_fill, NULL,
			      scratch, DC_BLOCKSIZE,
			      &eviction, &evict_addr, evict_buff);
   assert( res.first == true );
   assert( res.second != NULL );
   assert( fail_need_fill == false );
   assert( eviction == false );
   //buff should be equal to scratch
   for(UINT32 i = 0; i < DC_BLOCKSIZE; i++)
   {
      if ( buff[i] != scratch[i] )
	cout << "got unexpected value at scratch[" << i << "]: " << scratch[i] << endl;
      assert(buff[i] == scratch[i]);
   }
   cout << "**** test 4 passed  ****" << endl << endl;

   
   /* 
      test 5: eviction 
      NOTE: relies on load (hit) working, builds on tests 1-4
   */   
   cout << "**** running test 5 ****" << endl;
   for (UINT32 i = 3; i <= DC_ASSOC; i++)
   {
      addr += DC_SIZE;
      access_type = CacheBase::k_ACCESS_TYPE_STORE;
      res = dc->accessSingleLine(addr, access_type, 
				 &fail_need_fill, NULL,
				 buff, DC_BLOCKSIZE,
				 &eviction, &evict_addr, evict_buff);
      assert( res.first == false );
      assert( res.second == NULL );
      assert( fail_need_fill == true );
      assert( eviction == false );
      res = dc->accessSingleLine(addr, access_type,
				 NULL, fill_buff,
				 buff, DC_BLOCKSIZE,
				 &eviction, &evict_addr, evict_buff);
      assert( res.first == false );
      assert( res.second != NULL );
      assert( eviction == false );
      access_type = CacheBase::k_ACCESS_TYPE_LOAD;
      res = dc->accessSingleLine(addr, access_type, 
				 &fail_need_fill, NULL,
				 scratch, DC_BLOCKSIZE,
				 &eviction, &evict_addr, evict_buff);
      assert( res.first == true );
      assert( res.second != NULL );
      assert( fail_need_fill == false );
      assert( eviction == false );
      //buff should be equal to scratch
      for(UINT32 i = 0; i < DC_BLOCKSIZE; i++)
      {
	 if ( buff[i] != scratch[i] )
	    cout << "got unexpected value at scratch[" << i << "]: " << scratch[i] << endl;
	 assert(buff[i] == scratch[i]);
      }
   }


   memset(buff, 0x20, DC_BLOCKSIZE);
   addr += DC_SIZE;
   access_type = CacheBase::k_ACCESS_TYPE_STORE;
   res = dc->accessSingleLine(addr, access_type, 
			      &fail_need_fill, NULL,
			      buff, DC_BLOCKSIZE,
			      &eviction, &evict_addr, evict_buff);
   assert( res.first == false );
   assert( res.second == NULL );
   assert( fail_need_fill == true );
   assert( eviction == false );
   res = dc->accessSingleLine(addr, access_type,
			      NULL, fill_buff,
			      buff, DC_BLOCKSIZE,
			      &eviction, &evict_addr, evict_buff);
   assert( res.first == false );
   assert( res.second != NULL );
   assert( eviction == true );
   assert( evict_addr == (dummy / DC_BLOCKSIZE) );
   //evicted buffer should be equal to what the last written to it was (test 3)
   for(UINT32 i = 0; i < DC_BLOCKSIZE; i++)
   {
      char tmp = (char) 0xCC;
      if ( tmp != evict_buff[i] )
         cout << "got unexpected value at evict_buff[" << i << "]: " << evict_buff[i] << endl;
      assert( tmp == evict_buff[i]);
   }   
   access_type = CacheBase::k_ACCESS_TYPE_LOAD;
   res = dc->accessSingleLine(addr, access_type, 
			      &fail_need_fill, NULL,
			      scratch, DC_BLOCKSIZE,
			      &eviction, &evict_addr, evict_buff);
   assert( res.first == true );
   assert( res.second != NULL );
   assert( fail_need_fill == false );
   assert( eviction == false );
   //buff should be equal to scratch
   for(UINT32 i = 0; i < DC_BLOCKSIZE; i++)
   {
      if ( buff[i] != scratch[i] )
	 cout << "got unexpected value at scratch[" << i << "]: " << scratch[i] << endl;
      assert(buff[i] == scratch[i]);
   }
   cout << "**** test 5 passed  ****" << endl << endl;

   

   /* 
      test 6: simple load (cold miss) 
   */
   cout << "**** running test 6 ****" << endl;
   addr = dummy + DC_SIZE / 2;
   access_type = CacheBase::k_ACCESS_TYPE_LOAD;
   res = dc->accessSingleLine(addr, access_type);
   assert( res.first == false );
   assert( res.second != NULL );
   cout << "**** test 6 passed  ****" << endl << endl;

   /* 
      test 7: simple load (hit) depends on 6
   */
   cout << "**** running test 7 ****" << endl;
   res = dc->accessSingleLine(addr, access_type);   
   assert( res.first == true );
   assert( res.second != NULL );
   cout << "**** test 7 passed  ****" << endl << endl;

   /* 
      test 8: simple store (hit) depends on 6-7
   */
   cout << "**** running test 8 ****" << endl;
   access_type = CacheBase::k_ACCESS_TYPE_STORE;
   res = dc->accessSingleLine(addr, access_type);
   assert( res.first == true );
   assert( res.second != NULL );
   cout << "**** test 8 passed  ****" << endl << endl;

   /* 
      test 9: simple store (cold miss)
   */
   cout << "**** running test 9 ****" << endl;
   addr = dummy + DC_SIZE / 2 + DC_BLOCKSIZE;
   access_type = CacheBase::k_ACCESS_TYPE_STORE;
   res = dc->accessSingleLine(addr, access_type);
   assert( res.first == false );
   assert( res.second != NULL );
   cout << "**** test 9 passed  ****" << endl << endl;

   /* 
      test 10: simple eviction depends on 9
   */
   cout << "**** running test 10 ****" << endl;
   for(UINT32 i=2; i<=DC_ASSOC; i++)
   {
      addr += DC_SIZE;
      access_type = CacheBase::k_ACCESS_TYPE_STORE;
      res = dc->accessSingleLine(addr, access_type);
      assert( res.first == false );
      assert( res.second != NULL );
   }
   addr += DC_SIZE;
   access_type = CacheBase::k_ACCESS_TYPE_STORE;
   res = dc->accessSingleLine(addr, access_type);   
   assert( res.first == false );
   assert( res.second != NULL );   
   for(UINT32 i = 0; i < DC_ASSOC; i++)
   {
      access_type = CacheBase::k_ACCESS_TYPE_LOAD;
      res = dc->accessSingleLine(addr, access_type);
      assert( res.first == true );
      assert( res.second != NULL );      
      addr -= DC_SIZE;
   }
   assert( addr == (dummy + DC_SIZE /2 + DC_BLOCKSIZE) );
   access_type = CacheBase::k_ACCESS_TYPE_LOAD;
   res = dc->accessSingleLine(addr, access_type);
   assert( res.first == false );
   assert( res.second != NULL );    
   cout << "**** test 10 passed  ****" << endl << endl;   


   return 0;
}


