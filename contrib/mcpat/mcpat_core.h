/*****************************************************************************
 * Graphite-McPAT Core Interface
 ***************************************************************************/

#pragma once

// [graphite]

#include "XML_Parse.h"
#include "area.h"
#include "decoder.h"
#include "parameter.h"
#include "array.h"
#include "arbiter.h"
#include <vector>
#include "basic_components.h"
#include "core.h"

namespace McPAT
{

//---------------------------------------------------------------------------
// McPAT Core Class
//---------------------------------------------------------------------------
class McPATCore : public Component
{
  public:
    ParseXML *XML;
    vector<Core *> cores;
    InputParameter interface_ip;
    ProcParam procdynp;
    Component core;
    int  numCore;

    // Create McPATCore
    McPATCore(ParseXML *XML_interface);
    // Compute Energy
    void computeEnergy();
    // Set McPATCore Parameters
    void set_proc_param();
    // Print Energy from McPATCore
    void displayEnergy(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    // Print Device Type
    void displayDeviceType(int device_type_, uint32_t indent = 0);
    // Print Interconnect Type
    void displayInterconnectType(int interconnect_type_, uint32_t indent = 0);
    // Delete McPATCore
    ~McPATCore();
};

}
