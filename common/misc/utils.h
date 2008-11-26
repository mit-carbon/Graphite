// Jonathan Eastep (eastep@mit.edu)
// 04.07.08
// This file provides some useful bit-manipulating functions


#ifndef UTILS_H
#define UTILS_H

#include "pin.H"
#include <assert.h>
#include <sstream>
#include <iostream>

using namespace std;


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

#endif
