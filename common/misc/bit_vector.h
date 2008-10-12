// Jonathan Eastep (eastep@mit.edu)
// 10.10.08
// This file provides a bit vector class


#ifndef BIT_VECTOR_H
#define BIT_VECTOR_H


#include "pin.H"
#include <assert.h>
#include <vector>


#define BITVECT_DEBUG 0

class BitVector {
   private:
      UINT32 size;
      const UINT32 VECTOR_SIZE;
      vector<UINT64> words;

   public:

      #if BITVECT_DEBUG 
      void debug();
      #endif

      BitVector(UINT32 bits): size(bits), VECTOR_SIZE((bits+64-1)>>6), words(VECTOR_SIZE) { }

      UINT32 getSize() { return size; }

      void reset();

      bool at(UINT32 bit);

      void set(UINT32 bit);

      void clear(UINT32 bit);

      void set(const BitVector& vec2);

      void clear(const BitVector& vec2);

      bool test(const BitVector& vec2);

};


#endif
