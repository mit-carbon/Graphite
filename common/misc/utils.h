#ifndef UTILS_H
#define UTILS_H

#include "fixed_types.h"
#include <assert.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <typeinfo>
#include <cstdio>

using namespace std;


string myDecStr(UInt64 v, UInt32 w);


#define safeFDiv(x) (x ? (double) x : 1.0)

// Checks if n is a power of 2.
// returns true if n is power of 2

bool isPower2(UInt32 n);


// Computes floor(log2(n))
// Works by finding position of MSB set.
// returns -1 if n == 0.

SInt32 floorLog2(UInt32 n);


// Computes floor(log2(n))
// Works by finding position of MSB set.
// returns -1 if n == 0.

SInt32 ceilLog2(UInt32 n);

// Checks if (n) is a perfect square

bool isPerfectSquare(UInt32 n);

// Is Even and Is Odd ?

bool isEven(UInt32 n);
bool isOdd(UInt32 n);

// Get and set bits in an unsigned int variable (including low, not including high)
template <class T>
T getBits(T x, UInt8 high, UInt8 low)
{
   UInt8 bits = high-low;
   T bitmask = ( ((T)1) << bits) - 1;
   return (x >> low) & bitmask;
}

template <class T>
void setBits(T x, UInt8 high, UInt8 low, T val)
{
   UInt8 bits = high-low;
   T bitmask = (( ((T)1) << bits) - 1) << low;
   T valp = (val << low) & bitmask;
   x = (x & ~bitmask) | valp;
}

// Max and Min functions
template <class T>
T getMin(T v1, T v2)
{
   return (v1 < v2) ? v1 : v2;
}

template <class T>
T getMin(T v1, T v2, T v3)
{
   if ((v1 < v2) && (v1 < v3))
      return v1;
   else if (v2 < v3)
      return v2;
   else
      return v3;
}

template <class T>
T getMax(T v1, T v2)
{
   return (v1 > v2) ? v1 : v2;
}

template <class T>
T getMax(T v1, T v2, T v3)
{
   if ((v1 > v2) && (v1 > v3))
      return v1;
   else if (v2 > v3)
      return v2;
   else
      return v3;
}

// Use this only for basic data types
// char, int, float, double

template <class T>
T convertFromString(const string& s)
{
   T t;
   istringstream iss(s);
   if ((iss >> t).fail())
   {
      fprintf(stderr, "Conversion from (std::string) -> (%s) FAILED\n", typeid(t).name());
      exit(EXIT_FAILURE);
   }
   return t;
}

template <class T>
string convertToString(const T& t)
{
   ostringstream oss;
   oss << t;
   return oss.str();
}

template <class T>
string convertToString(const vector<T>& vec)
{
   ostringstream oss;
   for (typename vector<T>::const_iterator it = vec.begin(); it != vec.end(); it++)
   {
      T elem = (*it);
      oss << elem;
      if ((it + 1) != vec.end())
         oss << ", ";
   }
   return oss.str();
}

// Convert bits to bytes
UInt32 convertBitsToBytes(UInt32 num_bits);

// Trim the beginning and ending spaces in a string

string trimSpaces(const string& str);

// Parse an arbitrary list separated by arbitrary delimiters
// into a vector of strings

void parseList(const string& list, vector<string>& vec, string delim);

// Split a line into tokens separated by a list of delimiters

void splitIntoTokens(const string& line, vector<string>& tokens, const char* delimiters);

// Compute different statistics
double computeMean(const vector<UInt64>& vec);
double computeStddev(const vector<UInt64>& vec);
double computeCoefficientOfVariation(double mean, double stddev);

#endif
