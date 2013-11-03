#include <cstdlib>
#include <cstdio>
#include "hash_map.h"

HashMap::HashMap()
{
   for (int i = 0; i < HASH_SIZE; i++)
      _buckets[i] = (Entry*) NULL;
}

HashMap::~HashMap()
{}
   
void*
HashMap::get(UInt32 key)
{
   Entry* entry = _buckets[key % HASH_SIZE];
   if (entry && (entry->key == key))
      return entry->value;

   ScopedLock sl(_locks[key % HASH_SIZE]);
   
   entry = _buckets[key % HASH_SIZE];

   while (entry)
   {
      if (entry->key == key)
         return entry->value;
      entry = entry->next;
   }
   return NULL;
}

void
HashMap::set(UInt32 key, void *value)
{
   ScopedLock sl(_locks[key % HASH_SIZE]);

   Entry* entry = _buckets[key % HASH_SIZE];
   while (entry)
   {
      if (entry->key == key)
      {
         entry->value = value;
         return;
      }
      entry = entry->next;
   }
   fprintf(stderr, "*ERROR* [hash_map.cc] set(): Could not find key(%u)\n", key);
   exit(EXIT_FAILURE);
}

void
HashMap::insert(UInt32 key, void *value)
{
   ScopedLock sl(_locks[key % HASH_SIZE]);

   Entry* entry = _buckets[key % HASH_SIZE];
   bool initialized = false;

   if (!entry)
   {
      _buckets[key % HASH_SIZE] = new Entry(key, value, NULL, NULL);
      initialized = true;
   }
   while (!initialized)
   {
      if (entry->key == key)
      {
         fprintf(stderr, "*ERROR* [hash_map.cc] insert(): Key(%i) already initialized with value(%p)\n", key, entry->value);
         exit(EXIT_FAILURE);
      }
      if (!entry->next)
      {
         entry->next = new Entry(key, value, entry, NULL);
         initialized = true;
      }
      else
      {
         entry = entry->next;
      }
   }
}

void
HashMap::erase(UInt32 key)
{
   ScopedLock sl(_locks[key % HASH_SIZE]);

   Entry* entry = _buckets[key % HASH_SIZE];
   bool destroyed = false;

   if (!entry)
   {
      fprintf(stderr, "*ERROR* [hash_map.cc] erase(): Could not find key(%i) to erase\n", key);
      exit(EXIT_FAILURE);
   }

   if (entry->key == key)
   {
      _buckets[key % HASH_SIZE] = entry->next;
      if (entry->next)
         entry->next->prev = NULL;
      delete entry;
      destroyed = true;
   }

   while (entry && !destroyed)
   {
      if (entry->key == key)
      {
         // Found key - Destroy it
         if (entry->prev)
            entry->prev->next = entry->next;
         if (entry->next)
            entry->next->prev = entry->prev;
         delete entry;
         destroyed = true;
      }
      else
      {
         entry = entry->next;
      }
   }
   if (!destroyed)
   {
      fprintf(stderr, "*ERROR* [hash_map.cc] erase(): Could not find key(%i) to erase\n", key);
      exit(EXIT_FAILURE);
   }
}
