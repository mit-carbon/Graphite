/*****************************************************************************
 * Graphite-McPAT Core Interface
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

namespace McPAT
{

//---------------------------------------------------------------------------
// Core Wrapper Class
//---------------------------------------------------------------------------
class CoreWrapper : public Component
{
  public:
    ParseXML *XML;
    vector<Core *> cores;
    InputParameter interface_ip;
    ProcParam procdynp;
    Component core;
    int  numCore;

    // Create CoreWrapper
    CoreWrapper(ParseXML *XML_interface);
    // Compute Energy
    void computeEnergy();
    // Set CoreWrapper Parameters
    void set_proc_param();
    // Print Energy from CoreWrapper
    void displayEnergy(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    // Print Device Type
    void displayDeviceType(int device_type_, uint32_t indent = 0);
    // Print Interconnect Type
    void displayInterconnectType(int interconnect_type_, uint32_t indent = 0);
    // Delete CoreWrapper
    ~CoreWrapper();
};

}
