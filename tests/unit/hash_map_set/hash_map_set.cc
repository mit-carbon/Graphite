#include <stdio.h>
#include <cassert>

#include "hash_map_set.h"
#include "fixed_types.h"

template <class T>
UInt32 moduloHashFn(T key, UInt32 hash_fn_param, UInt32 num_buckets)
{
   return (key >> hash_fn_param) % num_buckets;
}

int main(int argc, char *argv[])
{
   const UInt32 num_buckets = 1024;
   const UInt32 hash_fn_param = 6;
   HashMapSet<IntPtr> hash_map_set(num_buckets, &moduloHashFn<IntPtr>, hash_fn_param);

   // Insert many elements
   for (UInt32 i = 0; i < num_buckets*2; i++)
   {
      hash_map_set.insert(i << 6);
   }

   // Check if all the elements are there
   for (UInt32 i = 0; i < num_buckets*2; i++)
   {
      assert(hash_map_set.count(i << 6) == 1);
   }

   // Check if elements that does not exist are not present
   for (UInt32 i = num_buckets*2; i < num_buckets*4; i++)
   {
      assert(hash_map_set.count(i << 6) == 0);
   }

   // Erase all the elements
   for (UInt32 i = 0; i < num_buckets*2; i++)
   {
      hash_map_set.erase(i << 6);
   }

   // Check if they have been erased
   for (UInt32 i = 0; i < num_buckets*2; i++)
   {
      assert(hash_map_set.count(i << 6) == 0);
   }

   printf ("Hash Map Set tests successful\n");

   return 0;
}
