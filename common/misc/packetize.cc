#include "packetize.h"

UnstructuredBuffer::UnstructuredBuffer()
{
}

template<class T> void UnstructuredBuffer::put(T* data, size_t num)
{
   the_chars.append((char *) data, num * sizeof(T));
}

template<class T> bool UnstructuredBuffer::get(T* data, size_t num)
{
   if ( the_chars.size() < (num * sizeof(T)) )
      return false;

   the_chars.copy((char *) data, num * sizeof(T));
   the_chars.erase(0, num * sizeof(T));

   return true;
}

template<class T> void UnstructuredBuffer::put(T data)
{ 
   put<T>(&data, 1); 
}

template<class T> bool UnstructuredBuffer::get(T& data)
{ 
   return get<T>(&data, 1); 
}


const char* UnstructuredBuffer::getBuffer()
{
   return the_chars.data();
}

void UnstructuredBuffer::clear()
{
   the_chars.erase();
}


#ifdef DEBUG_UNSTRUCTURED_BUFFER

int main(int argc, char* argv[])
{
   UnstructuredBuffer buff;
   UInt32 data1 = 1;
   UInt8 data2 = 'a';
   UInt32 data3 = 3;

   buff.put(data1);
   buff.put(data2);
   buff.put(data3);

   UInt32 data1prime;
   UInt8 data2prime;
   UInt32 data3prime;

   buff.get(data1prime);
   buff.get(data2prime);
   buff.get(data3prime);

   cout << data1prime << " " << data2prime << " " << data3prime << endl;

   assert( data1 == data1prime );
   assert( data2 == data2prime );
   assert( data3 == data3prime );

   char data4[] = "hello world";
   char data4prime[] = "aaaaaaaaaaaa";
   buff.put((UInt8*) data4, 12);
   buff.get((UInt8*) data4prime, 12);
  
   cout << data4prime << endl;
  
   assert( strcmp(data4, data4prime) == 0 );

   cout << "All tests passed" << endl;
 
   return 0;   
}

#endif
