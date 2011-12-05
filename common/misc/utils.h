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

// Trim the beginning and ending spaces in a string

string trimSpaces(string& str);

// Parse an arbitrary list separated by arbitrary delimiters
// into a vector of strings

void parseList(string& list, vector<string>& vec, string delim);

#endif
