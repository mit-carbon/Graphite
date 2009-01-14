#include "locked_hash.h"

using namespace std;

LockedHash::LockedHash(UInt64 size) 
:
   _size(size),
   _bins(new Bucket[size]),
   _locks(new PIN_LOCK[size])
{
   for(unsigned int i = 0; i < size; i++)
      InitLock (&_locks[i]);
}

LockedHash::~LockedHash()
{
    delete [] _bins;
    delete [] _locks;
}

pair<bool, UInt64> LockedHash::find(UInt64 key)
{
   UInt64 index = key % _size;
   pair<bool, UInt64> res;

   res.first = false;

   GetLock(&_locks[index], 1);

   map<UInt64,UInt64>::iterator iter = _bins[index].find(key);
   if( iter != _bins[index].end() ) 
   {
      res.first = true;
      res.second = iter->second;
   }

   ReleaseLock(&_locks[index]);
   return res; 
}

bool LockedHash::insert(UInt64 key, UInt64 value)
{
   UInt64 index = key % _size;
   GetLock(&_locks[index], 1);
   _bins[index].insert(make_pair(key, value));
   ReleaseLock(&_locks[index]);

   return true;
}


#ifdef DEBUG_LOCKED_HASH

int main(int argc, char* argv[])
{
   LockedHash hash(100);
   UInt64 ids[4] = {1001, 1050, 1011, 1099}; 

   for(int i = 0; i < 4; i++)
      hash.insert(ids[i], i);

   for(int i = 3; i >= 0; i--)
      assert( hash.find(ids[i]).first == true );
   cerr << "Test 1 passed" << endl;

   
   cerr << "Test 2 should fail in assertion" << endl;
   ids[3] = ids[0] + 100;
   
   for(int i = 0; i < 4; i++)
      hash.insert(ids[i], i);
   
   cerr << "All tests passed" << endl;

   return 0;
}


#endif
