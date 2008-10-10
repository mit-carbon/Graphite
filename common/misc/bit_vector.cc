#include "bit_vector.h"

/* ================================================================ */
/* Bit vector class method definitions */
/* ================================================================ */


void BitVector::reset()
{
   for( UINT32 i = 0; i < VECTOR_SIZE; i++)
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

   for (UINT32 i = 0; i < VECTOR_SIZE; i++)
      words[i] |= vec2.words[i];
}

void BitVector::clear(const BitVector& vec2)
{
   assert( size == vec2.size );

   for (UINT32 i = 0; i < VECTOR_SIZE; i++)
      words[i] &= ~vec2.words[i];
}

bool BitVector::test(const BitVector& vec2)
{
   assert( vec2.size == size );

   for (UINT32 i = 0; i < VECTOR_SIZE; i++) {
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
