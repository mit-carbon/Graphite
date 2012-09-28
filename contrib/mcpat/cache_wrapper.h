/*****************************************************************************
 * Graphite-McPAT Cache Interface
 ***************************************************************************/

#pragma once

#include "XML_Parse.h"
#include "area.h"
#include "decoder.h"
#include "parameter.h"
#include "array.h"
#include "arbiter.h"
#include <vector>
#include "basic_components.h"
#include "core.h"
#include "memoryctrl.h"
#include "router.h"
#include "sharedcache.h"
#include "noc.h"

namespace McPAT
{

//---------------------------------------------------------------------------
// Cache Wrapper Class
//---------------------------------------------------------------------------
class CacheWrapper : public Component
{
  public:
    ParseXML *XML;
    SharedCache * cache;
    InputParameter interface_ip;

    // Create CacheWrapper
    CacheWrapper(ParseXML *XML_interface);
    // Compute Energy
    void computeEnergy();
    // Set CacheWrapper Parameters
    void set_proc_param();
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
