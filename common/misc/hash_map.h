#pragma once

#include "fixed_types.h"
#include "lock.h"

class HashMap
{
private:
   class Entry
   {
   public:
      Entry(UInt32 key_, void* value_, Entry* prev_, Entry* next_)
         : key(key_), value(value_), prev(prev_), next(next_)
      {}
      ~Entry()
      {}
      
      UInt32 key;
      void* value;
      Entry* prev;
      Entry* next;
   };

public:
    HashMap();
    ~HashMap();

    // Get entry
    void* get(UInt32 key);
    template<class T>
       T* get(UInt32 key) { return (T*) get(key); }

    // Set entry
    void set(UInt32 key, void* value);
    template<class T>
       void set(UInt32 key, T* value) { return set(key, (void*) value); }
    
    // Insert entry
    void insert(UInt32 key, void* value);
    template<class T>
       void insert(UInt32 key, T* value) { return insert(key, (void*) value); }
    
    // Erase entry
    void erase(UInt32 key);

private:
   static const int HASH_SIZE = 10007; // prime
   Entry* _buckets[HASH_SIZE];
   Lock _locks[HASH_SIZE];
};
