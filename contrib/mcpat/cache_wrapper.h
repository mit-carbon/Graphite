/*****************************************************************************
 * Graphite-McPAT Cache Interface
 ***************************************************************************/

#pragma once

#include "XML_Parse.h"
#include "sharedcache.h"
#include "cacti_interface.h"

namespace McPAT
{

class InputParameter;

//---------------------------------------------------------------------------
// Cache Wrapper Class
//---------------------------------------------------------------------------
class CacheWrapper
{
  public:
    ParseXML* XML;
    SharedCache* cache;

    // Create CacheWrapper
    CacheWrapper(ParseXML *XML_interface);
    // Compute Energy
    void computeEnergy();
    // Set CacheWrapper Parameters
    void set_proc_param(InputParameter& interface_ip);
    // Print Energy from CacheWrapper
    void displayEnergy(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    // Print Device Type
    void displayDeviceType(int device_type_, uint32_t indent = 0);
    // Print Interconnect Type
    void displayInterconnectType(int interconnect_type_, uint32_t indent = 0);
    // Delete CacheWrapper
    ~CacheWrapper();
};

}
