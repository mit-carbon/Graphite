// Jonathan Eastep
// Build up / consume unstructured packets through a simple interface

#ifndef PACKETIZE_H
#define PACKETIZE_H

//#define DEBUG_UNSTRUCTURED_BUFFER


#include <assert.h>
#include <string>
#include <iostream>
#include "fixed_types.h"

using namespace std;

class UnstructuredBuffer {

   private:
      string the_chars;

      // These put / get scalars
      template<class T> void put(T& data);
      template<class T> bool get(T& data);

      // These put / get arrays
      // Note: these are shallow copy only
      template<class T> void put(T* data, size_t num);
      template<class T> bool get(T* data, size_t num);
      
   public:

      UnstructuredBuffer();
      const char* getBuffer();
      void clear();
      size_t size();


      // Wrappers

      // put scalar
      void put(UInt64& data);
      void put(SInt64& data);
      void put(UInt32& data);
      void put(SInt32& data);
      void put(UInt8&  data);
      void put(SInt8&  data);

      // get scalar
      bool get(UInt64& data);
      bool get(SInt64& data);
      bool get(UInt32& data);
      bool get(SInt32& data);
      bool get(UInt8&  data);
      bool get(SInt8&  data);      
      
      // put buffer
      void put(UInt8* data, size_t bytes);
      void put(SInt8* data, size_t bytes);      

      // get buffer
      bool get(UInt8* data, size_t bytes);
      bool get(SInt8* data, size_t bytes);
      
};

#endif
