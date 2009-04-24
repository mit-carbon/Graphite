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


class UnstructuredBuffer
{

   private:
      std::string m_chars;

   public:

      UnstructuredBuffer();
      const void* getBuffer();
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
      UnstructuredBuffer& operator<<(std::pair<const void*, int> buffer);

      // get buffer
      UnstructuredBuffer& operator>>(std::pair<const void*, int> buffer);

};

template<class T> void UnstructuredBuffer::put(T* data, int num)
{
   assert(num >= 0);
   m_chars.append((char *) data, num * sizeof(T));
}

template<class T> bool UnstructuredBuffer::get(T* data, int num)
{
   assert(num >= 0);
   if (m_chars.size() < (num * sizeof(T)))
      return false;

   m_chars.copy((char *) data, num * sizeof(T));
   m_chars.erase(0, num * sizeof(T));

   return true;
}

template<class T> void UnstructuredBuffer::put(T& data)
{
   put<T>(&data, 1);
}

template<class T> bool UnstructuredBuffer::get(T& data)
{
   return get<T>(&data, 1);
}

#endif
