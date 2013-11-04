#include <unistd.h>
#include <sys/syscall.h>
#include <cstdio>
#include <cstdlib>
#include "tls.h"
#include "lock.h"

/*
  HashTLS implements TLS via a large hash map that we assume will
  never have collisions (in order for there to be collisions, two
  thread ids within the same process would have to differ by
  HASH_SIZE).

  If PinTLS is ever fixed, then HashTLS should probably be replaced by
  PinTLS. PthreadTLS is certainly safer when Pin is not being used.
*/

class HashTLS : public TLS
{
private:
   class Entry
   {
   public:
      Entry(int key_, void* value_, Entry* prev_, Entry* next_)
         : key(key_), value(value_), prev(prev_), next(next_)
      {}
      ~Entry()
      {}

      int key;
      void* value;
      Entry* prev;
      Entry* next;
   };
   
public:
   HashTLS()
   {
      for (int i = 0; i < HASH_SIZE; i++)
         _buckets[i] = (Entry*) NULL;
   }

   ~HashTLS()
   {
   }

   void* get()
   {
      int tid = syscall(SYS_gettid);
      
      Entry* entry = _buckets[tid % HASH_SIZE];
      if (entry && (entry->key == tid))
         return entry->value;

      ScopedLock sl(_locks[tid % HASH_SIZE]);
      
      entry = _buckets[tid % HASH_SIZE];

      while (entry)
      {
         if (entry->key == tid)
            return entry->value;
         entry = entry->next;
      }
      return NULL;
   }

   const void* get() const
   {
      return ((HashTLS*)this)->get();
   }

   void set(void *vp)
   {
      int tid = syscall(SYS_gettid);

      ScopedLock sl(_locks[tid % HASH_SIZE]);

      Entry* entry = _buckets[tid % HASH_SIZE];

      while (entry)
      {
         if (entry->key == tid)
         {
            entry->value = vp;
            return;
         }
         entry = entry->next;
      }
      fprintf(stderr, "*ERROR* [hash_tls.cc] set(): Could not find tid(%i)\n", tid);
      exit(EXIT_FAILURE);
   }

   void insert(void *vp)
   {
      int tid = syscall(SYS_gettid);
      
      ScopedLock sl(_locks[tid % HASH_SIZE]);

      Entry* entry = _buckets[tid % HASH_SIZE];
      bool initialized = false;

      if (!entry)
      {
         _buckets[tid % HASH_SIZE] = new Entry(tid, vp, NULL, NULL);
         initialized = true;
      }
      while (!initialized)
      {
         if (entry->key == tid)
         {
            fprintf(stderr, "*ERROR* [hash_tls.cc] insert(): Tid(%i) already initialized with value(%p)\n", tid, entry->value);
            exit(EXIT_FAILURE);
         }
         if (!entry->next)
         {
            entry->next = new Entry(tid, vp, entry, NULL);
            initialized = true;
         }
         else
         {
            entry = entry->next;
         }
      }
   }

   void erase()
   {
      int tid = syscall(SYS_gettid);
      
      ScopedLock sl(_locks[tid % HASH_SIZE]);

      Entry* entry = _buckets[tid % HASH_SIZE];
      bool destroyed = false;

      if (!entry)
      {
         fprintf(stderr, "*ERROR* [hash_tls.cc] erase(): Could not find tid(%i) to erase\n", tid);
         exit(EXIT_FAILURE);
      }

      if (entry->key == tid)
      {
         _buckets[tid % HASH_SIZE] = entry->next;
         if (entry->next)
            entry->next->prev = NULL;
         delete entry;
         destroyed = true;
      }

      while (entry && !destroyed)
      {
         if (entry->key == tid)
         {
            // Found tid - Destroy it
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
         fprintf(stderr, "*ERROR* [hash_tls.cc] erase(): Could not find tid(%i) to erase\n", tid);
         exit(EXIT_FAILURE);
      }
   }

private:
   static const int HASH_SIZE = 10007; // prime
   Entry* _buckets[HASH_SIZE];
   Lock _locks[HASH_SIZE];
};

// override PthreadTLS
TLS* TLS::create()
{
   return new HashTLS();
}
