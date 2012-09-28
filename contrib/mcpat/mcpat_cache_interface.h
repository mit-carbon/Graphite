/*****************************************************************************
 * Graphite-McPAT Cache Interface
 ***************************************************************************/

#pragma once

#include "XML_Parse.h"
#include "cache_wrapper.h"
#include <iostream>

using namespace std;

//---------------------------------------------------------------------------
// McPAT Cache Interface Data Structures for Area and Power
//---------------------------------------------------------------------------
typedef struct
{
   double area;                           // Area
   double leakage;                        // Subthreshold Leakage
   double longer_channel_leakage;         // Subthreshold Leakage
   double gate_leakage;                   // Gate Leakage
   double dynamic;                        // Runtime Dynamic
} mcpat_cache_output;

class McPATCacheInterface
{
public:
   // Execution Time
   double executionTime;
   // McPAT Objects
   McPAT::ParseXML* _xml;
   McPAT::CacheWrapper* _cache_wrapper;
   // McPAT Output Data Structure
   mcpat_cache_output mcpat_cache_out;

   // McPAT Cache Interface Constructor
   McPATCacheInterface();
   // McPAT Cache Interface Destructor
   ~McPATCacheInterface();

   // Initialize Architectural Parameters
   void initializeArchitecturalParameters();
   // Initialize Event Counters
   void initializeEventCounters();
   // Initialize Output Data Structure
   void initializeOutputDataStructure();

   // Compute Energy from McPAT
   void computeEnergy();

   // Display Energy from McPAT
   void displayEnergy();
   // Display Architectural Parameters
   void displayParam();
   // Display Event Counters
   void displayStats();

   // Set Event Counters
   void setEventCounters();

public:
   // System Parameters
   // |---- General Parameters
   int _clock_rate;
   double _tech_node;

   // Architectural Parameters

   // Event Counters
};
