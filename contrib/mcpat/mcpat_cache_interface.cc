/*****************************************************************************
 * Graphite-McPAT Cache Interface
 ***************************************************************************/

#include "mcpat_cache_interface.h"

//---------------------------------------------------------------------------
// McPAT Cache Interface Constructor
//---------------------------------------------------------------------------
McPATCacheInterface::McPATCacheInterface()
{
   // Initialize Architectural Parameters
   initializeArchitecturalParameters();
   // Initialize Event Counters
   initializeEventCounters();
   // Initialize Output Data Structure
   initializeOutputDataStructure();

   // Make a ParseXML Object and Initialize it
   _xml = new McPAT::ParseXML();

   // Initialize ParseXML Params and Stats
   _xml->initialize();
   _xml->setNiagara1();

   // Fill the ParseXML's Core Params from McPATCacheInterface
   _xml->fillCacheParamsFromMcPATCacheInterface(this);

   // Make a Processor Object from the ParseXML
   _cache_wrapper = new McPAT::CacheWrapper(_xml);
}

//---------------------------------------------------------------------------
// McPAT Core Interface Destructor
//---------------------------------------------------------------------------
McPATCacheInterface::~McPATCacheInterface()
{
   delete _cache_wrapper;
   delete _xml;
}

//---------------------------------------------------------------------------
// Initialize Architectural Parameters
//---------------------------------------------------------------------------
void McPATCacheInterface::initializeArchitecturalParameters()
{
   // System Parameters
   _clock_rate = 1000;
   _tech_node = 45;
   // Architectural Parameters
}

//---------------------------------------------------------------------------
// Initialize Event Counters
//---------------------------------------------------------------------------
void McPATCacheInterface::initializeEventCounters()
{
   // Event Counters
}

//---------------------------------------------------------------------------
// Initialize Output Data Structure
//---------------------------------------------------------------------------
void McPATCacheInterface::initializeOutputDataStructure()
{
   mcpat_cache_out.area                           = 0;
   mcpat_cache_out.leakage                        = 0;
   mcpat_cache_out.longer_channel_leakage         = 0;
   mcpat_cache_out.gate_leakage                   = 0;
   mcpat_cache_out.dynamic                        = 0;
}

//---------------------------------------------------------------------------
// Compute Energy from McPat
//---------------------------------------------------------------------------
void McPATCacheInterface::computeEnergy()
{
   // Fill the ParseXML's Core Event Stats from McPATCacheInterface
   _xml->fillCacheStatsFromMcPATCacheInterface(this);

   // Compute Energy from Processor
   _cache_wrapper->computeEnergy();

   // Execution Time
   executionTime = (_cache_wrapper->cache->cachep.executionTime);

   // Store Energy into Data Structure
   // Core
   mcpat_cache_out.area                   = _cache_wrapper->cache->area.get_area()*1e-6;
   mcpat_cache_out.leakage                = _cache_wrapper->cache->power.readOp.leakage*executionTime;
   mcpat_cache_out.longer_channel_leakage = _cache_wrapper->cache->power.readOp.longer_channel_leakage*executionTime;
   mcpat_cache_out.gate_leakage           = _cache_wrapper->cache->power.readOp.gate_leakage*executionTime;
   mcpat_cache_out.dynamic                = _cache_wrapper->cache->rt_power.readOp.dynamic;
}

//---------------------------------------------------------------------------
// Display Energy from McPAT
//---------------------------------------------------------------------------
void McPATCacheInterface::displayEnergy()
{
   // Test Output from Data Structure
   int indent = 2;
   string indent_str(indent, ' ');
   string indent_str_next(indent+2, ' ');
   cout <<"*****************************************************************************************"<<endl;
   cout << endl;
   cout << "Cache:" << endl;
   cout << indent_str << "Area = "                   << mcpat_cache_out.area                   << " mm^2" << endl;
   cout << indent_str << "Leakage = "                << mcpat_cache_out.leakage                << " W" << endl;
   cout << indent_str << "Longer Channel Leakage = " << mcpat_cache_out.longer_channel_leakage << " W" << endl;
   cout << indent_str << "Gate Leakage = "           << mcpat_cache_out.gate_leakage           << " W" << endl;
   cout << indent_str << "Runtime Dynamic = "        << mcpat_cache_out.dynamic                << " J" << endl;
   cout << endl;
   cout <<"*****************************************************************************************"<<endl;
}

//---------------------------------------------------------------------------
// Display Architectural Parameters
//---------------------------------------------------------------------------
void McPATCacheInterface::displayParam()
{
   cout << "  Cache Parameters:" << endl;
   // System Parameters
   cout << "    clock_rate : " << _clock_rate << endl;
   cout << "    tech_node : " << _tech_node << endl;
   // Architectural Parameters
}

//---------------------------------------------------------------------------
// Display Event Counters
//---------------------------------------------------------------------------
void McPATCacheInterface::displayStats()
{
   // Event Counters
   cout << "  Cache Statistics:" << endl;
}

//---------------------------------------------------------------------------
// Set Event Counters
//---------------------------------------------------------------------------
void McPATCacheInterface::setEventCounters()
{
   // Event Counters
}
