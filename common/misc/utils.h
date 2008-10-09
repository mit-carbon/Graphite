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
#include <iostream>

using namespace std;

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


#define BITVECT_DEBUG 1

class BitVector {
   private:
      UINT32 size;
      const UINT32 VECTOR_SIZE;
      vector<UINT64> words;
      INT32 last_pos; //used in find function, for iterating through the set bits
							  //marks the position of the last set bit found.
							  //value of -1 means no last_pos bit 

   public:

      #if BITVECT_DEBUG 
      void debug();
      #endif

//<<<<<<< HEAD:common/misc/utils.h
//		BitVector(UINT32 bits): size(bits), words( (bits + 64 - 1) >> 6 ), last_pos(-1) {} 
		BitVector(UINT32 bits): size(bits), VECTOR_SIZE((bits+64-1)>>6), words( (bits + 64 - 1) >> 6 ), last_pos(-1) {} 

		//starting from position 'last_pos', find the next "1" bit. 
		//private variable 'last_pos' remembers the last position found,
		//allowing subsequent calls to "find" to effectively iterate
		//through the entire BitVector
		//returns -1 if no bits set (after last_pos)
		
		INT32 find();
	
		bool resetFind();
		
		//given an 8bit word, test to see if 'bit' is set
		//this is a helper function to the "find" function
		bool bTestBit(UINT8 word, UINT32 bit);

		UINT32 getSize() { return size; }
//=======
//      BitVector(UINT32 bits): size(bits), VECTOR_SIZE((bits+64-1)>>6), words(VECTOR_SIZE) { }

//>>>>>>> master:common/misc/utils.h

      void reset();

      bool at(UINT32 bit);

      void set(UINT32 bit);

      void clear(UINT32 bit);

      void set(const BitVector& vec2);

      void clear(const BitVector& vec2);

      bool test(const BitVector& vec2);

};


#endif
