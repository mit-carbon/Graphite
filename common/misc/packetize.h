// Jonathan Eastep
// Build up / consume unstructured packets through a simple interface

#ifndef PACKETIZE_H
#define PACKETZIE_H

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
      template<class T> void put(T data);
      template<class T> bool get(T& data);

      // These put / get arrays
      // Note: these are shallow copy only
      template<class T> void put(T* data, size_t num);
      template<class T> bool get(T* data, size_t num);
      
   public:

      UnstructuredBuffer();
      const char* getBuffer();
      void clear();

      void put(UInt64 data) { put<UInt64>(data); }
      void put(SInt64 data) { put<SInt64>(data); }
      void put(UInt32 data) { put<UInt32>(data); }
      void put(SInt32 data) { put<SInt32>(data); }
      void put(UInt8  data) { put<UInt8>(data); }
      void put(SInt8  data) { put<SInt8>(data); }

      bool get(UInt64& data) { return get<UInt64>(data); }
      bool get(SInt64& data) { return get<SInt64>(data); }
      bool get(UInt32& data) { return get<UInt32>(data); }
      bool get(SInt32& data) { return get<SInt32>(data); }
      bool get(UInt8&  data) { return get<UInt8>(data); }
      bool get(SInt8&  data) { return get<SInt8>(data); }      
      
      void put(UInt8* data, size_t bytes) { put<UInt8>(data, bytes); }
      bool get(UInt8* data, size_t bytes) { return get<UInt8>(data, bytes); }
      
};

#endif
