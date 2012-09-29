/*****************************************************************************
 * Graphite-McPAT Core Interface
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "XML_Parse.h"
#include "core.h"
#include "cacti_interface.h"

namespace McPAT
{

class InputParameter;

//---------------------------------------------------------------------------
// Core Wrapper Class
//---------------------------------------------------------------------------
class CoreWrapper
{
  public:
    ParseXML *XML;
    Core* core;

    // Create CoreWrapper
    CoreWrapper(ParseXML *XML_interface);
    // Compute Energy
    void computeEnergy();
    // Set interface_ip Parameters
    void set_proc_param(InputParameter& interface_ip);
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
