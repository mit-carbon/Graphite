#include "DSENTRouter.h"
#include "DSENTInterface.h"
#include "dsent-core/libutil/String.h"

#include <iostream>
#include <cassert>
#include <vector>

using namespace std;
using namespace LibUtil;

namespace dsent_contrib
{
    DSENTRouter::DSENTRouter(
            float frequency_,
            unsigned int num_in_port_,
            unsigned int num_out_port_,
            unsigned int num_vclass_,
            unsigned int num_vchannel_,
            unsigned int num_buffers_,
            unsigned int flit_width_,
            const DSENTInterface* dsent_interface_
            )
    {
        assert(frequency_ > 0);
        assert((num_in_port_ == num_in_port_) && (num_in_port_ != 0));
        assert((num_out_port_ == num_out_port_) && (num_out_port_ != 0));
        assert((num_vclass_ == num_vclass_) && (num_vclass_ != 0));
        assert((num_vchannel_ == num_vchannel_) && (num_vchannel_ != 0));
        assert(num_buffers_ == num_buffers_);
        assert((flit_width_ == flit_width_) && (flit_width_ != 0));

        init(frequency_, num_in_port_, num_out_port_, num_vclass_, num_vchannel_,
                num_buffers_, flit_width_, dsent_interface_);
    }

    DSENTRouter::~DSENTRouter()
    {}

    void DSENTRouter::init(
            float frequency_,
            unsigned int num_in_port_,
            unsigned int num_out_port_,
            unsigned int num_vclass_,
            unsigned int num_vchannel_,
            unsigned int num_buffers_,
            unsigned int flit_width_,
            const DSENTInterface* dsent_interface_
            )
    {
        // Create DSENT evaluate
        vector<String> eval = vector<String>();
        // Create DSENT overwrites vector
        vector<DSENTInterface::Overwrite> overwrites = vector<DSENTInterface::Overwrite>();
        // DSENT outputs
        vector<String> outputs;

        // Create evaluate String    
        // Dynamic power components
        eval.push_back( "$(Energy>>Router:WriteBuffer)");
        eval.push_back( "$(Energy>>Router:ReadBuffer)");
        eval.push_back( "($(Energy>>Router:ArbitrateSwitch->ArbitrateStage1) + "
                "$(Energy>>Router:ArbitrateSwitch->ArbitrateStage2))");
        eval.push_back( "$(Energy>>Router:TraverseCrossbar->Multicast1)");
        eval.push_back( "$(Energy>>Router:DistributeClock)");

        // Static power components
        eval.push_back( "$(NddPower>>Router->InputPort:Leakage) * $(NumberInputPorts) + "
                "($(NddPower>>Router->PipelineReg0:Leakage) + $(NddPower>>Router->PipelineReg1:Leakage)) * "
                "$(NumberInputPorts) * $(NumberBitsPerFlit)");
        eval.push_back( "$(NddPower>>Router->SwitchAllocator:Leakage)");
        eval.push_back( "$(NddPower>>Router->Crossbar:Leakage) + "
                "$(NddPower>>Router->PipelineReg2_0:Leakage) * $(NumberOutputPorts) * $(NumberBitsPerFlit);");
        eval.push_back( "$(NddPower>>Router->ClockTree:Leakage)");

        // Create overwrites
        overwrites.push_back(DSENTInterface::Overwrite("Frequency", String(frequency_)));
        overwrites.push_back(DSENTInterface::Overwrite("NumberInputPorts", String(num_in_port_)));
        overwrites.push_back(DSENTInterface::Overwrite("NumberOutputPorts", String(num_out_port_)));
        overwrites.push_back(DSENTInterface::Overwrite("NumberVirtualNetworks", String(num_vclass_))); 
        overwrites.push_back(DSENTInterface::Overwrite("NumberVirtualChannelsPerVirtualNetwork",
                    vectorToString<unsigned int>(vector<unsigned int>(num_vclass_, num_vchannel_))));
        overwrites.push_back(DSENTInterface::Overwrite("NumberBuffersPerVirtualChannel",
                    vectorToString<unsigned int>(vector<unsigned int>(num_vclass_, num_buffers_))));
        overwrites.push_back(DSENTInterface::Overwrite("NumberBitsPerFlit", String(flit_width_)));

        // Run DSENT
        outputs = dsent_interface_->run_dsent(dsent_interface_->get_router_cfg_file_path(), eval, overwrites);

        // Check to make sure we get exactly 9 outputs
        assert(outputs.size() == 9);

        // Store outputs
        // Dynamic energy components
        m_dynamic_energy_buf_write_ = outputs[0];
        m_dynamic_energy_buf_read_ = outputs[1];
        m_dynamic_energy_sa_ = outputs[2];
        m_dynamic_energy_xbar_ = outputs[3];
        m_dynamic_energy_clock_ = outputs[4];

        // Static power components
        m_static_power_buf_ = outputs[5];
        m_static_power_xbar_ = outputs[6];
        m_static_power_sa_ = outputs[7];
        m_static_power_clock_ = outputs[8];

        return;
    }

    void DSENTRouter::print() const
    {
        cout << "Router - Dynamic Energy" << endl;
        cout << "\t" << "Buffer Write = " << calc_dynamic_energy_buf_write(1) << endl;
        cout << "\t" << "Buffer Read = " << calc_dynamic_energy_buf_read(1) << endl;
        cout << "\t" << "SW Allocator = " << calc_dynamic_energy_sa(1) << endl;
        cout << "\t" << "Crossbar = " << calc_dynamic_energy_xbar(1) << endl;
        cout << "\t" << "Clock = " << calc_dynamic_energy_clock(1) << endl;
        cout << endl;
        cout << "Router - Static Power" << endl;
        cout << "\t" << "Buffer = " << get_static_power_buf() << endl;
        cout << "\t" << "SW Allocator = " << get_static_power_sa() << endl;
        cout << "\t" << "Crossbar = " << get_static_power_xbar() << endl;
        cout << "\t" << "Clock = " << get_static_power_clock() << endl;
        cout << endl;
        return;
    }
}

