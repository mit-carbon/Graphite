// Jonathan Eastep (eastep@mit.edu)
// 04.07.08
// This file provides some useful bit-manipulating classes and 
// functions


#ifndef UTILS_H
#define UTILS_H

#include "pin.H"
#include <vector>
#include <assert.h>
#include <sstream>


/* ================================================================ */
/* Utility function declarations */
/* ================================================================ */ 


string myDecStr(UINT64 v, UINT32 w);


#define safeFDiv(x) (x ? (double) x : 1.0)


// Checks if n is a power of 2.
// returns true if n is power of 2

bool isPower2(UINT32 n);


// Computes floor(log2(n))
// Works by finding position of MSB set.
// returns -1 if n == 0.

INT32 floorLog2(UINT32 n);


// Computes floor(log2(n))
// Works by finding position of MSB set.
// returns -1 if n == 0.

INT32 ceilLog2(UINT32 n);


/* ================================================================ */
/* BITVECT class */
/* ================================================================ */ 


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
