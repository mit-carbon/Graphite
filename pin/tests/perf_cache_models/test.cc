/*
   Jonathan Eastep 10.17.07
   Quick and dirty tests for resizing sets
	FIXME: these probably don't work anymore!
	DEPRECATED
*/

#include "pin.H"

#include <iostream>
#include <fstream>

#include "cache.h"
#include "ocache.h"
#include "pin_profile.H"
#include "knobs.h"

KNOB<UINT32> TestKnobDCacheThresholdHit(KNOB_MODE_WRITEONCE , "pintool",
    "drh", "100", "only report dcache memops with hit count above threshold");
KNOB<UINT32> TestKnobDCacheThresholdMiss(KNOB_MODE_WRITEONCE, "pintool",
    "drm","100", "only report dcache memops with miss count above threshold");
KNOB<UINT32> TestKnobICacheThresholdHit(KNOB_MODE_WRITEONCE , "pintool",
    "irh", "100", "only report icache ops with hit count above threshold");
KNOB<UINT32> TestKnobICacheThresholdMiss(KNOB_MODE_WRITEONCE, "pintool",
    "irm","100", "only report icache ops with miss count above threshold");

KNOB<UINT32> TestKnobCacheSize(KNOB_MODE_WRITEONCE,     "pintool",
    "c","64", "cache size in kilobytes");
KNOB<UINT32> TestKnobLineSize(KNOB_MODE_WRITEONCE,      "pintool",
    "b","32", "cache block size in bytes");
KNOB<UINT32> TestKnobAssociativity(KNOB_MODE_WRITEONCE, "pintool",
    "a","8", "cache associativity (1 for direct mapped)");
KNOB<UINT32> TestKnobMutationInterval(KNOB_MODE_WRITEONCE, "pintool",
    "m","4", "cache auto-reconfiguration period in number of accesses");

KNOB<UINT32> TestKnobDCacheMaxSearchDepth(KNOB_MODE_WRITEONCE, "pintool",
    "dd","1", "data cache max hash search depth (1 for no multi-cycle hashing)");
KNOB<UINT32> TestKnobICacheMaxSearchDepth(KNOB_MODE_WRITEONCE, "pintool",
    "id","1", "instruction cache max hash search depth (1 for no multi-cycle hashing)");

typedef CACHE_SET::RoundRobin<8> OSET;


INT32 Usage()
{
    cerr <<
        "This tool represents a cache simulator.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

void test_set_resizing()
{

  cout << "*** testing set resizing ***" << endl;

  OSET *set = new OSET(4);
  set->replace(CacheTag(0xCAFECA00));
  set->replace(CacheTag(0xCAFECA20));
  set->replace(CacheTag(0xCAFECA40));
  set->replace(CacheTag(0xCAFECA60));
  set->print();
  // 4 3     cafeca60 cafeca40 cafeca20 cafeca00 0 0 0 0 

  set->replace(CacheTag(0xCAFECA80));
  set->print();
  // 4 2     cafeca60 cafeca40 cafeca20 cafeca80 0 0 0 0

  set->modifyAssociativity(5);
  set->print();
  // 5 4     cafeca60 cafeca40 cafeca20 cafeca80 0 0 0 0

  set->replace(CacheTag(0xCAFECA00));
  set->print();
  // 5 3     cafeca60 cafeca40 cafeca20 cafeca80 cafeca00 0 0 0

  set->modifyAssociativity(4);
  set->print();
  // 4 3     cafeca60 cafeca40 cafeca20 cafeca80 0 0 0 0

  set->modifyAssociativity(3);
  set->print();
  // 3 2     cafeca60 cafeca40 cafeca20 0 0 0 0 0

  set->modifyAssociativity(1);
  set->print();
  // 1 0     cafeca60 0 0 0 0 0 0 0

  set->modifyAssociativity(8);
  set->replace(0xCAFECA00);
  set->print();
  // 8 6     cafeca60 0 0 0 0 0 0 cafeca00

  //error checking
  //set->modifyAssociativity(0);
  //set->modifyAssociativity(9);
}

void test_cache_resizing()
{
  OCache *cache = new OCache("Test Cache", 
                             TestKnobCacheSize.Value() * k_KILO,
                             TestKnobLineSize.Value(),
                             TestKnobAssociativity.Value(),
                             TestKnobMutationInterval.Value(),
                             TestKnobDCacheThresholdHit.Value(),
                             TestKnobDCacheThresholdMiss.Value(),
                             (TestKnobCacheSize.Value() * k_KILO) >> 1,
                             TestKnobAssociativity.Value() >> 1,
                             TestKnobDCacheMaxSearchDepth.Value(),
                             TestKnobICacheThresholdHit.Value(),
                             TestKnobICacheThresholdMiss.Value(),
                             (TestKnobCacheSize.Value() * k_KILO) >> 1,
                             TestKnobAssociativity.Value() >> 1,
                             TestKnobICacheMaxSearchDepth.Value());

  cout << "*** testing cache resizing via mutation ***" << endl;

  cout << dec << "CacheSize  = " << TestKnobCacheSize.Value() * k_KILO << endl
              << "DCacheSize = " << cache->dCacheSize()   << endl
              << "ICacheSize = " << cache->iCacheSize() << endl << endl;

  ASSERTX( cache->dCacheSize() + cache->iCacheSize() == TestKnobCacheSize.Value() * k_KILO );

  // under EvolveNaive, should cause dcache to grow
  cache->runDCacheLoadModel(0xCAFECAE0, 4);
  cache->runDCacheLoadModel(0xCAFECB00, 4);
  cache->runDCacheStoreModel(0xCAFECB20, 4);
  cout << "3 accesses complete" << endl;
  cache->runDCacheLoadModel(0xCAFECB40, 4);
  cout << "4 accesses complete" << endl;

  // under EvolveNaive, should cause icache to grow
  cache->runICacheLoadModel(0xCAFECAE0, 4);
  cache->runICacheLoadModel(0xCAFECB00, 4);
  cache->runICacheLoadModel(0xCAFECB20, 4);
  cout << "3 accesses complete" << endl;
  cache->runICacheLoadModel(0xCAFECB40, 4);
  cout << "4 accesses complete" << endl;  
  
  // under EvolveNaive, should cause no change
  cache->runDCacheLoadModel(0xCAFECB40, 4);  //hit
  cache->runDCacheStoreModel(0xCAFECB60, 4); //miss
  cache->runICacheLoadModel(0xCAFECB60, 4);  //miss
  cout << "3 accesses complete" << endl;
  cache->runICacheLoadModel(0xCAFECB80, 4);  //miss
  cout << "4 accesses complete" << endl;

  cout << endl << dec << cache->StatsLong() << endl;
}

void test_multicycle_hashing()
{

  OCache *cache = new OCache("Test Cache", 
                             TestKnobCacheSize.Value() * k_KILO,
                             TestKnobLineSize.Value(),
                             TestKnobAssociativity.Value(),
                             10000,
                             TestKnobDCacheThresholdHit.Value(),
                             TestKnobDCacheThresholdMiss.Value(),
                             (TestKnobCacheSize.Value() * k_KILO) >> 1,
                             TestKnobAssociativity.Value() >> 1,
                             TestKnobDCacheMaxSearchDepth.Value(),
                             TestKnobICacheThresholdHit.Value(),
                             TestKnobICacheThresholdMiss.Value(),
                             (TestKnobCacheSize.Value() * k_KILO) >> 1,
                             TestKnobAssociativity.Value() >> 1,
                             TestKnobICacheMaxSearchDepth.Value());

  cout << "*** testing multicylce hashing ***" << endl;


  cache->runDCacheLoadModel(0xCAFECAE0 + 0*8192, 4); //miss
  cache->runDCacheLoadModel(0xCAFECAE0 + 1*8192, 4); //miss
  cache->runDCacheLoadModel(0xCAFECAE0 + 2*8192, 4); //miss
  cache->runDCacheLoadModel(0xCAFECAE0 + 3*8192, 4); //miss
  cache->runDCacheLoadModel(0xCAFECAE0 + 4*8192, 4); //miss

  cache->runDCacheLoadModel(0xCAFECAE0 + 4*8192, 4); //hit
  cache->runDCacheLoadModel(0xCAFECAE0 + 3*8192, 4); //hit
  cache->runDCacheLoadModel(0xCAFECAE0 + 2*8192, 4); //hit 
  cache->runDCacheLoadModel(0xCAFECAE0 + 1*8192, 4); //hit
  cache->runDCacheLoadModel(0xCAFECAE0 + 0*8192, 4); //miss

  // 6/10 miss rate
  cout << endl << dec << cache->StatsLong() << endl;


  cache->runDCacheLoadModel(0*8192, 4); //miss
  cache->runDCacheLoadModel(0*8192, 4); //hit
  cache->runDCacheLoadModel(0*8192, 4); //hit
  cache->runDCacheLoadModel(1*8192, 4); //miss
  cache->runDCacheLoadModel(1*8192, 4); //hit
  cache->runDCacheLoadModel(2*8192, 4); //miss
  cache->runDCacheLoadModel(3*8192, 4); //miss  
  cache->runDCacheLoadModel(0*8192, 4); //hit
  cache->runDCacheLoadModel(1*8192, 4); //hit
  cache->runDCacheLoadModel(2*8192, 4); //hit
  cache->runDCacheLoadModel(3*8192, 4); //hit

  cache->runDCacheLoadModel(4*8192, 4); //miss
  cache->runDCacheLoadModel(0*8192, 4); //miss
  cache->runDCacheLoadModel(4*8192, 4); //hit

  // 6/14 miss rate, 12/24 total miss rate
  cout << endl << dec << cache->StatsLong() << endl;

  // extend set 0 into set 1
  // contents of set 0: 3 2 0 4
  cache->DCacheSetSetPtr(0,2);

  // if 1*8192 gets placed in set 1, only first misses; rest hit
  cache->runDCacheLoadModel(1*8192, 4); 
  cache->runDCacheLoadModel(0*8192, 4); 
  cache->runDCacheLoadModel(1*8192, 4);
  cache->runDCacheLoadModel(2*8192, 4);
  cache->runDCacheLoadModel(3*8192, 4); 
  cache->runDCacheLoadModel(4*8192, 4); 

  //FIXME: communicate the result of this test to stdout somehow

}

int main(int argc, char *argv[])
{
  PIN_InitSymbols();

  if( PIN_Init(argc,argv) )
    return Usage();

  test_set_resizing();
  test_cache_resizing();
  test_multicycle_hashing();

  return 0;
}
