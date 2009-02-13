// Jonathan Eastep
// Build up / consume unstructured packets through a simple interface

#ifndef PACKETIZE_H
#define PACKETIZE_H

//#define DEBUG_UNSTRUCTURED_BUFFER


#include <assert.h>
#include <string>
#include <iostream>
#include <utility>
#include "fixed_types.h"


class UnstructuredBuffer {

   private:
      std::string m_chars;
      
   public:

      UnstructuredBuffer();
      const char* getBuffer();
      void clear();
      int size();

      // These put / get scalars
      template<class T> void put(T& data);
      template<class T> bool get(T& data);

      // These put / get arrays
      // Note: these are shallow copy only
      template<class T> void put(T* data, int num);
      template<class T> bool get(T* data, int num);


      // Wrappers

      // put scalar
      UnstructuredBuffer& operator<<(UInt64 data);
      UnstructuredBuffer& operator<<(SInt64 data);
      UnstructuredBuffer& operator<<(UInt32 data);
      UnstructuredBuffer& operator<<(SInt32 data);
      UnstructuredBuffer& operator<<(UInt8  data);
      UnstructuredBuffer& operator<<(SInt8  data);

      UnstructuredBuffer& operator>>(UInt64& data);
      UnstructuredBuffer& operator>>(SInt64& data);
      UnstructuredBuffer& operator>>(UInt32& data);
      UnstructuredBuffer& operator>>(SInt32& data);
      UnstructuredBuffer& operator>>(UInt8&  data);
      UnstructuredBuffer& operator>>(SInt8&  data);      
      
      // put buffer
      UnstructuredBuffer& operator<<(std::pair<void*, int> buffer);

      // get buffer
      UnstructuredBuffer& operator>>(std::pair<void*, int> buffer);

};

#endif
