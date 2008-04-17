#include "utils.h"


/* ================================================================ */
/* Utility function definitions */
/* ================================================================ */

string myDecStr(UINT64 v, UINT32 w)
{
    ostringstream o;
    o.width(w);
    o << v;
    string str(o.str());
    return str;
}


bool isPower2(UINT32 n)
{ return ((n & (n - 1)) == 0); }


INT32 floorLog2(UINT32 n)
{
   INT32 p = 0;

   if (n == 0) return -1;

   if (n & 0xffff0000) { p += 16; n >>= 16; }
   if (n & 0x0000ff00) { p +=  8; n >>=  8; }
   if (n & 0x000000f0) { p +=  4; n >>=  4; }
   if (n & 0x0000000c) { p +=  2; n >>=  2; }
   if (n & 0x00000002) { p +=  1; }

   return p;
}


INT32 ceilLog2(UINT32 n)
{ return floorLog2(n - 1) + 1; }



/* ================================================================ */
/* Bit vector class method definitions */
/* ================================================================ */


void BitVector::reset()
{
   for( UINT32 i = 0; i < ((size + 64 - 1) >> 6); i++)
      words[i] = 0;
}

bool BitVector::at(UINT32 bit) 
{
   assert( bit < size);

   UINT32 index = bit >> 6;
   UINT64 shift = bit & 63;
   UINT64 one   = 1;
   UINT64 mask  = one << shift;
   return (words[index] & mask) ? true : false;
}

void BitVector::set(UINT32 bit)
{
   assert( bit < size );

   UINT32 index  = bit >> 6;
   UINT64 shift  = bit & 63;
   UINT64 one    = 1;
   UINT64 mask   = one << shift;     
   words[index] |= mask;
}

void BitVector::clear(UINT32 bit)
{
   assert( bit < size );

   UINT32 index  = bit >> 6;
   UINT64 shift  = bit & 63;
   UINT64 one    = 1;
   UINT64 mask   = ~(one << shift);     
   words[index] &= mask;
}

void BitVector::set(const BitVector& vec2)
{
   assert( size == vec2.size );

   for (UINT32 i = 0; i < ((size + 64 - 1) >> 6); i++)
      words[i] |= vec2.words[i];
}

void BitVector::clear(const BitVector& vec2)
{
   assert( size == vec2.size );

   for (UINT32 i = 0; i < ((size + 64 - 1) >> 6); i++)
      words[i] &= ~vec2.words[i];
}

bool BitVector::test(const BitVector& vec2)
{
   assert( vec2.size == size );

   for (UINT32 i = 0; i < ((size + 64 - 1) >> 6); i++) {
      if ( vec2.words[i] & words[i] )
         return true;
   }
   
   return false;
}


#if BITVECT_DEBUG

void BitVector::debug()
{
   assert( size > 68 );

   set(66);
   cout << "set(66) -> " << at(66) << endl;
    
   clear(66);
   cout << "clear(66) -> " << at(66) << endl;

   set(66);
   set(68);
    
   BitVector vec2(size);
   vec2.set(44);

   cout << "test( (1<<66 | 1<<68), (1<<44) ) -> " << test(vec2) << endl;

   vec2.set(66);
   cout << "test( (1<<66 | 1<<68), (1<<66) | (1<<44) ) -> " 
        << test(vec2) << endl;    

   clear(vec2);
   cout << "test( (1<<68), (1<<66) | (1<<44) ) -> " << test(vec2) 
        << endl;

   cout << "at(66) = " << at(66) << "; at(68) = " << at(68) << endl; 

   cout << endl;

   //reset();
}

#endif
