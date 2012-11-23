#include "DSENTElectricalLink.h"
#include "DSENTInterface.h"
#include "dsent-core/libutil/String.h"

#include <iostream>
#include <cassert>

using namespace std;
using namespace LibUtil;

namespace dsent_contrib
{
    DSENTElectricalLink::DSENTElectricalLink(float frequency_, double link_len_, unsigned int flit_width_, const DSENTInterface* dsent_interface_)
    {
        assert(frequency_ > 0);
        assert(link_len_ >= 0);
        assert(flit_width_ > 0);
        init(frequency_, link_len_, flit_width_, dsent_interface_);
    }

    DSENTElectricalLink::~DSENTElectricalLink()
    {}

    void DSENTElectricalLink::init(float frequency_, double link_len_, unsigned int flit_width_, const DSENTInterface* dsent_interface_)
    {
        // Create DSENT evaluate
        vector<String> eval = vector<String>();
        // Create DSENT overwrites vector
        vector<DSENTInterface::Overwrite> overwrites = vector<DSENTInterface::Overwrite>();
        // DSENT outputs
        vector<String> outputs;

        // Create evaluate String
        eval.push_back("$(Energy>>RepeatedLink:Send)");
        eval.push_back("$(NddPower>>RepeatedLink:Leakage)");
        // Create overwrites
        overwrites.push_back(DSENTInterface::Overwrite("Delay", String(1.0 / frequency_)));
        overwrites.push_back(DSENTInterface::Overwrite("WireLength", String(link_len_)));
        overwrites.push_back(DSENTInterface::Overwrite("NumberBits", String(flit_width_)));

        // Run DSENT
        outputs = dsent_interface_->run_dsent(dsent_interface_->get_el_link_cfg_file_path(), eval, overwrites);

        // Check to make sure we get exactly 2 outputs
        assert(outputs.size() == 2);

        // Store outputs
        m_dynamic_energy_per_flit_ = outputs[0];
        m_static_power_ = outputs[1];

        return;
    }

    void DSENTElectricalLink::print() const
    {
        cout << "Link - Dynamic Energy Per Flit = " << calc_dynamic_energy(1) << endl;
        cout << "Link - Static Power = " << get_static_power() << endl;
        return;
    }
}

